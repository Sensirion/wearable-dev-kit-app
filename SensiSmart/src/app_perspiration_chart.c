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

#include <math.h> /* isnan, roundf, NAN */
#include <pebble.h>
#include <time.h>
#include "backpack.h"
#include "SensiSmartApp.h"
#include "utils.h"
#include "app_perspiration_chart.h"

#define CHART_LEN 40
#define NUMBER_BUF_LEN 20
#define LONG_PRESS_INTERVAL 1000
#define CHART_MARGIN 0.01

static const int TOAST_TIMEOUT_MS = 2000;
static const int POLLING_INTERVAL_MS = 2000;
static const int CHART_H = 125;
static const int CHART_W = 144;
enum chart_range {
  RANGE_NARROW,
  RANGE_NORMAL,
  RANGE_BROAD,
  RANGE_NUM_INDICES
};
static const float CHART_RANGE[] = {50.0f, 150.0f, 20.0f};
static const char *CHART_RANGE_DESC[] = {
  "normal",
  "broad",
  "narrow"
};
static const char *CONFIG_CHANGE_TEXT = "Changing chart scale to\n%s";
static const GColor ONBODY_COLOR[] = { {GColorRedARGB8}, {GColorIslamicGreenARGB8} };

static struct {
  Window *window;
  AppTimer *toast_show_timer;
  Layer *axes_layer;
  Layer *chart_layer;
  TextLayer *current_value_layer;
  TextLayer *toast_text_layer;
  GPath *chart_path;
  GPoint chart_path_points[CHART_LEN + 2];
  Dialog dialog;
  char current_value_buf[NUMBER_BUF_LEN];
  char toast_text_layer_buf[60];
  int chart_idx;
  int chart_size;
  float chart[CHART_LEN + 2];
  enum chart_range range_idx;
  struct BackpackAttribute at_transpiration;
  bool ui_initialized;
} app;

static void on_axes_update_proc(Layer *layer, GContext *ctx) {
  GRect rect = layer_get_bounds(app.axes_layer);
  graphics_context_set_stroke_color(ctx, GColorWhite);
  graphics_draw_rect(ctx, rect);
}

static int16_t scale_idx_value(int idx) {
  return (idx * CHART_W) / CHART_LEN;
}

static int16_t scale_p_value(float p) {
  return CHART_H - (CHART_H * (p / CHART_RANGE[app.range_idx] + CHART_MARGIN));
}

static void on_chart_update_proc(Layer *layer, GContext *ctx) {
  graphics_context_set_stroke_width(ctx, 1);
  graphics_context_set_stroke_color(ctx, GColorGreen);
  graphics_context_set_fill_color(ctx, GColorGreen);

  int len = app.chart_size;
  if (len == 0)
    return;
  app.chart_path->num_points = len + 2;
  app.chart_path_points[0] = (GPoint) {
    .x = 0,
    .y = CHART_H
  };
  int i;
  for (i = 0; i < len; ++i) {
    int idx_i = (app.chart_idx + CHART_LEN - (len - 1) + i) % CHART_LEN;
    app.chart_path_points[i+1] = (GPoint) {
      .x = scale_idx_value(i),
      .y = scale_p_value(app.chart[idx_i])
    };
  }
  app.chart_path_points[len + 1] = (GPoint) {
    .x = scale_idx_value(len-1),
    .y = CHART_H
  };
  gpath_draw_outline(ctx, app.chart_path);
  gpath_draw_filled(ctx, app.chart_path);
}

static void init_chart() {
  app.chart_idx = -1;
  app.chart_size = 0;
  GPathInfo path_info = { .num_points = 0, .points = app.chart_path_points };
  app.chart_path = gpath_create(&path_info);
  layer_set_update_proc(app.axes_layer, (LayerUpdateProc)on_axes_update_proc);
  layer_set_update_proc(app.chart_layer, (LayerUpdateProc)on_chart_update_proc);
}

static void update_current_value_text(float p) {
  if (p < 0.0f) {
    /* firmware prohibits values below zero, thus raise error */
    text_layer_set_text(app.current_value_layer, "ERROR: Reading out data");
  } else {
    char fbuf[NUMBER_BUF_LEN];
    ftoa(fbuf, p, 2);
    snprintf(app.current_value_buf, sizeof(app.current_value_buf),
             "%s g/h*m²", fbuf);
    text_layer_set_text(app.current_value_layer, app.current_value_buf);
  }
}

static void on_connection_state_changed(bool connected) {
  layer_set_hidden(app.dialog.layer, connected);
  if (!connected)
    update_current_value_text(0.0f);
}

static void on_subscribed_processed_values(const uint8_t *data, size_t length,
                                           SmartstrapAttributeId flags) {
  if (flags != ATTR_PROCESSED_VALUES_TRANSPIRATION) {
    WARN("Unexpected processed values - ignoring");
    return;
  }
  int offset = 0;
  float p;
  bp_readval(data, length, &offset, &p,
             ATTR_PROCESSED_VALUES_TRANSPIRATION_LEN, "transpiration");
  if (app.chart_size < CHART_LEN)
    app.chart_size += 1;
  app.chart_idx = (app.chart_idx + 1) % CHART_LEN;
  app.chart[app.chart_idx] = p;
  if (app.ui_initialized) {
    layer_mark_dirty(app.chart_layer);
    update_current_value_text(p);
  }
}

static void hide_toast() {
  layer_set_hidden((Layer *)app.toast_text_layer, true);
}

