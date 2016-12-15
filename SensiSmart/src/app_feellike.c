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

#include <math.h> /* roundf, INFINITY, NAN */
#include <pebble.h>
#include <time.h>
#include "backpack.h"
#include "SensiSmartApp.h"
#include "utils.h"
#include "app_feellike.h"

/* Thresholds below which the feel like value applies */
static const float COLD_THRESHOLD = -5.0f;
static const float COOL_THRESHOLD = -2.5f;
/** The relative baseline value - keep this at 0 */
static const float BASE_THRESHOLD = 0.0f;
/** The default absolute temperature of the baseline value */
static const float BASE_TEMPERATURE = 21.0f;
static const float GOOD_THRESHOLD = +2.5f;
static const float WARM_THRESHOLD = +5.0f;
static const float HOT_THRESHOLD = INFINITY;

static struct {
  Window *window;
  TextLayer *time_layer;
  TextLayer *comfort_level_layer;
  TextLayer *fl_text_layer;
  TextLayer *fl_temperature_layer;
  TextLayer *logo_text_layer;
  /** heat index baseline */
  float hi_base;
  /** heat index baseline is currently set manually */
  bool hi_base_is_set;
  float last_t_feellike;
  char time_buf[9];
  char fl_temperature_buf[10];
} app;

static const char *fl_comfort_level(float heat_index) {
  float diff = heat_index - app.hi_base;
  if (diff < COLD_THRESHOLD)
    return "cold";
  if (diff < COOL_THRESHOLD)
    return "cool";
  if (diff < GOOD_THRESHOLD)
    return "good";
  if (diff < WARM_THRESHOLD)
    return "warm";
  return "hot";
}

static GColor8 fl_color(float heat_index) {
  /* https://developer.pebble.com/more/color-picker/ */
  float diff = heat_index - app.hi_base;
  if (diff < COOL_THRESHOLD)
    return GColorBlue;
  if (diff < GOOD_THRESHOLD)
    return GColorIslamicGreen;
  if (diff < WARM_THRESHOLD)
    return GColorChromeYellow;
  return GColorRed;
}

static void update_clock() {
  clock_copy_time_string(app.time_buf, sizeof(app.time_buf));
  text_layer_set_text(app.time_layer, app.time_buf);
}

static void on_connection_state_changed(bool connected) {
  update_clock();
  if (!connected) {
    snprintf(app.fl_temperature_buf, sizeof(app.fl_temperature_buf), "-- °C");
    text_layer_set_text(app.fl_temperature_layer, app.fl_temperature_buf);
  }
}

static void on_processed_values(float t_skin, float t_feellike,
                                float t_apparent, float t_humidex) {
  char fbuf[16];
  app.last_t_feellike = t_feellike;
  update_clock();

  // Comfort level
  text_layer_set_text_color(app.comfort_level_layer, fl_color(t_feellike));
  text_layer_set_text(app.comfort_level_layer, fl_comfort_level(t_feellike));

  // Feels like
  ftoa(fbuf, t_feellike, 1);
  snprintf(app.fl_temperature_buf, sizeof(app.fl_temperature_buf),
           "%s °C", fbuf);
  text_layer_set_text(app.fl_temperature_layer, app.fl_temperature_buf);
}

static void on_load_window(Window *window) {
  sensismart_window_load(&AppFeellike);
  Layer *root_layer = window_get_root_layer(app.window);
  GFont clock_font = fonts_get_system_font(FONT_KEY_BITHAM_30_BLACK);
  GFont text_font = fonts_get_system_font(FONT_KEY_GOTHIC_24);
  GFont comfort_level_font = fonts_get_system_font(FONT_KEY_BITHAM_30_BLACK);

  // app.time_layer
  app.time_layer = text_layer_create(GRect(0, 5, 144, 38));
  text_layer_set_background_color(app.time_layer, GColorClear);
  text_layer_set_text_color(app.time_layer, GColorWhite);
  text_layer_set_text_alignment(app.time_layer, GTextAlignmentCenter);
  text_layer_set_font(app.time_layer, clock_font);
  update_clock();
  layer_add_child(root_layer, (Layer *)app.time_layer);

  // app.comfort_level_layer
  app.comfort_level_layer = text_layer_create(GRect(0, 50, 144, 35));
  text_layer_set_background_color(app.comfort_level_layer, GColorClear);
  text_layer_set_text_color(app.comfort_level_layer, fl_color(app.last_t_feellike));
  text_layer_set_text(app.comfort_level_layer, fl_comfort_level(app.last_t_feellike));
  text_layer_set_text_alignment(app.comfort_level_layer, GTextAlignmentCenter);
  text_layer_set_font(app.comfort_level_layer, comfort_level_font);
  layer_add_child(root_layer, (Layer *)app.comfort_level_layer);

  // app.fl_text_layer
  app.fl_text_layer = text_layer_create(GRect(0, 95, 72, 24));
  text_layer_set_background_color(app.fl_text_layer, GColorClear);
  text_layer_set_text_color(app.fl_text_layer, GColorWhite);
  text_layer_set_text(app.fl_text_layer, "feels like");
  text_layer_set_font(app.fl_text_layer, text_font);
  text_layer_set_text_alignment(app.fl_text_layer, GTextAlignmentCenter);
  layer_add_child(root_layer, (Layer *)app.fl_text_layer);

  // app.fl_temperature_layer
  app.fl_temperature_layer = text_layer_create(GRect(72, 95, 72, 24));
  text_layer_set_background_color(app.fl_temperature_layer, GColorClear);
  text_layer_set_text_color(app.fl_temperature_layer, GColorWhite);
  text_layer_set_text(app.fl_temperature_layer, "-- °C");
  text_layer_set_font(app.fl_temperature_layer, text_font);
  text_layer_set_text_alignment(app.fl_temperature_layer, GTextAlignmentCenter);
  layer_add_child(root_layer, (Layer *)app.fl_temperature_layer);

  // Sensirion logo
  layer_add_child(root_layer, sensismart_get_branding_layer());
}

static void on_unload_window(Window *window) {
  text_layer_destroy(app.time_layer);
  text_layer_destroy(app.comfort_level_layer);
  text_layer_destroy(app.fl_text_layer);
  text_layer_destroy(app.fl_temperature_layer);
  text_layer_destroy(app.logo_text_layer);
  window_destroy(app.window);
}

static void on_click_select(ClickRecognizerRef recognizer, void *context) {
  if (app.hi_base_is_set) {
    app.hi_base = BASE_TEMPERATURE;
    text_layer_set_text_color(app.fl_text_layer, GColorWhite);
  } else {
    app.hi_base = app.last_t_feellike;
    text_layer_set_text_color(app.fl_text_layer, GColorOrange);
  }
  app.hi_base_is_set = !app.hi_base_is_set;
  on_processed_values(NAN, app.last_t_feellike, NAN, NAN);
}

static void click_config_provider(Window *window) {
  sensismart_setup_controls(&AppFeellike);
  window_single_click_subscribe(BUTTON_ID_SELECT, on_click_select);
}

static void activate() {
  AppFeellike.window = window_create();
  app.window = AppFeellike.window;
  app.hi_base_is_set = false;
  app.hi_base = BASE_TEMPERATURE;
  app.last_t_feellike = BASE_TEMPERATURE;
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

SensiSmartApp AppFeellike = {
  .name = "Feellike",
  .window = NULL,
  .activate = activate,
  .deactivate = deactivate
};

