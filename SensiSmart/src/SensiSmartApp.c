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
#include "SensiSmartApp.h"
#include "utils.h"

static const char *BACKPACK_DISCONNECTED_TEXT = "Searching for\nBackpack...";
static const int DIALOG_HEIGHT = 60;
static const int DIALOG_WIDTH = 144;
static GBitmap *res_branding_logo;
static BitmapLayer *branding_layer;

static int num_apps = 0;
static int current_app = -1;
static SensiSmartApp **registered_apps;

static void on_click_back(ClickRecognizerRef recognizer, void *context) {
  /* DO NOTHING (long pressing will still exit) */
}

static void on_click_up(ClickRecognizerRef recognizer, void *context) {
  sensismart_app_prev();
}

static void on_click_down(ClickRecognizerRef recognizer, void *context) {
  sensismart_app_next();
}

void sensismart_window_load(SensiSmartApp *app) {
  window_set_background_color(app->window, GColorBlack);
}

void sensismart_app_init(int apps_len, SensiSmartApp *apps[]) {
  res_branding_logo = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_LOGO_BLACK);
  branding_layer = bitmap_layer_create(GRect(0, 135, 144, 23));
  bitmap_layer_set_bitmap(branding_layer, res_branding_logo);

  num_apps = apps_len;
  registered_apps = apps;
  current_app = -1;
  for (int i = 0; i < num_apps; ++i) {
    if (apps[i]->load)
      apps[i]->load();
  }
}

void sensismart_app_deinit() {
  for (int i = 0; i < num_apps; ++i) {
    if (registered_apps[i]->unload)
      registered_apps[i]->unload();
  }

  gbitmap_destroy(res_branding_logo);
  bitmap_layer_destroy(branding_layer);
}

static void switch_app(int app_idx) {
  if (current_app != -1) {
    if (num_apps == 1)
      return;
    registered_apps[current_app]->deactivate();
  }
  current_app = app_idx;
  DBG("Switch app: %s", registered_apps[current_app]->name);
  registered_apps[current_app]->activate();
}

void sensismart_app_next() {
  switch_app((current_app + 1) % num_apps);
}

void sensismart_app_prev() {
  switch_app((current_app + num_apps - 1) % num_apps);
}

void sensismart_setup_controls(SensiSmartApp *app) {
  window_single_click_subscribe(BUTTON_ID_BACK, on_click_back);
  window_single_click_subscribe(BUTTON_ID_UP, on_click_up);
  window_single_click_subscribe(BUTTON_ID_DOWN, on_click_down);
}

Layer *sensismart_get_branding_layer() {
  return (Layer *)branding_layer;
}

void on_dialog_update_proc(Layer *layer, GContext *ctx) {
  GRect rect = layer_get_bounds(layer);
  graphics_context_set_fill_color(ctx, GColorWhite);
  graphics_fill_rect(ctx, rect, 0, GCornerNone);
}

void dialog_create_disconnect_warning(Dialog *dialog) {
  dialog->layer = layer_create(GRect(0, 45, DIALOG_WIDTH, DIALOG_HEIGHT));

  dialog->res_icon = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_CAUTION);
  dialog->icon_layer = bitmap_layer_create(GRect(5, 5, 46, 50));
  bitmap_layer_set_bitmap(dialog->icon_layer, dialog->res_icon);
  layer_add_child(dialog->layer, (Layer *)dialog->icon_layer);

  dialog->text_layer = text_layer_create(GRect(50, 7, 84, 40));
  text_layer_set_font(dialog->text_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18));
  text_layer_set_text(dialog->text_layer, BACKPACK_DISCONNECTED_TEXT);
  text_layer_set_text_color(dialog->text_layer, GColorBlack);
  text_layer_set_background_color(dialog->text_layer, GColorWhite);
  text_layer_set_text_alignment(dialog->text_layer, GTextAlignmentCenter);
  layer_add_child(dialog->layer, text_layer_get_layer(dialog->text_layer));

  layer_set_update_proc(dialog->layer, (LayerUpdateProc)on_dialog_update_proc);
  layer_mark_dirty(dialog->layer);
}

void dialog_destroy(Dialog* dialog) {
  gbitmap_destroy(dialog->res_icon);
  bitmap_layer_destroy(dialog->icon_layer);
  layer_destroy(dialog->layer);
  text_layer_destroy(dialog->text_layer);
}
