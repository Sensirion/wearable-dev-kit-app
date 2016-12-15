/*
 * Copyright (c) 2016, Sensirion AG
 * Author: Bjoern Muntwyler <bjoern.muntwyler@sensirion.com>
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
#include "app_thermal_values.h"

static const uint8_t DEFAULT_COMPENSATION_MODE = 2;
static const char LABEL_SKIN_TEXT[] = "skin";
static const char LABEL_FEELS_LIKE_TEXT[] = "feels like";
static const char EMPTY_VALUE_TEXT[] = "-";
static const char CONNECTED_TEXT[] = "Connected!";
static const char CONNECTING_TEXT[] = "Connecting...";
static const char TEMPERATURE_FORMAT[] = "%s °C";
static const char UNKNOWN_COMPENSATION_MODE_NAME[] = "unknown";
static const char THERMAL_VALUES_TITLE[] = "Thermal Values";

static const char *COMPENSATION_MODE_NAMES[] = {
  "non-accelerated",
  "smooth",
  "balanced",
  "responsive",
};

static const size_t NUMBER_OF_COMPENSATION_MODES = sizeof(COMPENSATION_MODE_NAMES)/sizeof(COMPENSATION_MODE_NAMES[0]);

static struct {
  Window *window;
  TextLayer *title_layer;
  TextLayer *skin_text_layer;
  TextLayer *feel_like_text_layer;
  TextLayer *skin_label_text_layer;
  TextLayer *feel_like_label_text_layer;
  TextLayer *mode_name_text_layer;
  char skin_text_layer_buf[20];
  char feel_like_text_layer_buf[20];
  uint8_t current_compensation_mode;
  uint8_t number_of_compensation_modes;
  GBitmap *res_bmp_logo_black;
  BitmapLayer *bmp_logo_black_layer;
  Dialog dialog;
} app;

static void update_connection_text(bool connected) {
  layer_set_hidden(app.dialog.layer, connected);
}

static const char *current_compensation_mode_name() {
  if (app.current_compensation_mode < NUMBER_OF_COMPENSATION_MODES)
    return COMPENSATION_MODE_NAMES[app.current_compensation_mode];
  return UNKNOWN_COMPENSATION_MODE_NAME;
}

static void on_load_window(Window *window) {
  sensismart_window_load(&AppThermalValues);
  Layer *root_layer = window_get_root_layer(window);
  GFont gothic_24 = fonts_get_system_font(FONT_KEY_GOTHIC_24);
  GFont gothic_28 = fonts_get_system_font(FONT_KEY_GOTHIC_28);

  // Screen Title
  app.title_layer = text_layer_create(GRect(0, 0, 144, 20));
  text_layer_set_font(app.title_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18));
  text_layer_set_text(app.title_layer, THERMAL_VALUES_TITLE);
  text_layer_set_text_color(app.title_layer, GColorWhite);
  text_layer_set_background_color(app.title_layer, GColorBlack);
  text_layer_set_text_alignment(app.title_layer, GTextAlignmentCenter);
  layer_add_child(root_layer, text_layer_get_layer(app.title_layer));

  app.skin_label_text_layer = text_layer_create(GRect(0, 28, 60, 40));
  text_layer_set_font(app.skin_label_text_layer, gothic_24);
  text_layer_set_text(app.skin_label_text_layer, LABEL_SKIN_TEXT);
  text_layer_set_text_color(app.skin_label_text_layer, GColorWhite);
  text_layer_set_text_alignment(app.skin_label_text_layer, GTextAlignmentRight);
  text_layer_set_background_color(app.skin_label_text_layer, GColorBlack);
  layer_add_child(root_layer, text_layer_get_layer(app.skin_label_text_layer));

  app.skin_text_layer = text_layer_create(GRect(65, 28, 75, 40));
  text_layer_set_font(app.skin_text_layer, gothic_28);
  text_layer_set_text(app.skin_text_layer, EMPTY_VALUE_TEXT);
  text_layer_set_text_color(app.skin_text_layer, GColorBrightGreen);
  text_layer_set_text_alignment(app.skin_text_layer, GTextAlignmentRight);
  text_layer_set_background_color(app.skin_text_layer, GColorBlack);
  layer_add_child(root_layer, text_layer_get_layer(app.skin_text_layer));

  app.feel_like_label_text_layer = text_layer_create(GRect(0, 60, 60, 40));
  text_layer_set_font(app.feel_like_label_text_layer, gothic_24);
  text_layer_set_text(app.feel_like_label_text_layer, LABEL_FEELS_LIKE_TEXT);
  text_layer_set_text_color(app.feel_like_label_text_layer, GColorWhite);
  text_layer_set_text_alignment(app.feel_like_label_text_layer, GTextAlignmentRight);
  text_layer_set_background_color(app.feel_like_label_text_layer, GColorBlack);
  layer_add_child(root_layer, text_layer_get_layer(app.feel_like_label_text_layer));

  app.feel_like_text_layer = text_layer_create(GRect(65, 60, 75, 40));
  text_layer_set_font(app.feel_like_text_layer, gothic_28);
  text_layer_set_text(app.feel_like_text_layer, EMPTY_VALUE_TEXT);
  text_layer_set_text_color(app.feel_like_text_layer, GColorBrightGreen);
  text_layer_set_text_alignment(app.feel_like_text_layer, GTextAlignmentRight);
  text_layer_set_background_color(app.feel_like_text_layer, GColorBlack);
  layer_add_child(root_layer, text_layer_get_layer(app.feel_like_text_layer));

  app.mode_name_text_layer = text_layer_create(GRect(0, 94, 144, 40));
  text_layer_set_font(app.mode_name_text_layer, gothic_24);
  text_layer_set_text(app.mode_name_text_layer, current_compensation_mode_name());
  text_layer_set_text_color(app.mode_name_text_layer, GColorWhite);
  text_layer_set_text_alignment(app.mode_name_text_layer, GTextAlignmentCenter);
  text_layer_set_background_color(app.mode_name_text_layer, GColorBlack);
  layer_add_child(root_layer, text_layer_get_layer(app.mode_name_text_layer));

  // Sensirion Logo
  app.res_bmp_logo_black = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_LOGO_BLACK);
  app.bmp_logo_black_layer = bitmap_layer_create(GRect(7, 135, 131, 23));
  bitmap_layer_set_bitmap(app.bmp_logo_black_layer, app.res_bmp_logo_black);
  layer_add_child(root_layer, (Layer *)app.bmp_logo_black_layer);

  // Dialog Box for Disconnect Events
  dialog_create_disconnect_warning(&app.dialog);
  layer_add_child(root_layer, app.dialog.layer);
  layer_set_hidden(app.dialog.layer, bp_get_status());
}

static void on_unload_window(Window *window) {
  text_layer_destroy(app.title_layer);
  text_layer_destroy(app.skin_text_layer);
  text_layer_destroy(app.feel_like_text_layer);
  text_layer_destroy(app.skin_label_text_layer);
  text_layer_destroy(app.feel_like_label_text_layer);
  text_layer_destroy(app.mode_name_text_layer);
  gbitmap_destroy(app.res_bmp_logo_black);
  bitmap_layer_destroy(app.bmp_logo_black_layer);
  dialog_destroy(&app.dialog);
  window_destroy(app.window);
}

static void on_processed_values(float t_skin, float t_feellike,
                                float t_apparent, float t_humidex) {
  char fbuf[16];

  /* skin temperature */
  ftoa(fbuf, t_skin, 1);
  snprintf(app.skin_text_layer_buf, sizeof(app.skin_text_layer_buf),
           TEMPERATURE_FORMAT, fbuf);
  text_layer_set_text(app.skin_text_layer, app.skin_text_layer_buf);

  /* feellike temperature */
  ftoa(fbuf, t_feellike, 1);
  snprintf(app.feel_like_text_layer_buf, sizeof(app.feel_like_text_layer_buf),
           TEMPERATURE_FORMAT, fbuf);
  text_layer_set_text(app.feel_like_text_layer, app.feel_like_text_layer_buf);
}

