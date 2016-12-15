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

#ifndef SENSI_SMART_APP_H
#define SENSI_SMART_APP_H

#include <pebble.h>

/** Basic structure for a SensiSmart mini-app/window */
typedef struct SensiSmartApp {
  /** SensiSmartApp name (mainly for debug messages) */
  const char *name;
  /** Pointer to the Window (NULL if none) */
  Window *window;
  /** Mini-app initialization callback (only called once) */
  void (*load)();
  /** Mini-app finalization callback (only called once) */
  void (*unload)();
  /** Mini-app activation callback */
  void (*activate)();
  /** Mini-app deactivation callback */
  void (*deactivate)();
} SensiSmartApp;

/**
 * Initialize the app switching module and load all apps
 * Call sensismart_app_next() to open the initial app
 */
void sensismart_app_init(int apps_len, SensiSmartApp *apps[]);

/**
 * Finalize the app switching module and unload all apps
 */
void sensismart_app_deinit();

/** Switch to the next app */
void sensismart_app_next();

/** Switch to the previous app */
void sensismart_app_prev();

/**
 * Setup the basic window
 * o set default background color
 * o push window on window stack
 */
void sensismart_window_load(SensiSmartApp *app);

/**
 * Setup the button handlers for previous/next window.
 * This function will map the back, up and down buttons.
 *
 * This function can only be called from within a ClickConfigProvider function!
 */
void sensismart_setup_controls(SensiSmartApp *app);

/**
 * Retrieve the layer containing the branding (logo)
 * It is the caller's responsability to add the layer to the window but it must
 * not be freed or modified!
 */
Layer *sensismart_get_branding_layer();

typedef struct Dialog {
  Layer *layer;
  GBitmap *res_icon;
  BitmapLayer *icon_layer;
  TextLayer *text_layer;
} Dialog;

/**
 * Create a backpack disconnected warning dialog
 */
void dialog_create_disconnect_warning(Dialog *dialog);

/**
 * Free a dialog
 */
void dialog_destroy(Dialog *dialog);


#endif /* SENSI_SMART_APP_H */