static void show_toast(const char *message) {
  text_layer_set_text(app.toast_text_layer, message);
  layer_set_hidden((Layer *)app.toast_text_layer, false);

  if (!app.toast_show_timer ||
      !app_timer_reschedule(app.toast_show_timer, TOAST_TIMEOUT_MS)) {
    app.toast_show_timer = app_timer_register(TOAST_TIMEOUT_MS,
                                              hide_toast, NULL);
  }
}

static void on_load_window(Window *window) {
  sensismart_window_load(&AppPerspirationChart);
  Layer *root_layer = window_get_root_layer(window);

  // Perspiration Plot
  app.axes_layer = layer_create(GRect(0, 0, CHART_W, CHART_H));
  app.chart_layer = layer_create(GRect(0, 0, CHART_W, CHART_H));
  layer_add_child(app.axes_layer, app.chart_layer);
  layer_add_child(root_layer, app.axes_layer);

  init_chart();
  app.ui_initialized = true;

  // app.current_value_layer
  app.current_value_layer = text_layer_create(GRect(0, 0, 144, 20));
  text_layer_set_font(app.current_value_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18));
  text_layer_set_text_color(app.current_value_layer, GColorWhite);
  text_layer_set_background_color(app.current_value_layer, GColorClear);
  text_layer_set_text_alignment(app.current_value_layer, GTextAlignmentCenter);
  update_current_value_text(0);
  layer_add_child(root_layer, text_layer_get_layer(app.current_value_layer));

  // Sensirion Logo
  layer_add_child(root_layer, sensismart_get_branding_layer());

  // app.toast_text_layer
  app.toast_text_layer = text_layer_create(GRect(14, 52, 117, 48));
  text_layer_set_text(app.toast_text_layer, CONFIG_CHANGE_TEXT);
  text_layer_set_font(app.toast_text_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14));
  text_layer_set_text_alignment(app.toast_text_layer, GTextAlignmentCenter);
  layer_add_child(root_layer, (Layer *)app.toast_text_layer);
  layer_set_hidden((Layer *)app.toast_text_layer, true);

  // Dialog Box for Disconnect Events
  dialog_create_disconnect_warning(&app.dialog);
  layer_add_child(root_layer, app.dialog.layer);
  layer_set_hidden(app.dialog.layer, bp_get_status());
}

static void on_unload_window(Window *window) {
  app.ui_initialized = false;
  gpath_destroy(app.chart_path);
  layer_destroy(app.chart_layer);
  layer_destroy(app.axes_layer);
  text_layer_destroy(app.current_value_layer);
  text_layer_destroy(app.toast_text_layer);
  dialog_destroy(&app.dialog);
  window_destroy(app.window);
}

static void on_short_click_select(ClickRecognizerRef recognizer, void *context) {
  app.range_idx = (app.range_idx + 1) % RANGE_NUM_INDICES;
  layer_mark_dirty(app.chart_layer);

  snprintf(app.toast_text_layer_buf, sizeof(app.toast_text_layer_buf),
           CONFIG_CHANGE_TEXT, CHART_RANGE_DESC[app.range_idx]);
  show_toast(app.toast_text_layer_buf);
}

static void on_long_click_select(ClickRecognizerRef recognizer, void *context) {
  DBG("Resetting chart");
  app.chart_idx = -1;
  app.chart_size = 0;
  layer_mark_dirty(app.chart_layer);
}

static void click_config_provider(Window *window) {
  sensismart_setup_controls(&AppPerspirationChart);
  window_single_click_subscribe(BUTTON_ID_SELECT, on_short_click_select);
  window_long_click_subscribe(BUTTON_ID_SELECT, 1000, on_long_click_select, NULL);
}

static void activate() {
  AppPerspirationChart.window = window_create();
  app.window = AppPerspirationChart.window;
  window_set_window_handlers(app.window, (WindowHandlers) {
    .load = on_load_window,
    .unload = on_unload_window
  });
  window_set_click_config_provider(app.window, (ClickConfigProvider) click_config_provider);
  window_stack_push(app.window, true);

  bp_set_polling_interval(POLLING_INTERVAL_MS);
  bp_subscribe_attribute(&app.at_transpiration);
  bp_subscribe((BackpackHandlers) {
    .on_connection_state_changed = on_connection_state_changed
  });
}

static void deactivate() {
  if (app.toast_show_timer) {
    app_timer_cancel(app.toast_show_timer);
    app.toast_show_timer = NULL;
  }
  window_stack_pop(true);
  bp_unsubscribe();
}

static void load() {
  bp_init_attribute(&app.at_transpiration,
      SERVICE_PROCESSED_VALUES,
      ATTR_PROCESSED_VALUES_TRANSPIRATION,
      ATTR_PROCESSED_VALUES_TRANSPIRATION_LEN,
      "Transpiration",
      on_subscribed_processed_values);
}

static void cleanup_attribute(void *ctx) {
  if (!bp_destroy_attribute(&app.at_transpiration)) {
    INFO("Waiting to clean attribute...");
    app_timer_register(DESTROY_RETRY_INTERVAL_MS, cleanup_attribute, ctx);
  }
}

static void unload() {
  cleanup_attribute(NULL);
}

SensiSmartApp AppPerspirationChart = {
  .name = "PerspirationChart",
  .window = NULL,
  .activate = activate,
  .deactivate = deactivate,
  .load = load,
  .unload = unload
};