void on_compensation_mode_changed(uint8_t mode, uint8_t num_modes) {
  app.current_compensation_mode = mode;
  app.number_of_compensation_modes = num_modes;
  if (app.window)
    text_layer_set_text(app.mode_name_text_layer, current_compensation_mode_name());
}

static void on_connection_state_changed(bool connected) {
  update_connection_text(connected);
  if (connected) {
      bp_set_temperature_compensation_mode(app.current_compensation_mode,
                                           on_compensation_mode_changed);
  }
}

static void next_compensation_mode() {
  bp_set_temperature_compensation_mode(
    (app.current_compensation_mode + 1) % app.number_of_compensation_modes,
    on_compensation_mode_changed);
}

static void on_short_click(ClickRecognizerRef recognizer, void *context) {
  next_compensation_mode();
}

static void on_long_click(ClickRecognizerRef recognizer, void *context) {
  bp_set_temperature_compensation_mode(DEFAULT_COMPENSATION_MODE,
                                       on_compensation_mode_changed);
}

static void click_config_provider(Window *window) {
  sensismart_setup_controls(&AppThermalValues);
  window_single_click_subscribe(BUTTON_ID_SELECT, on_short_click);
  window_long_click_subscribe(BUTTON_ID_SELECT, 0, on_long_click, NULL);
}

static void activate() {
  AppThermalValues.window = window_create();
  app.window = AppThermalValues.window;
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
  app.window = NULL;
  bp_unsubscribe();
}

static void load() {
  app.current_compensation_mode = DEFAULT_COMPENSATION_MODE;
  if (bp_get_status()) {
    bp_set_temperature_compensation_mode(DEFAULT_COMPENSATION_MODE,
                                         on_compensation_mode_changed);
  }
}

SensiSmartApp AppThermalValues = {
  .name = "ThermalValues",
  .window = NULL,
  .activate = activate,
  .deactivate = deactivate,
  .load = load,
};

