/*
 * Copyright (c) 2016, Sensirion AG
 * Author: Andreas Brauchli <andreas.brauchli@sensirion.com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * * Redistributions of source code must retain the above copyright notice, this
 *   list of conditions and the following disclaimer.
 *
 * * Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 *
 * * Neither the name of Sensirion AG nor the names of its
 *   contributors may be used to endorse or promote products derived from
 *   this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <pebble.h>
#include "backpack.h"
#include "SensiSmartApp.h"
#include "utils.h"
#include "app_temp_compensation.h"

static struct {
  Window *window;
  TextLayer *status_layer;
  TextLayer *skin_text_layer;
  TextLayer *feel_like_text_layer;
  TextLayer *apparent_text_layer;
  char skin_text_layer_buf[20];
  char feel_like_text_layer_buf[20];
  char apparent_text_layer_buf[20];
} app;

static void update_connection_text(bool connected) {
  if (connected) {
    text_layer_set_text(app.status_layer, "Connected!");
  } else {
    text_layer_set_text(app.status_layer, "Connecting...");
  }
}

static void on_connection_state_changed(bool connected) {
  update_connection_text(connected);
}

static void on_load_window(Window *window) {
  sensismart_window_load(&AppTempCompensation);
  Layer *root_layer = window_get_root_layer(window);

  app.status_layer = text_layer_create(GRect(0, 5, 144, 40));
  text_layer_set_font(app.status_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18));
  update_connection_text(bp_get_status());
  text_layer_set_text_color(app.status_layer, GColorWhite);
  text_layer_set_background_color(app.status_layer, GColorBlack);
  text_layer_set_text_alignment(app.status_layer, GTextAlignmentCenter);
  text_layer_set_overflow_mode(app.status_layer, GTextOverflowModeWordWrap);
  layer_add_child(root_layer, text_layer_get_layer(app.status_layer));

  app.skin_text_layer = text_layer_create(GRect(0, 28, 144, 40));
  text_layer_set_font(app.skin_text_layer, fonts_get_system_font(FONT_KEY_GOTHIC_28));
  text_layer_set_text(app.skin_text_layer, "-");
  text_layer_set_text_color(app.skin_text_layer, GColorBrightGreen);
  text_layer_set_background_color(app.skin_text_layer, GColorBlack);
  text_layer_set_text_alignment(app.skin_text_layer, GTextAlignmentCenter);
  text_layer_set_overflow_mode(app.skin_text_layer, GTextOverflowModeWordWrap);
  layer_add_child(root_layer, text_layer_get_layer(app.skin_text_layer));

  app.feel_like_text_layer = text_layer_create(GRect(0, 60, 144, 40));
  text_layer_set_font(app.feel_like_text_layer, fonts_get_system_font(FONT_KEY_GOTHIC_28));
  text_layer_set_text(app.feel_like_text_layer, "-");
  text_layer_set_text_color(app.feel_like_text_layer, GColorBrightGreen);
  text_layer_set_background_color(app.feel_like_text_layer, GColorBlack);
  text_layer_set_text_alignment(app.feel_like_text_layer, GTextAlignmentCenter);
  text_layer_set_overflow_mode(app.feel_like_text_layer, GTextOverflowModeWordWrap);
  layer_add_child(root_layer, text_layer_get_layer(app.feel_like_text_layer));

  app.apparent_text_layer = text_layer_create(GRect(0, 92, 144, 40));
  text_layer_set_font(app.apparent_text_layer, fonts_get_system_font(FONT_KEY_GOTHIC_28));
  text_layer_set_text(app.apparent_text_layer, "-");
  text_layer_set_text_color(app.apparent_text_layer, GColorBrightGreen);
  text_layer_set_background_color(app.apparent_text_layer, GColorBlack);
  text_layer_set_text_alignment(app.apparent_text_layer, GTextAlignmentCenter);
  text_layer_set_overflow_mode(app.apparent_text_layer, GTextOverflowModeWordWrap);
  layer_add_child(root_layer, text_layer_get_layer(app.apparent_text_layer));

  layer_add_child(root_layer, sensismart_get_branding_layer());
}

static void on_unload_window(Window *window) {
  text_layer_destroy(app.status_layer);
  text_layer_destroy(app.skin_text_layer);
  text_layer_destroy(app.feel_like_text_layer);
  text_layer_destroy(app.apparent_text_layer);
  window_destroy(app.window);
}

static void on_processed_values(float t_skin, float t_feellike,
                                float t_apparent, float t_humidex) {
  char fbuf[16];

  /* skin temperature */
  ftoa(fbuf, t_skin, 2);
  snprintf(app.skin_text_layer_buf, sizeof(app.skin_text_layer_buf),
           "%s °C", fbuf);
  text_layer_set_text(app.skin_text_layer, app.skin_text_layer_buf);

  /* feellike temperature */
  ftoa(fbuf, t_feellike, 2);
  snprintf(app.feel_like_text_layer_buf, sizeof(app.feel_like_text_layer_buf),
           "%s °C", fbuf);
  text_layer_set_text(app.feel_like_text_layer, app.feel_like_text_layer_buf);

  /* apparent temperature */
  ftoa(fbuf, t_apparent, 2);
  snprintf(app.apparent_text_layer_buf, sizeof(app.apparent_text_layer_buf),
           "%s °C", fbuf);
  text_layer_set_text(app.apparent_text_layer, app.apparent_text_layer_buf);
}

static void click_config_provider(Window *window) {
  sensismart_setup_controls(&AppTempCompensation);
}

static void activate() {
  AppTempCompensation.window = window_create();
  app.window = AppTempCompensation.window;
  window_set_window_handlers(app.window, (WindowHandlers) {
    .load = on_load_window,
    .unload = on_unload_window
  });
  window_set_click_config_provider(app.window, (ClickConfigProvider) click_config_provider);
  bp_subscribe((BackpackHandlers) {
    .on_connection_state_changed = on_connection_state_changed,
    .on_processed_values = on_processed_values
  });
  window_stack_push(app.window, true);
}

static void deactivate() {
  window_stack_pop(true);
  bp_unsubscribe();
}

SensiSmartApp AppTempCompensation = {
  .name = "TempCompensation",
  .window = NULL,
  .activate = activate,
  .deactivate = deactivate
};

