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
#include "app_thermal_context.h"

static const int METER_HEIGHT = 168;
static const int METER_WIDTH = 144;

static const char *THERMAL_CONTEXT_TITLE = "Thermal Context";

static const char *CONTEXT_TITLES[] = {"Activity",
                                       "Comfort",
                                       "Leisure"};

static const uint32_t CONTEXT_RESOURCES[] = {RESOURCE_ID_IMAGE_CONTEXT_ACTIVITY,
                                             RESOURCE_ID_IMAGE_CONTEXT_COMFORT,
                                             RESOURCE_ID_IMAGE_CONTEXT_LEISURE};

static const int NUM_CONTEXTS = sizeof(CONTEXT_TITLES)/sizeof(CONTEXT_TITLES[0]);

static struct {
  int min;
  int max;
} CONTEXT_RANGES[] = {{0, 40}, {14, 34}, {10, 35}};

static struct {
  Window *window;
  TextLayer *title_layer;
  GBitmap *res_bmp_context;
  BitmapLayer *bmp_context_layer;
  Layer *meter_layer;
  GPath *indicator_path;
  float current_humidex;
  int current_context_idx;
  TextLayer *context_type_layer;
  Dialog dialog;
} app;

static void init_bp_subscriptions();

static const GPathInfo INDICATOR_PATH_INFO = {
  .num_points = 5,
  .points = (GPoint []) {{0, -6}, {-30, 0}, {0, 5}, {6, 0}, {0, -6}}
};

static void update_connection_text(bool connected) {
  layer_set_hidden(app.dialog.layer, connected);
}

static void on_connection_state_changed(bool connected) {
  update_connection_text(connected);
  if (connected) {
    bp_unsubscribe();
    init_bp_subscriptions();
  }
}

static int32_t angle_for_temperature(float temp) {
  if (temp <= CONTEXT_RANGES[app.current_context_idx].min) {
    return -10 * TRIG_MAX_ANGLE / 360;
  } else if (temp >= CONTEXT_RANGES[app.current_context_idx].max) {
    return 190 * TRIG_MAX_ANGLE / 360;
  } else {
    return (int32_t) -10 * TRIG_MAX_ANGLE / 360
           + (200.0f * TRIG_MAX_ANGLE * (temp - CONTEXT_RANGES[app.current_context_idx].min)
           / (360 * (CONTEXT_RANGES[app.current_context_idx].max - CONTEXT_RANGES[app.current_context_idx].min)));
  }
}

static void on_indicator_update_proc(Layer *layer, GContext *ctx) {
  int32_t angle = angle_for_temperature(app.current_humidex);
  gpath_rotate_to(app.indicator_path, angle);
  gpath_move_to(app.indicator_path, GPoint(72, 105));

  graphics_context_set_fill_color(ctx, GColorWhite);
  gpath_draw_filled(ctx, app.indicator_path);
}

static void init_indicator() {
  app.indicator_path = gpath_create(&INDICATOR_PATH_INFO);
  layer_set_update_proc(app.meter_layer, on_indicator_update_proc);
}

