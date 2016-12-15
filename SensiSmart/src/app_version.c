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
#include "app_version.h"

enum display_mode {
  DISPLAY_MODE_VALUES,
  DISPLAY_MODE_LEGEND_SENSOR_READINGS,
  DISPLAY_MODE_LEGEND_PROCESSED_VALUES,
  NUM_DISPLAY_MODES
};

static struct {
  Window *window;
  TextLayer *bp_lib_version_text_layer;
  TextLayer *fw_version_text_layer;
  TextLayer *cap_text_layer;
  char capabilities_buf[160];
  enum display_mode display_mode;
  Dialog dialog;
} app;

static void update_capabilities() {
  char env_cap[3];
  char skin_cap[3];
  char press_cap[3];
  char pv_cap[8];
  uint16_t s_caps = bp_get_available_sensor_readings_mask();
  uint16_t p_caps = bp_get_available_processed_values_mask();
  uint32_t log_caps = bp_get_logged_values_mask();

  env_cap[0] = s_caps & ATTR_SENSOR_READINGS_TEMPERATURE ? 'T' : 't';
  env_cap[1] = s_caps & ATTR_SENSOR_READINGS_HUMIDITY ? 'H' : 'h';
  env_cap[2] = '\0';

  skin_cap[0] = s_caps & ATTR_SENSOR_READINGS_SKIN_TEMPERATURE ? 'T' : 't';
  skin_cap[1] = s_caps & ATTR_SENSOR_READINGS_SKIN_HUMIDITY ? 'H' : 'h';
  skin_cap[2] = '\0';

  press_cap[0] = '_';
  press_cap[1] = '_';
  press_cap[2] = '\0';

  pv_cap[0] = p_caps & ATTR_PROCESSED_VALUES_SKIN_TEMPERATURE ? 'S' : 's';
  pv_cap[1] = p_caps & ATTR_PROCESSED_VALUES_APPARENT_TEMPERATURE ? 'A' : 'a';
  pv_cap[2] = p_caps & ATTR_PROCESSED_VALUES_FEELLIKE_TEMPERATURE ? 'F' : 'f';
  pv_cap[3] = p_caps & ATTR_PROCESSED_VALUES_HUMIDEX ? 'X' : 'x';
  pv_cap[4] = '_';
  pv_cap[5] = p_caps & ATTR_PROCESSED_VALUES_AIRTOUCH_START_EVENT ? 'R' : 'r';
  pv_cap[6] = p_caps & ATTR_PROCESSED_VALUES_ONBODY_STATE ? 'B' : 'b';
  pv_cap[7] = '\0';

  snprintf(app.capabilities_buf, sizeof(app.capabilities_buf),
           "Version: %s\n"
           "Sensors: 0x%04x %04x\n"
           "Log:       0x%04x %04x\n"
           "Env:  %s\n"
           "Skin: %s\n"
           "Reserved: %s\n"
           "Proc.Val:  %s",
           bp_get_version(), p_caps, s_caps,
           (uint16_t)(log_caps >> 16), (uint16_t)log_caps,
           env_cap, skin_cap, press_cap, pv_cap);
  text_layer_set_text(app.cap_text_layer, app.capabilities_buf);
}

static void update_display() {
  switch (app.display_mode) {
    case DISPLAY_MODE_LEGEND_SENSOR_READINGS:
      snprintf(app.capabilities_buf, sizeof(app.capabilities_buf),
               "Missing features are\n"
               "  lower cased\n"
               "Symbol legend:\n"
               "T: Temperature in °C\n"
               "H: Relative Humidity in %%\n"
               "...: Reservred ");
      text_layer_set_text(app.cap_text_layer, app.capabilities_buf);
      break;
    case DISPLAY_MODE_LEGEND_PROCESSED_VALUES:
      snprintf(app.capabilities_buf, sizeof(app.capabilities_buf),
               "S: Skin temperature\n"
               "A: Apparent temperature\n"
               "F: Feellike temperature\n"
               "X: Humidex\n"
               "...:Reserved\n"
               "R: AirTouch\n"
               "B: Onbody Detection\n");
      text_layer_set_text(app.cap_text_layer, app.capabilities_buf);
      break;
    case DISPLAY_MODE_VALUES:
    default:
      update_capabilities();
      break;
  }
}

static void on_connection_state_changed(bool connected) {
  layer_set_hidden((Layer *)app.dialog.layer, connected);
  update_capabilities();
}

static void on_load_window(Window *window) {
  sensismart_window_load(&AppVersion);
  Layer *root_layer = window_get_root_layer(window);
  GFont font = fonts_get_system_font(FONT_KEY_GOTHIC_14);
  bool connected = bp_get_status();

  app.bp_lib_version_text_layer = text_layer_create(GRect(0, 5, 144, 16));
  text_layer_set_font(app.bp_lib_version_text_layer, font);
  text_layer_set_text(app.bp_lib_version_text_layer, "App lib " BACKPACK_LIB_VERSION);
  text_layer_set_text_color(app.bp_lib_version_text_layer, GColorWhite);
  text_layer_set_background_color(app.bp_lib_version_text_layer, GColorBlack);
  text_layer_set_text_alignment(app.bp_lib_version_text_layer, GTextAlignmentCenter);
  text_layer_set_overflow_mode(app.bp_lib_version_text_layer, GTextOverflowModeWordWrap);
  layer_add_child(root_layer, text_layer_get_layer(app.bp_lib_version_text_layer));

  app.cap_text_layer = text_layer_create(GRect(0, 28, 144, 100));
  text_layer_set_font(app.cap_text_layer, font);
  text_layer_set_text_color(app.cap_text_layer, GColorBrightGreen);
  text_layer_set_background_color(app.cap_text_layer, GColorBlack);
  text_layer_set_overflow_mode(app.cap_text_layer, GTextOverflowModeWordWrap);
  update_capabilities();
  layer_add_child(root_layer, text_layer_get_layer(app.cap_text_layer));

  layer_add_child(root_layer, sensismart_get_branding_layer());

  dialog_create_disconnect_warning(&app.dialog);
  layer_add_child(root_layer, app.dialog.layer);
  layer_set_hidden(app.dialog.layer, connected);
}

static void on_unload_window(Window *window) {
  text_layer_destroy(app.bp_lib_version_text_layer);
  text_layer_destroy(app.fw_version_text_layer);
  text_layer_destroy(app.cap_text_layer);
  dialog_destroy(&app.dialog);
  window_destroy(app.window);
}

static void on_click_select(ClickRecognizerRef recognizer, void *context) {
  app.display_mode = (app.display_mode + 1) % NUM_DISPLAY_MODES;
  update_display();
}

static void click_config_provider(Window *window) {
  sensismart_setup_controls(&AppVersion);
  window_single_click_subscribe(BUTTON_ID_SELECT, on_click_select);
}

static void activate() {
  app.display_mode = DISPLAY_MODE_VALUES;
  AppVersion.window = window_create();
  app.window = AppVersion.window;
  window_set_window_handlers(app.window, (WindowHandlers) {
    .load = on_load_window,
    .unload = on_unload_window
  });
  window_set_click_config_provider(app.window, (ClickConfigProvider) click_config_provider);
  bp_subscribe((BackpackHandlers) {
    .on_connection_state_changed = on_connection_state_changed
  });
  window_stack_push(app.window, true);
}

static void deactivate() {
  window_stack_pop(true);
  bp_unsubscribe();
}

SensiSmartApp AppVersion = {
  .name = "Version",
  .window = NULL,
  .activate = activate,
  .deactivate = deactivate
};

