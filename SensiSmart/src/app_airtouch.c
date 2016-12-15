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
#include "app_airtouch.h"

static const char *AIRTOUCH_TEXT = "AirTouch ®";
static const char *CONFIG_CHANGE_TEXT = "Changing event interval to level:\n%s";
static const char *PRESENTS_TEXT = "presents";
static const char *DISMISS_TEXT = "to dismiss\nnotifications";

static const int TOAST_TIMEOUT_MS = 2000;

enum event_time_range {
  RANGE_SECONDS,
  RANGE_FEW_MINUTES,
  RANGE_LONG,
  RANGE_NUM_INDICES
};
static const int EVENT_TIME_MIN_DELAY[] = {3000, 60000, 600000};
static const int EVENT_TIME_MAX_RANDOM[] = {19000, 240000, 1200000};
static const char *EVENT_TIME_RANGE_NAME[] = {"SECONDS", "MINUTES", "LONG"};

static struct {
  Window *window;
  TextLayer *airtouch_text_layer;
  TextLayer *time_layer;
  char time_buf[9];
  AppTimer *notification_event_timer;
  bool notification_active;
  int notification_range_idx;
  TextLayer *toast_text_layer;
  AppTimer *toast_show_timer;
  char toast_text_layer_buf[60];
  GBitmap *res_bmp_logo_black;
  GBitmap *res_bmp_logo_white;
  BitmapLayer *bmp_logo_black_layer;
  BitmapLayer *bmp_logo_white_layer;
  TextLayer *presents_text_layer;
  TextLayer *airtouch_big_text_layer;
  TextLayer *dismiss_text_layer;
  BitmapLayer *top_bar_layer;
  BitmapLayer *bottom_bar_layer;
  Dialog dialog;
} app;

static void update_clock() {
  clock_copy_time_string(app.time_buf, sizeof(app.time_buf));
  text_layer_set_text(app.time_layer, app.time_buf);
}

static void handle_minute_tick(struct tm *tick_time, TimeUnits units_changed) {
  update_clock();
}

static void update_display() {
  GColor bg_color = app.notification_active ? GColorWhite : GColorBlack;
  window_set_background_color(app.window, bg_color);

  // Main Screen
  layer_set_hidden(bitmap_layer_get_layer(app.bmp_logo_black_layer), app.notification_active);
  layer_set_hidden(text_layer_get_layer(app.airtouch_text_layer), app.notification_active);
  layer_set_hidden(text_layer_get_layer(app.time_layer), app.notification_active);

  // Notification
  layer_set_hidden(bitmap_layer_get_layer(app.bmp_logo_white_layer), !app.notification_active);
  layer_set_hidden(text_layer_get_layer(app.airtouch_big_text_layer), !app.notification_active);
  layer_set_hidden(text_layer_get_layer(app.presents_text_layer), !app.notification_active);
  layer_set_hidden(text_layer_get_layer(app.airtouch_big_text_layer), !app.notification_active);
  layer_set_hidden(text_layer_get_layer(app.dismiss_text_layer), !app.notification_active);
  layer_set_hidden(bitmap_layer_get_layer(app.top_bar_layer), !app.notification_active);
  layer_set_hidden(bitmap_layer_get_layer(app.bottom_bar_layer), !app.notification_active);
}

static void on_connection_state_changed(bool connected) {
  layer_set_hidden(app.dialog.layer, connected);
}

static void hide_toast() {
  layer_set_hidden((Layer *)app.toast_text_layer, true);

  update_display();
}

static void show_notification(void *ctx) {
  hide_toast();

  vibes_short_pulse();
  app.notification_active = true;

  update_display();
}

static void schedule_notification_timer() {
  int event_time_ms = EVENT_TIME_MIN_DELAY[app.notification_range_idx] +
      (rand() % EVENT_TIME_MAX_RANDOM[app.notification_range_idx]);
  INFO("Scheduling next Notification Event in %d ms", event_time_ms);

  if (!app.notification_event_timer ||
      !app_timer_reschedule(app.notification_event_timer, event_time_ms)) {
    app.notification_event_timer = app_timer_register(event_time_ms,
                                                      show_notification, NULL);
  }
}