static void on_load_window(Window *window) {
  sensismart_window_load(&AppThermalContext);
  Layer *root_layer = window_get_root_layer(window);

  // Screen Title
  app.title_layer = text_layer_create(GRect(0, 0, 144, 20));
  text_layer_set_font(app.title_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18));
  text_layer_set_text(app.title_layer, THERMAL_CONTEXT_TITLE);
  text_layer_set_text_color(app.title_layer, GColorWhite);
  text_layer_set_background_color(app.title_layer, GColorBlack);
  text_layer_set_text_alignment(app.title_layer, GTextAlignmentCenter);
  layer_add_child(root_layer, text_layer_get_layer(app.title_layer));

  // Sensirion Logo
  layer_add_child(root_layer, sensismart_get_branding_layer());

  // Context Scale
  app.res_bmp_context = gbitmap_create_with_resource(CONTEXT_RESOURCES[app.current_context_idx]);
  app.bmp_context_layer = bitmap_layer_create(GRect(5, 27, 134, 102));
  bitmap_layer_set_bitmap(app.bmp_context_layer, app.res_bmp_context);
  layer_add_child(root_layer, (Layer *)app.bmp_context_layer);

  // Indicator layer
  app.meter_layer = layer_create(GRect(0, 0, METER_WIDTH, METER_HEIGHT));
  layer_add_child(root_layer, app.meter_layer);
  init_indicator();

  // Context Type Description
  app.context_type_layer = text_layer_create(GRect(0, 113, 144, 20));
  text_layer_set_font(app.context_type_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14));
  text_layer_set_text(app.context_type_layer, CONTEXT_TITLES[app.current_context_idx]);
  text_layer_set_text_color(app.context_type_layer, GColorWhite);
  text_layer_set_background_color(app.context_type_layer, GColorClear);
  text_layer_set_text_alignment(app.context_type_layer, GTextAlignmentCenter);
  layer_add_child(root_layer, text_layer_get_layer(app.context_type_layer));

  // Dialog Box for Disconnect Events
  dialog_create_disconnect_warning(&app.dialog);
  layer_add_child(root_layer, app.dialog.layer);
  layer_set_hidden(app.dialog.layer, bp_get_status());
}

static void on_unload_window(Window *window) {
  text_layer_destroy(app.title_layer);
  gpath_destroy(app.indicator_path);
  layer_destroy(app.meter_layer);

  gbitmap_destroy(app.res_bmp_context);
  bitmap_layer_destroy(app.bmp_context_layer);
  text_layer_destroy(app.context_type_layer);

  dialog_destroy(&app.dialog);

  window_destroy(app.window);
}

static void on_processed_values(float t_skin, float t_feellike,
                                float t_apparent, float t_humidex) {
  app.current_humidex = t_humidex;
  layer_mark_dirty(app.meter_layer);
}

static void toggle_context() {
  app.current_context_idx = (app.current_context_idx + 1) % NUM_CONTEXTS;

  gbitmap_destroy(app.res_bmp_context);

  app.res_bmp_context = gbitmap_create_with_resource(CONTEXT_RESOURCES[app.current_context_idx]);
  bitmap_layer_set_bitmap(app.bmp_context_layer, app.res_bmp_context);
  text_layer_set_text(app.context_type_layer, CONTEXT_TITLES[app.current_context_idx]);
}

static void on_click_select(ClickRecognizerRef recognizer, void *context) {
  toggle_context();
}

static void on_long_click_select(ClickRecognizerRef recognizer, void *context) {
  CONTEXT_RANGES[app.current_context_idx].min = app.current_humidex - 20;
  CONTEXT_RANGES[app.current_context_idx].max = app.current_humidex + 20;
  layer_mark_dirty(app.meter_layer);
}

static void click_config_provider(Window *window) {
  sensismart_setup_controls(&AppThermalContext);
  window_single_click_subscribe(BUTTON_ID_SELECT, on_click_select);
  window_long_click_subscribe(BUTTON_ID_SELECT, 2000, on_long_click_select, NULL);
}

static void init_bp_subscriptions() {
  bp_subscribe((BackpackHandlers) {
    .on_connection_state_changed = on_connection_state_changed,
    .on_processed_values = on_processed_values
  });
}

static void activate() {
  AppThermalContext.window = window_create();
  app.window = AppThermalContext.window;
  window_set_window_handlers(app.window, (WindowHandlers) {
    .load = on_load_window,
    .unload = on_unload_window
  });
  window_set_click_config_provider(app.window, (ClickConfigProvider) click_config_provider);
  init_bp_subscriptions();
  window_stack_push(app.window, true);
}

static void deactivate() {
  window_stack_pop(true);
  bp_unsubscribe();
}

SensiSmartApp AppThermalContext = {
  .name = "ThermalContext",
  .window = NULL,
  .activate = activate,
  .deactivate = deactivate
};

