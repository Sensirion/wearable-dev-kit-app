/*
 * Copyright (c) 2016, Sensirion AG
 * Author: Daniel Lehmann <daniel.lehman@sensirion.com>
 *         Andreas Brauchli <andreas.brauchli@sensirion.com>
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
//#include "app_airtouch.h"
//#include "app_thermal_context.h"
#include "app_perspiration_chart.h"
//#include "app_feellike.h"
#include "app_logger.h"
#include "app_version.h"
//#include "app_onbody_demo.h"
//#include "app_raw.h"
//#include "app_temp_compensation.h"
//#include "app_thermal_values.h"

/* Keep the list in sync with the apps array below */
enum AppList {

  //APP_THERMAL_CONTEXT,
  //APP_THERMAL_VALUES,
  //APP_AIRTOUCH,
  APP_LOGGER,
  APP_VERSION,
  //APP_ONBODY_DEMO,
  APP_PERSPIRATION_CHART,
  //APP_RAW,
  //APP_FEELLIKE,
  //APP_TEMP_COMPENSATION,
  NUM_APPS
};

/* Keep the list in sync with the app list enum above */
static SensiSmartApp *apps[] = {
  //&AppThermalContext,
  //&AppThermalValues,
  //&AppAirtouch,
  &AppLogger,
  &AppVersion,
  //&AppOnbodyDemo,
  &AppPerspirationChart,
  //&AppRaw,
  //&AppFeellike,
  //&AppTempCompensation,
  NULL
};

static int init() {
  int ret = bp_init();
  sensismart_app_init(NUM_APPS, apps);
  sensismart_app_next();
  return ret;
}

static void deinit() {
  sensismart_app_deinit();
  bp_deinit();
}

int main() {
  DBG("STARTING APP");
  if (init())
    app_event_loop();
  DBG("ENDING APP");
  deinit();
  return 0;
}