static void show_toast(const char *message) {
  text_layer_set_text(app.toast_text_layer, message);

  layer_set_hidden((Layer *)app.time_layer, true);
  layer_set_hidden((Layer *)app.airtouch_big_text_layer, true);
  layer_set_hidden((Layer *)app.presents_text_layer, true);
  layer_set_hidden((Layer *)app.toast_text_layer, false);

  if (!app.toast_show_timer ||
      !app_timer_reschedule(app.toast_show_timer, TOAST_TIMEOUT_MS)) {
    app.toast_show_timer = app_timer_register(TOAST_TIMEOUT_MS, hide_toast, NULL);
  }
}

static void toggle_event_time_range() {
  app.notification_range_idx = (app.notification_range_idx + 1) % RANGE_NUM_INDICES;

  snprintf(app.toast_text_layer_buf, sizeof(app.toast_text_layer_buf),
           CONFIG_CHANGE_TEXT, EVENT_TIME_RANGE_NAME[app.notification_range_idx]);
  show_toast(app.toast_text_layer_buf);

  schedule_notification_timer();
}

static void on_airtouch(bool start) {
  if (start && app.notification_active) {
    app.notification_active = false;

    update_display();

    schedule_notification_timer();
  }
}

static void on_load_window(Window *window) {
  sensismart_window_load(&AppAirtouch);
  Layer *root_layer = window_get_root_layer(app.window);
  GFont clock_font = fonts_get_system_font(FONT_KEY_BITHAM_30_BLACK);
  GFont text_font = fonts_get_system_font(FONT_KEY_GOTHIC_18);
  GFont toast_font = fonts_get_system_font(FONT_KEY_GOTHIC_14);
  GFont big_font = fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD);

  // app.airtouch_text_layer
  app.airtouch_text_layer = text_layer_create(GRect(0, 0, 144, 20));
  Layer *airtouch_text_layer = text_layer_get_layer(app.airtouch_text_layer);
  text_layer_set_background_color(app.airtouch_text_layer, GColorClear);
  text_layer_set_text_color(app.airtouch_text_layer, GColorWhite);
  text_layer_set_text(app.airtouch_text_layer, AIRTOUCH_TEXT);
  text_layer_set_text_alignment(app.airtouch_text_layer, GTextAlignmentCenter);
  text_layer_set_font(app.airtouch_text_layer, text_font);
  layer_add_child(root_layer, airtouch_text_layer);

  // app.time_layer
  app.time_layer = text_layer_create(GRect(0, 53, 144, 38));
  text_layer_set_background_color(app.time_layer, GColorClear);
  text_layer_set_text_color(app.time_layer, GColorWhite);
  text_layer_set_text_alignment(app.time_layer, GTextAlignmentCenter);
  text_layer_set_font(app.time_layer, clock_font);
  layer_add_child(root_layer, (Layer *)app.time_layer);

  // app.toast_text_layer
  app.toast_text_layer = text_layer_create(GRect(14, 52, 117, 48));
  text_layer_set_text(app.toast_text_layer, CONFIG_CHANGE_TEXT);
  text_layer_set_font(app.toast_text_layer, toast_font);
  text_layer_set_text_alignment(app.toast_text_layer, GTextAlignmentCenter);
  layer_add_child(root_layer, (Layer *)app.toast_text_layer);
  layer_set_hidden((Layer *)app.toast_text_layer, true);

  // logo bitmaps
  app.res_bmp_logo_black = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_LOGO_BLACK);
  app.res_bmp_logo_white = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_LOGO_WHITE);

  // bmp_logo_black_layer
  app.bmp_logo_black_layer = bitmap_layer_create(GRect(0, 135, 144, 23));
  bitmap_layer_set_bitmap(app.bmp_logo_black_layer, app.res_bmp_logo_black);
  layer_add_child(root_layer, (Layer *)app.bmp_logo_black_layer);

  // Notification Layers
  // bmp_logo_white_layer
  app.bmp_logo_white_layer = bitmap_layer_create(GRect(5, 30, 131, 23));
  bitmap_layer_set_bitmap(app.bmp_logo_white_layer, app.res_bmp_logo_white);
  layer_add_child(root_layer, (Layer *)app.bmp_logo_white_layer);
  layer_set_hidden((Layer *)app.bmp_logo_white_layer, true);

  // airtouch_big_text_layer
  app.airtouch_big_text_layer = text_layer_create(GRect(0, 73, 144, 28));
  text_layer_set_text(app.airtouch_big_text_layer, AIRTOUCH_TEXT);
  text_layer_set_text_alignment(app.airtouch_big_text_layer, GTextAlignmentCenter);
  text_layer_set_font(app.airtouch_big_text_layer, big_font);
  layer_add_child(root_layer, (Layer *)app.airtouch_big_text_layer);
  layer_set_hidden((Layer *)app.airtouch_big_text_layer, true);

  // presents_text_layer
  app.presents_text_layer = text_layer_create(GRect(0, 57, 144, 20));
  text_layer_set_text(app.presents_text_layer, PRESENTS_TEXT);
  text_layer_set_font(app.presents_text_layer, toast_font);
  text_layer_set_text_alignment(app.presents_text_layer, GTextAlignmentCenter);
  layer_add_child(root_layer, (Layer *)app.presents_text_layer);
  layer_set_hidden((Layer *)app.presents_text_layer, true);

  // dismiss_text_layer
  app.dismiss_text_layer = text_layer_create(GRect(0, 100, 144, 42));
  text_layer_set_text(app.dismiss_text_layer, DISMISS_TEXT);
  text_layer_set_font(app.dismiss_text_layer, text_font);
  text_layer_set_text_alignment(app.dismiss_text_layer, GTextAlignmentCenter);
  layer_add_child(root_layer, (Layer *)app.dismiss_text_layer);
  layer_set_hidden((Layer *)app.dismiss_text_layer, true);

  // notificatoin bars
  app.top_bar_layer = bitmap_layer_create(GRect(0, 0, 144, 20));
  bitmap_layer_set_background_color(app.top_bar_layer, GColorBlack);
  layer_add_child(root_layer, (Layer *)app.top_bar_layer);
  layer_set_hidden((Layer *)app.top_bar_layer, true);

  app.bottom_bar_layer = bitmap_layer_create(GRect(0, 148, 144, 20));
  bitmap_layer_set_background_color(app.bottom_bar_layer, GColorBlack);
  layer_add_child(root_layer, (Layer *)app.bottom_bar_layer);
  layer_set_hidden((Layer *)app.bottom_bar_layer, true);

  // Show notification on start
  app.notification_active = true;

  // Dialog Box for Disconnect Events
  dialog_create_disconnect_warning(&app.dialog);
  layer_add_child(root_layer, app.dialog.layer);
  layer_set_hidden(app.dialog.layer, bp_get_status());

  update_display();
}

