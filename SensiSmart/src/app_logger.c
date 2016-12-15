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
#include "app_logger.h"

#define LONG_PRESS_INTERVAL_MS 1000

static const char *LOGGING_TITLE = "Logging";

static const char *LOG_CLEAR_TEXT     = "Press mid button to clear log";
static const char *LOG_CLEARING_TEXT  = "Clearing log...\n%d";
static const char *LOG_START_TEXT     = "Press mid button to start logging";
static const char *LOG_STOP_TEXT      = "Press mid button to stop logging";
static const char *LOG_CONTINUE_TEXT  = "Press mid button to continue";

static struct {
  Window *window;
  TextLayer *title_layer;
  TextLayer *log_text_layer;
  char log_text_layer_buf[20];
  AppTimer *clear_log_timer;
  Dialog dialog;
} app;

static void click_config_provider(Window *window);
static void on_log_clear_tick(void *data);

static void update_dialog_view_state(bool connected) {
  layer_set_hidden(app.dialog.layer, connected);
  layer_set_hidden(text_layer_get_layer(app.log_text_layer), !connected);
}

static void update_log_status_text(enum bp_log_status status, time_t remaining) {
  layer_set_frame(text_layer_get_layer(app.log_text_layer), GRect(0, 25, 144, 100));

  switch (status) {
  case STATUS_LOG_DIRTY:
    text_layer_set_text(app.log_text_layer, LOG_CLEAR_TEXT);
    break;

  case STATUS_LOG_CLEARING:
    layer_set_frame(text_layer_get_layer(app.log_text_layer), GRect(0, 45, 144, 100));

    snprintf(app.log_text_layer_buf, sizeof(app.log_text_layer_buf),
             LOG_CLEARING_TEXT, (int) remaining);
    text_layer_set_text(app.log_text_layer, app.log_text_layer_buf);
    break;

  case STATUS_LOG_CLEARED:
    text_layer_set_text(app.log_text_layer, LOG_START_TEXT);
    break;

  case STATUS_LOG_STARTED:
    text_layer_set_text(app.log_text_layer, LOG_STOP_TEXT);
    break;

  case STATUS_LOG_STOPPED:
    text_layer_set_text(app.log_text_layer, LOG_CONTINUE_TEXT);
    break;
  }

  update_dialog_view_state(bp_get_status());
  layer_mark_dirty(text_layer_get_layer(app.log_text_layer));
}

static void on_log_interrupt() {
  update_log_status_text(bp_log_get_status(), 0);
}

static void on_connection_state_changed(bool connected) {
  update_dialog_view_state(connected);
}

static void on_load_window(Window *window) {
  sensismart_window_load(&AppLogger);
  Layer *root_layer = window_get_root_layer(window);

  // Screen Title
  app.title_layer = text_layer_create(GRect(0, 0, 144, 30));
  text_layer_set_font(app.title_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18));
  text_layer_set_text(app.title_layer, LOGGING_TITLE);
  text_layer_set_text_color(app.title_layer, GColorWhite);
  text_layer_set_background_color(app.title_layer, GColorBlack);
  text_layer_set_text_alignment(app.title_layer, GTextAlignmentCenter);
  layer_add_child(root_layer, text_layer_get_layer(app.title_layer));

  // Logging Status
  app.log_text_layer = text_layer_create(GRect(0, 25, 144, 100));
  text_layer_set_font(app.log_text_layer, fonts_get_system_font(FONT_KEY_GOTHIC_28));
  text_layer_set_text_color(app.log_text_layer, GColorBrightGreen);
  text_layer_set_background_color(app.log_text_layer, GColorBlack);
  text_layer_set_text_alignment(app.log_text_layer, GTextAlignmentCenter);
  text_layer_set_overflow_mode(app.log_text_layer, GTextOverflowModeWordWrap);
  layer_add_child(root_layer, text_layer_get_layer(app.log_text_layer));

  // Sensirion Logo
  layer_add_child(root_layer, sensismart_get_branding_layer());

  // Dialog Box for Disconnect Events
  dialog_create_disconnect_warning(&app.dialog);
  layer_add_child(root_layer, app.dialog.layer);

  update_dialog_view_state(bp_get_status());

  enum bp_log_status status = bp_log_get_status();
  if (status == STATUS_LOG_CLEARING)
    on_log_clear_tick(NULL);
  else
    update_log_status_text(status, 0);
}

static void on_unload_window(Window *window) {
  text_layer_destroy(app.title_layer);
  text_layer_destroy(app.log_text_layer);
  dialog_destroy(&app.dialog);
  window_destroy(app.window);
}

static void on_log_clear_tick(void *data) {
  time_t remaining = bp_log_remaining();
  if (remaining <= 0) {
    app.clear_log_timer = NULL;
    update_log_status_text(STATUS_LOG_CLEARED, 0);
  } else {
    update_log_status_text(STATUS_LOG_CLEARING, remaining);
    app.clear_log_timer =
        app_timer_register(1000, (AppTimerCallback) on_log_clear_tick, NULL);
  }
}

static void on_click_select(ClickRecognizerRef recognizer, void *context) {
  if (!bp_get_status())
    return;

  enum bp_log_status status = bp_log_get_status();
  uint8_t click = click_number_of_clicks_counted(recognizer);
  if (click > 3 || status == STATUS_LOG_CLEARING)
    return;
  if (click > 1 && status > STATUS_LOG_CLEARED)
    return;

  if (status < STATUS_LOG_CLEARED) {
    if (click == 1) {
      text_layer_set_text(app.log_text_layer, LOG_CLEAR_TEXT);
    } else if (bp_get_status()) {
      /* click 2 is sent right after 1, only click 3 is delayed correctly */
      if (click == 3)
        INFO("Forcing log clearing");
      else
        return;
      bp_log_clear();
      on_log_clear_tick(NULL);
    }
  } else if (status == STATUS_LOG_CLEARED ||
             status == STATUS_LOG_STOPPED) {
    bp_log_start();
    text_layer_set_text(app.log_text_layer, LOG_STOP_TEXT);
  } else if (status == STATUS_LOG_STARTED) {
    bp_log_stop();
    text_layer_set_text(app.log_text_layer, LOG_CONTINUE_TEXT);
  }
}

static void click_config_provider(Window *window) {
  sensismart_setup_controls(&AppLogger);
  window_single_repeating_click_subscribe(BUTTON_ID_SELECT,
                                          LONG_PRESS_INTERVAL_MS,
                                          on_click_select);
}

static void activate() {
  AppLogger.window = window_create();
  app.window = AppLogger.window;
  app.clear_log_timer = NULL;
  window_set_window_handlers(app.window, (WindowHandlers) {
    .load = on_load_window,
    .unload = on_unload_window
  });
  window_set_click_config_provider(app.window, (ClickConfigProvider) click_config_provider);
  bp_subscribe((BackpackHandlers) {
    .on_connection_state_changed = on_connection_state_changed
  });
  bp_set_log_interrupt_handler(on_log_interrupt);
  window_stack_push(app.window, true);
}

static void deactivate() {
  window_stack_pop(true);
  if (app.clear_log_timer)
    app_timer_cancel(app.clear_log_timer);
  bp_unsubscribe();
}

SensiSmartApp AppLogger = {
  .name = "Logger",
  .window = NULL,
  .activate = activate,
  .deactivate = deactivate
};

