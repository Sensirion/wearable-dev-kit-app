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
#include "app_onbody_demo.h"

static const char *ONBODY_TEXT[] = { "OFF BODY", "ON BODY" };
static const char *ONBODY_TITLE = "On/Off Body";
static const GColor ONBODY_COLOR[] = { {GColorRedARGB8}, {GColorIslamicGreenARGB8} };

static struct {
  Window *window;
  TextLayer *title_layer;
  TextLayer *onbody_text_layer;
  bool onbody_state;
  Dialog dialog;
} app;

static void update_connection_text(bool connected) {
  layer_set_hidden(app.dialog.layer, connected);
}

static void update_onbody_state_text(bool onbody) {
  text_layer_set_text(app.onbody_text_layer, ONBODY_TEXT[onbody]);
  text_layer_set_background_color(app.onbody_text_layer, ONBODY_COLOR[onbody]);
}

static void on_load_window(Window *window) {
  sensismart_window_load(&AppOnbodyDemo);
  Layer *root_layer = window_get_root_layer(window);
  GFont gothic_24 = fonts_get_system_font(FONT_KEY_GOTHIC_24);
  bool onbody = app.onbody_state;

  // Screen Title
  app.title_layer = text_layer_create(GRect(0, 0, 144, 20));
  text_layer_set_font(app.title_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18));
  text_layer_set_text(app.title_layer, ONBODY_TITLE);
  text_layer_set_text_color(app.title_layer, GColorWhite);
  text_layer_set_background_color(app.title_layer, GColorBlack);
  text_layer_set_text_alignment(app.title_layer, GTextAlignmentCenter);
  layer_add_child(root_layer, text_layer_get_layer(app.title_layer));

  app.onbody_text_layer = text_layer_create(GRect(0, 28, 144, 40));
  text_layer_set_font(app.onbody_text_layer, gothic_24);
  text_layer_set_text_alignment(app.onbody_text_layer, GTextAlignmentCenter);
  text_layer_set_background_color(app.onbody_text_layer, ONBODY_COLOR[onbody]);
  update_onbody_state_text(onbody);
  layer_add_child(root_layer, text_layer_get_layer(app.onbody_text_layer));

  // Sensirion Logo
  layer_add_child(root_layer, sensismart_get_branding_layer());

  // Dialog Box for Disconnect Events
  dialog_create_disconnect_warning(&app.dialog);
  layer_add_child(root_layer, app.dialog.layer);
  layer_set_hidden(app.dialog.layer, bp_get_status());
}

static void on_unload_window(Window *window) {
  text_layer_destroy(app.title_layer);
  text_layer_destroy(app.onbody_text_layer);
  dialog_destroy(&app.dialog);
  window_destroy(app.window);
}

static void on_connection_state_changed(bool connected) {
  update_connection_text(connected);
}

static void on_onbody_event(bool onbody) {
  app.onbody_state = onbody;
  update_onbody_state_text(onbody);
}

static void click_config_provider(Window *window) {
  sensismart_setup_controls(&AppOnbodyDemo);
}

static void activate() {
  AppOnbodyDemo.window = window_create();
  app.window = AppOnbodyDemo.window;
  window_set_window_handlers(app.window, (WindowHandlers) {
    .load = on_load_window,
    .unload = on_unload_window
  });
  window_set_click_config_provider(app.window, (ClickConfigProvider) click_config_provider);
  window_stack_push(app.window, true);
  bp_subscribe((BackpackHandlers) {
    .on_connection_state_changed = on_connection_state_changed,
    .on_onbody_event = on_onbody_event
  });
}

static void deactivate() {
  window_stack_pop(true);
  app.window = NULL;
  bp_unsubscribe();
}

static void load() {
}

SensiSmartApp AppOnbodyDemo = {
  .name = "OnbodyDemo",
  .window = NULL,
  .activate = activate,
  .deactivate = deactivate,
  .load = load,
};