static void on_unload_window(Window *window) {
  text_layer_destroy(app.airtouch_text_layer);
  text_layer_destroy(app.time_layer);
  text_layer_destroy(app.toast_text_layer);
  text_layer_destroy(app.presents_text_layer);
  text_layer_destroy(app.airtouch_big_text_layer);
  text_layer_destroy(app.dismiss_text_layer);
  gbitmap_destroy(app.res_bmp_logo_black);
  gbitmap_destroy(app.res_bmp_logo_white);
  bitmap_layer_destroy(app.bmp_logo_black_layer);
  bitmap_layer_destroy(app.bmp_logo_white_layer);
  bitmap_layer_destroy(app.top_bar_layer);
  bitmap_layer_destroy(app.bottom_bar_layer);
  dialog_destroy(&app.dialog);
  window_destroy(app.window);
}

static void on_click_select(ClickRecognizerRef recognizer, void *context) {
  toggle_event_time_range();
}

static void click_config_provider(Window *window) {
  sensismart_setup_controls(&AppAirtouch);
  window_single_click_subscribe(BUTTON_ID_SELECT, on_click_select);
}

static void activate() {
  AppAirtouch.window = window_create();
  app.window = AppAirtouch.window;
  window_set_window_handlers(app.window, (WindowHandlers) {
    .load = on_load_window,
    .unload = on_unload_window
  });
  window_set_click_config_provider(app.window, (ClickConfigProvider) click_config_provider);
  bp_subscribe((BackpackHandlers) {
    .on_connection_state_changed = on_connection_state_changed,
    .on_airtouch_event = on_airtouch,
  });
  window_stack_push(app.window, true);

  update_clock();
  tick_timer_service_subscribe(MINUTE_UNIT, handle_minute_tick);
}

static void deactivate() {
  if (app.notification_event_timer) {
    app_timer_cancel(app.notification_event_timer);
    app.notification_event_timer = NULL;
  }
  if (app.toast_show_timer) {
    app_timer_cancel(app.toast_show_timer);
    app.toast_show_timer = NULL;
  }
  window_stack_pop(true);
  bp_unsubscribe();

  tick_timer_service_unsubscribe();
}

SensiSmartApp AppAirtouch = {
  .name = "Airtouch",
  .window = NULL,
  .activate = activate,
  .deactivate = deactivate
};

