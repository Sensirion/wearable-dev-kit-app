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
#include "utils.h"

static const int DELAY_POLL_INTERVAL_MS   = 10;
static const int LOGGER_CHECK_INTERVAL_MS = 60000;

static uint32_t polling_interval_ms       = DEFAULT_POLL_INTERVAL_MS;
static enum bp_log_status log_status      = STATUS_LOG_DIRTY;
static time_t log_clear_time_end;
static AppTimer *polling_timer            = NULL;
static AppTimer *log_watchdog_timer       = NULL;
static volatile int open_reads            = 0;
static uint32_t logged_values_mask        = 0x00000000;
static uint16_t available_sensor_readings_mask  = 0x0000;
static uint16_t available_processed_values_mask = 0x0000;
static char bp_firmware_version[60];
static bool is_plugged;

static enum init_state_flags {
  UNINITIALIZED                           = 0,
  READ_FW_VERSION                         = 1 << 0,
  READ_AVAILABLE_SENSOR_READINGS_MASK     = 1 << 1,
  READ_AVAILABLE_PROCESSED_VALUES_MASK    = 1 << 2,
  READ_LOGGED_VALUES_MASK                 = 1 << 3,
  INITIALIZED = READ_FW_VERSION |
                READ_AVAILABLE_SENSOR_READINGS_MASK |
                READ_AVAILABLE_PROCESSED_VALUES_MASK |
                READ_LOGGED_VALUES_MASK
} init_state = UNINITIALIZED;

struct log_start_msg {
  uint64_t start_time_ms;
  uint32_t log_interval_ms;
  uint32_t enabled_channels_mask;
};

#define MAX_SUBSCRIBED_ATTRIBUTES 32

struct BackpackAttribute at_sensor_readings;
struct BackpackAttribute at_processed_values;
struct BackpackAttribute at_logger_clear;
struct BackpackAttribute at_logger_start;
struct BackpackAttribute at_logger_pause;
struct BackpackAttribute at_logger_resume;
struct BackpackAttribute at_logger_state;
struct BackpackAttribute at_temperature_compensation_mode;
struct BackpackAttribute at_airtouch_start_event;
struct BackpackAttribute at_airtouch_stop_event;
struct BackpackAttribute at_onbody_event;
struct BackpackAttribute at_offbody_event;
struct BackpackAttribute at_onbody_state;
struct BackpackAttribute at_system_plugged;
struct BackpackAttribute at_system_unplugged;
struct BackpackAttribute at_system_available_sensor_readings;
struct BackpackAttribute at_system_available_processed_values;
struct BackpackAttribute at_system_version;

/**
 * CAUTION! Keep in sync with pebble firmware!
 */
enum fw_logger_state {
    EMPTY,
    DIRTY,
    ERASING,
    WRITING,
    WRITING_PAUSED,
    LOG_FULL, // log full
    READING,
    READING_FINISHED, // everything read
};

static int num_attributes = 0;
static int num_subscribed_attributes = 0;
static struct BackpackAttribute *attributes[MAX_SUBSCRIBED_ATTRIBUTES] = {NULL};
static struct BackpackAttribute *subscribed_attributes[MAX_SUBSCRIBED_ATTRIBUTES];

static BackpackHandlers bp_handlers;
static TemperatureCompensationModeHandler temperature_compensation_mode_handler = NULL;
static LogInterruptHandler log_interrupt_handler = NULL;

/* Forward declarations */
void check_log_state();
void log_watchdog_timer_fired();
static void on_battery_state_changed(BatteryChargeState charge);
static void timer_resume();
static void timer_suspend();

static void at_subscribe(struct BackpackAttribute *at) {
  if (num_subscribed_attributes>=MAX_SUBSCRIBED_ATTRIBUTES) {
    ERR("No more space for attributes! Increase MAX_SUBSCRIBED_ATTRIBUTES");
    return;
  }
  subscribed_attributes[num_subscribed_attributes++] = at;
}

static void at_unsubscribe_all() {
  num_subscribed_attributes = 0;
  open_reads = 0;
}

static void at_read(struct BackpackAttribute *at) {
  if (at->open_read) {
    ERR("at_read: attribute %s: still waiting on last read, discarding", at->desc);
    return;
  }
  if (smartstrap_attribute_read(at->attribute) == SmartstrapResultOk) {
    open_reads += 1;
    at->open_read = true;
  } else {
    ERR("at_read: attribute %s cannot be read", at->desc);
  }
}

static void at_write(struct BackpackAttribute *at, uint8_t value,
                     bool request_read) {
  if (at->open_read) {
    ERR("at_write: attribute %s still waiting on last read, discarding", at->desc);
    return;
  }
  uint8_t *buf;
  size_t buflen;
  if (smartstrap_attribute_begin_write(at->attribute,
                                       &buf,
                                       &buflen) == SmartstrapResultOk) {
    *buf = value;
    if (smartstrap_attribute_end_write(at->attribute,
                                       sizeof(uint8_t),
                                       request_read) == SmartstrapResultOk) {
      if (request_read) {
        at->open_read = true;
        open_reads += 1;
      }
    } else {
      ERR("End write failed for attribute %s", at->desc);
    }
  } else {
    ERR("Begin write failed for attribute %s", at->desc);
  }
}

static bool at_handle_read(struct BackpackAttribute *at, const uint8_t *data,
                           size_t length, SmartstrapAttribute *attr) {
  if (at->attribute != attr)
    return false;
  at->open_read = false;
  if (at->handler)
    at->handler(data, length, smartstrap_attribute_get_attribute_id(at->attribute));
  return true;
}

static void at_init(struct BackpackAttribute *attribute,
                    SmartstrapServiceId service_id,
                    SmartstrapAttributeId attribute_id,
                    size_t len, const char *desc,
                    BackpackAttributeHandler handler) {
  *attribute = (struct BackpackAttribute) {
    .attribute = smartstrap_attribute_create(service_id, attribute_id, len),
    .desc = desc,
    .handler = handler,
    .id = num_attributes++,
    .open_read = false
  };
  if (attribute->id >= MAX_SUBSCRIBED_ATTRIBUTES) {
    ERR("No more space for attributes! Increase MAX_SUBSCRIBED_ATTRIBUTES");
    return;
  }
  attributes[attribute->id] = attribute;
}

static void at_destroy(struct BackpackAttribute *at) {
  if (at->open_read)
    WARN("at_destroy: attribute %s: Destroying attribute with open reads", at->desc);
  if (at->attribute != NULL) {
    smartstrap_attribute_destroy(at->attribute);
    at->attribute = NULL;
  }
}

bool bp_destroy_attribute(struct BackpackAttribute *at) {
  if (at->open_read)
    return false;
  at_destroy(at);
  return true;
}

bool bp_readval(const uint8_t *data, size_t len, int *offset, void *result,
                size_t type_len, const char *desc) {
  if (*offset + type_len > len) {
    ERR("bp_readval: Attempting to read beyond buffer (offset: %d but length: %d on %s)",
        *offset, len, desc);
    return false;
  }
  memcpy(result, data + *offset, type_len);
  *offset += type_len;
  return true;
}

static void process_sensor_readings(const uint8_t *data, size_t length,
                                    SmartstrapAttributeId attribute_id) {
  int32_t t_c     = 0;
  int32_t rh      = 0;
  int32_t t_skin  = 0;
  // place holder for any value
  uint32_t reserved0 = 0;
  uint32_t reserved1 = 0;
  int offset      = 0;

  if (attribute_id & ATTR_SENSOR_READINGS_TEMPERATURE) {
    bp_readval(data, length, &offset, &t_c,
               ATTR_SENSOR_READINGS_TEMPERATURE_LEN, "temperature");
  }

  if (attribute_id & ATTR_SENSOR_READINGS_HUMIDITY) {
    bp_readval(data, length, &offset, &rh,
               ATTR_SENSOR_READINGS_HUMIDITY_LEN, "humidity");
  }

  if (attribute_id & ATTR_SENSOR_READINGS_SKIN_TEMPERATURE) {
    bp_readval(data, length, &offset, &t_skin,
               ATTR_SENSOR_READINGS_SKIN_TEMPERATURE_LEN, "skin temperature");
  }

  if (attribute_id & ATTR_SENSOR_READINGS_RESERVED0) {
    bp_readval(data, length, &offset, &reserved0,
               ATTR_SENSOR_READINGS_RESERVED_LEN, "resistance_reserved_1");
  }

  if (attribute_id & ATTR_SENSOR_READINGS_RESERVED1) {
    bp_readval(data, length, &offset, &reserved1,
               ATTR_SENSOR_READINGS_RESERVED_LEN, "resistance_reserved_2");
  }

  if (bp_handlers.on_sensor_readings)
      bp_handlers.on_sensor_readings(t_c, rh, t_skin, reserved0, reserved1);
}

static void process_processed_values(const uint8_t *data, size_t length,
                                     SmartstrapAttributeId attribute_id) {
  float t_skin;
  float t_feellike;
  float t_apparent;
  float t_humidex;
  int offset = 0;

  if (attribute_id & ATTR_PROCESSED_VALUES_SKIN_TEMPERATURE) {
    bp_readval(data, length, &offset, &t_skin,
               ATTR_PROCESSED_VALUES_SKIN_TEMPERATURE_LEN, "skin temperature");
  }

  if (attribute_id & ATTR_PROCESSED_VALUES_FEELLIKE_TEMPERATURE) {
    bp_readval(data, length, &offset, &t_feellike,
               ATTR_PROCESSED_VALUES_FEELLIKE_TEMPERATURE_LEN,
               "feellike temperature");
  }

  if (attribute_id & ATTR_PROCESSED_VALUES_APPARENT_TEMPERATURE) {
    bp_readval(data, length, &offset, &t_apparent,
               ATTR_PROCESSED_VALUES_APPARENT_TEMPERATURE_LEN,
               "apparent temperature");
  }

  if (attribute_id & ATTR_PROCESSED_VALUES_HUMIDEX) {
    bp_readval(data, length, &offset, &t_humidex,
               ATTR_PROCESSED_VALUES_HUMIDEX_LEN,
               "humidex temperature");
  }

  if (bp_handlers.on_processed_values)
      bp_handlers.on_processed_values(t_skin, t_feellike, t_apparent, t_humidex);
}

static bool reset_attribute_read_state(SmartstrapAttribute *attr) {
  if (open_reads && attr == NULL)
    return false;

  bool ret = false;
  int i;
  for (i = 0; i < num_attributes; ++i) {
    if (attributes[i]) {
      if (attr && attr == attributes[i]->attribute) {
        attributes[i]->open_read = false;
        ret = true;
      } else if (!open_reads) {
        attributes[i]->open_read = false;
      }
    }
  }
  return ret;
}

static void set_initialized_state(enum init_state_flags new_init_state) {
  if (new_init_state == UNINITIALIZED) {
    init_state = UNINITIALIZED;
    logged_values_mask = 0x00000000;
    if (polling_timer)
      timer_suspend();
  } else {
    init_state |= new_init_state;
  }

  bool initialized = (init_state == INITIALIZED);
  if (initialized) {
    logged_values_mask = (
      (
        /* Sensor reading flags */
        available_sensor_readings_mask & (
          ATTR_SENSOR_READINGS_TEMPERATURE              |
          ATTR_SENSOR_READINGS_HUMIDITY                 |
          ATTR_SENSOR_READINGS_SKIN_TEMPERATURE         |
          ATTR_SENSOR_READINGS_SKIN_HUMIDITY
        )
      ) | (
        (uint32_t) (
          /* Processed values flags */
          available_processed_values_mask & (
            ATTR_PROCESSED_VALUES_TRANSPIRATION
          )
        ) << 16
      )
    );

    if (polling_timer)
      timer_resume();
  }

  if (bp_handlers.on_connection_state_changed) {
    if (init_state == UNINITIALIZED || initialized)
      bp_handlers.on_connection_state_changed(initialized);
  }
}

static void on_connection_state_changed(bool connected) {
  if (connected) {
    at_read(&at_system_version);
    at_read(&at_system_available_sensor_readings);
    at_read(&at_system_available_processed_values);
    BatteryChargeState charge = battery_state_service_peek();
    /* init with wrong state as event is only triggered if state changes */
    is_plugged = !charge.is_plugged;
    on_battery_state_changed(charge);
  } else {
    set_initialized_state(UNINITIALIZED);
    bp_firmware_version[0] = '\0';
    available_sensor_readings_mask = 0x0000;
    available_processed_values_mask = 0x0000;
  }
}

static void on_did_read(SmartstrapAttribute *attr, SmartstrapResult result,
                        const uint8_t *data, size_t length) {
  if (open_reads > 0)
    --open_reads;
  SmartstrapServiceId service_id = smartstrap_attribute_get_service_id(attr);
  SmartstrapAttributeId attribute_id = smartstrap_attribute_get_attribute_id(attr);
  if (result != SmartstrapResultOk) {
    ERR("read %db from %04x:%04x failed (result %d)",
        length, service_id, attribute_id, result);
    reset_attribute_read_state(attr);
    return;
  }
  DBG("read %db from %04x:%04x", length, service_id, attribute_id);
  bool handled = false;
  for (int i = 0; i < num_attributes; ++i) {
    if (attributes[i] && at_handle_read(attributes[i], data, length, attr)) {
      handled = true;
      break;
    }
  }

  if (!handled && !reset_attribute_read_state(attr)) {
    WARN("read %db from unknown service %04x:%04x",
         length, service_id, attribute_id);
  }
}

static void on_did_write(SmartstrapAttribute *attr, SmartstrapResult result) {
  SmartstrapServiceId service_id = smartstrap_attribute_get_service_id(attr);
  SmartstrapAttributeId attribute_id = smartstrap_attribute_get_attribute_id(attr);
  if (result != SmartstrapResultOk) {
    ERR("Writing to %04x:%04x failed (result %d)",
        service_id, attribute_id, result);
  } else {
    DBG("Did write to %04x:%04x", service_id, attribute_id);
  }
}

static void send_request_loop(void *context) {
  int i;
  for (i = 0; i < num_subscribed_attributes; ++i) {
    if (subscribed_attributes[i]->open_read) {
      WARN("open read for attribute %s left, delaying poll", subscribed_attributes[i]->desc);
      continue;
    }
    at_read(subscribed_attributes[i]);
  }

  polling_timer = app_timer_register(polling_interval_ms, send_request_loop,
                                     context);
}

static void timer_suspend() {
  if (!polling_timer)
    return;
  app_timer_cancel(polling_timer);
  polling_timer = NULL;
}

static void timer_resume() {
  if (num_subscribed_attributes == 0 || polling_timer)
    return;
  polling_timer = app_timer_register(0, send_request_loop, NULL);
}

static void on_battery_state_changed(BatteryChargeState charge) {
  if (charge.is_plugged == is_plugged || init_state != INITIALIZED)
    return;
  is_plugged = charge.is_plugged;
  DBG("Pebble is plugged: %d", is_plugged);
  if (is_plugged) {
    at_write(&at_system_plugged, 0, false);
  } else {
    at_write(&at_system_unplugged, 0, false);
  }
}

static void on_availability_did_change(SmartstrapServiceId service_id,
                                       bool is_available) {
  DBG("Availability for 0x%04x is %d", service_id, is_available);
  if (service_id == SERVICE_SYSTEM)
    on_connection_state_changed(is_available);

  if (is_available && service_id == SERVICE_LOGGER)
    check_log_state();

  if (bp_handlers.availability_did_change)
    bp_handlers.availability_did_change(service_id, is_available);
}

static void on_temperature_compensation_mode_read(const uint8_t *data,
                                                  size_t length,
                                                  SmartstrapAttributeId id) {
  if (temperature_compensation_mode_handler != NULL) {
    if (length == ATTR_TEMPERATURE_COMPENSATION_MODE_LEN) {
      temperature_compensation_mode_handler(data[0], data[1]);
    } else {
      ERR("Temperature Compensation Mode has length %d", length);
    }
  }
  temperature_compensation_mode_handler = NULL;
}

void bp_set_temperature_compensation_mode(uint8_t mode,
                                          TemperatureCompensationModeHandler handler) {
  temperature_compensation_mode_handler = handler;
  at_write(&at_temperature_compensation_mode, mode, true);
}

static void on_onbody_state_read(const uint8_t *data, size_t length,
                                 SmartstrapAttributeId id) {
  if (bp_handlers.on_onbody_event) {
    if (length == ATTR_PROCESSED_VALUES_ONBODY_STATE_LEN) {
      bp_handlers.on_onbody_event((bool) *data); /* trigger backpack event */
    } else {
      ERR("Onbody state has length %d", length);
    }
  }
}

static void on_notified(SmartstrapAttribute *attr) {
  SmartstrapServiceId service_id = smartstrap_attribute_get_service_id(attr);
  SmartstrapAttributeId attribute_id = smartstrap_attribute_get_attribute_id(attr);
  if (service_id == SERVICE_SENSOR_READINGS) {
    DBG("notified from service %04x:%04x", service_id, attribute_id);

  } else if (service_id == SERVICE_PROCESSED_VALUES) {
    /* Airtouch */
    if (attribute_id == ATTR_PROCESSED_VALUES_AIRTOUCH_START_EVENT) {
      DBG("notified: AirTouch Start");
      if (bp_handlers.on_airtouch_event)
        bp_handlers.on_airtouch_event(true);
    } else if (attribute_id == ATTR_PROCESSED_VALUES_AIRTOUCH_STOP_EVENT) {
      DBG("notified: AirTouch Stop");
      if (bp_handlers.on_airtouch_event)
        bp_handlers.on_airtouch_event(false);
    /* On/Off body */
    } else if (attribute_id == ATTR_PROCESSED_VALUES_ONBODY_EVENT) {
      DBG("notified: on-body");
      if (bp_handlers.on_onbody_event)
        bp_handlers.on_onbody_event(true);
    } else if (attribute_id == ATTR_PROCESSED_VALUES_OFFBODY_EVENT) {
      DBG("notified: off-body");
      if (bp_handlers.on_onbody_event)
        bp_handlers.on_onbody_event(false);
    /* Processed values catch all */
    } else {
      DBG("notified from service %04x:%04x", service_id, attribute_id);
    }

  } else {
    ERR("notified from unknown service %04x:%04x", service_id, attribute_id);
  }
}

static void on_logger_status_read(const uint8_t *data, size_t length,
                                  SmartstrapAttributeId id) {
  switch (data[0]) {
    case EMPTY:
      INFO("Log state; EMPTY");
      log_status = STATUS_LOG_CLEARED;
      log_clear_time_end = 0;
      if (log_interrupt_handler)
        log_interrupt_handler();
      break;
    case DIRTY:
      INFO("Log state; DIRTY");
      log_status = STATUS_LOG_DIRTY;
      log_clear_time_end = 0;
      if (log_interrupt_handler)
        log_interrupt_handler();
      break;
    default:
      // ignore
      break;
  }
  set_initialized_state(READ_LOGGED_VALUES_MASK);
}

static void on_system_version_read(const uint8_t *data, size_t length,
                                   SmartstrapAttributeId id) {
  int len = length;
  if (length >= sizeof(bp_firmware_version))
    len = sizeof(bp_firmware_version) - 1;
  memcpy(bp_firmware_version, data, len);
  bp_firmware_version[len] = '\0';
  INFO("Backpack firmware version %s", bp_get_version());
  set_initialized_state(READ_FW_VERSION);
}

static void on_available_sensor_readings_read(const uint8_t *data, size_t length,
                                              SmartstrapAttributeId id) {
  available_sensor_readings_mask = *((uint16_t *)data);
  INFO("Available sensor readings: 0x%04x", available_sensor_readings_mask);
  set_initialized_state(READ_AVAILABLE_SENSOR_READINGS_MASK);
}

static void on_available_processed_values_read(const uint8_t *data, size_t length,
                                               SmartstrapAttributeId id) {
  available_processed_values_mask = *((uint16_t *)data);
  INFO("Available processed values: 0x%04x", available_processed_values_mask);
  set_initialized_state(READ_AVAILABLE_PROCESSED_VALUES_MASK);
}

static void peek_smartstrap_state() {
  unsigned i;
  const SmartstrapServiceId service_ids[] = { SERVICE_SENSOR_READINGS,
                                              SERVICE_PROCESSED_VALUES,
                                              SERVICE_LOGGER,
                                              SERVICE_SYSTEM };
  for (i = 0; i < sizeof(service_ids) / sizeof(SmartstrapServiceId); ++i) {
    if (smartstrap_service_is_available(service_ids[i]))
      on_availability_did_change(service_ids[i], true);
  }
}

int bp_init() {
  int ret = 1;

  SmartstrapAttributeId sensors =
      ATTR_SENSOR_READINGS_TEMPERATURE |
      ATTR_SENSOR_READINGS_HUMIDITY |
      ATTR_SENSOR_READINGS_SKIN_TEMPERATURE |
      ATTR_SENSOR_READINGS_RESERVED0 |
      ATTR_SENSOR_READINGS_RESERVED1;
  size_t sensors_len =
      ATTR_SENSOR_READINGS_TEMPERATURE_LEN +
      ATTR_SENSOR_READINGS_HUMIDITY_LEN +
      ATTR_SENSOR_READINGS_SKIN_TEMPERATURE_LEN +
      ATTR_SENSOR_READINGS_RESERVED_LEN +
      ATTR_SENSOR_READINGS_RESERVED_LEN;
  at_init(&at_sensor_readings, SERVICE_SENSOR_READINGS, sensors, sensors_len,
          "Sensor readings", process_sensor_readings);
  ret &= (at_sensor_readings.attribute != NULL);

  SmartstrapAttributeId processed_values =
      ATTR_PROCESSED_VALUES_SKIN_TEMPERATURE |
      ATTR_PROCESSED_VALUES_FEELLIKE_TEMPERATURE |
      ATTR_PROCESSED_VALUES_APPARENT_TEMPERATURE |
      ATTR_PROCESSED_VALUES_HUMIDEX |
      ATTR_SENSOR_READINGS_RESERVED0 |
      ATTR_SENSOR_READINGS_RESERVED1;
  size_t processed_values_len =
      ATTR_PROCESSED_VALUES_SKIN_TEMPERATURE_LEN +
      ATTR_PROCESSED_VALUES_FEELLIKE_TEMPERATURE_LEN +
      ATTR_PROCESSED_VALUES_APPARENT_TEMPERATURE_LEN +
      ATTR_PROCESSED_VALUES_HUMIDEX_LEN;

  at_init(&at_processed_values, SERVICE_PROCESSED_VALUES, processed_values,
          processed_values_len, "Processed values", process_processed_values);
  ret &= (at_processed_values.attribute != NULL);

  at_init(&at_logger_clear, SERVICE_LOGGER, ATTR_LOGGER_CLEAR,
          ATTR_LOGGER_CLEAR_LEN, "Log clear", NULL);
  ret &= (at_logger_clear.attribute != NULL);
  at_init(&at_logger_start, SERVICE_LOGGER, ATTR_LOGGER_START,
          ATTR_LOGGER_START_LEN, "Log start", NULL);
  ret &= (at_logger_start.attribute != NULL);
  at_init(&at_logger_pause, SERVICE_LOGGER, ATTR_LOGGER_PAUSE,
          ATTR_LOGGER_PAUSE_LEN, "Log pause", NULL);
  ret &= (at_logger_pause.attribute != NULL);
  at_init(&at_logger_resume, SERVICE_LOGGER, ATTR_LOGGER_RESUME,
          ATTR_LOGGER_RESUME_LEN, "Log resume", NULL);
  ret &= (at_logger_resume.attribute != NULL);

  at_init(&at_airtouch_start_event, SERVICE_PROCESSED_VALUES,
          ATTR_PROCESSED_VALUES_AIRTOUCH_START_EVENT,
          ATTR_EVENT_LEN, "Airtouch start", NULL);
  ret &= (at_airtouch_start_event.attribute != NULL);
  at_init(&at_airtouch_stop_event, SERVICE_PROCESSED_VALUES,
          ATTR_PROCESSED_VALUES_AIRTOUCH_STOP_EVENT,
          ATTR_EVENT_LEN, "Airtouch stop", NULL);
  ret &= (at_airtouch_stop_event.attribute != NULL);

  at_init(&at_onbody_event, SERVICE_PROCESSED_VALUES,
          ATTR_PROCESSED_VALUES_ONBODY_EVENT,
          ATTR_EVENT_LEN, "Onbody", NULL);
  ret &= (at_onbody_event.attribute != NULL);
  at_init(&at_offbody_event, SERVICE_PROCESSED_VALUES,
          ATTR_PROCESSED_VALUES_OFFBODY_EVENT,
          ATTR_EVENT_LEN, "Offbody", NULL);
  ret &= (at_offbody_event.attribute != NULL);

  at_init(&at_temperature_compensation_mode, SERVICE_PROCESSED_VALUES,
          ATTR_TEMPERATURE_COMPENSATION_MODE,
          ATTR_TEMPERATURE_COMPENSATION_MODE_LEN,
          "Temperature Compensation Mode", on_temperature_compensation_mode_read);
  ret &= (at_temperature_compensation_mode.attribute != NULL);

  at_init(&at_onbody_state, SERVICE_PROCESSED_VALUES,
          ATTR_PROCESSED_VALUES_ONBODY_STATE,
          ATTR_PROCESSED_VALUES_ONBODY_STATE_LEN,
          "Onbody State", on_onbody_state_read);
  ret &= (at_onbody_state.attribute != NULL);

  at_init(&at_logger_state, SERVICE_LOGGER,
          ATTR_LOGGER_STATE,
          ATTR_LOGGER_STATE_LEN,
          "Logger State", on_logger_status_read);
  ret &= (at_logger_state.attribute != NULL);

  at_init(&at_system_plugged, SERVICE_SYSTEM, ATTR_SYSTEM_PLUGGED,
          ATTR_SYSTEM_PLUGGED_LEN, "System Plugged", NULL);
  ret &= (at_system_plugged.attribute != NULL);
  at_init(&at_system_unplugged, SERVICE_SYSTEM, ATTR_SYSTEM_UNPLUGGED,
          ATTR_SYSTEM_UNPLUGGED_LEN, "System Unplugged", NULL);
  ret &= (at_system_unplugged.attribute != NULL);
  at_init(&at_system_version, SERVICE_SYSTEM, ATTR_SYSTEM_VERSION,
          ATTR_SYSTEM_VERSION_MAX_LEN, "System Version", on_system_version_read);
  ret &= (at_system_version.attribute != NULL);
  at_init(&at_system_available_sensor_readings, SERVICE_SYSTEM,
          ATTR_SYSTEM_AVAILABLE_SENSOR_READINGS_MASK,
          ATTR_SYSTEM_AVAILABLE_SENSOR_READINGS_MASK_LEN,
          "Available Sensor Readings", on_available_sensor_readings_read);
  ret &= (at_system_available_sensor_readings.attribute != NULL);
  at_init(&at_system_available_processed_values, SERVICE_SYSTEM,
          ATTR_SYSTEM_AVAILABLE_PROCESSED_VALUES_MASK,
          ATTR_SYSTEM_AVAILABLE_PROCESSED_VALUES_MASK_LEN,
          "Available Processed Values", on_available_processed_values_read);
  ret &= (at_system_available_processed_values.attribute != NULL);

  peek_smartstrap_state();

  SmartstrapHandlers handlers = (SmartstrapHandlers) {
    .availability_did_change = on_availability_did_change,
    .did_write = on_did_write,
    .did_read = on_did_read,
    .notified = on_notified
  };
  smartstrap_subscribe(handlers);
  smartstrap_set_timeout(BACKPACK_TIMEOUT);

  battery_state_service_subscribe(on_battery_state_changed);

  return ret;
}

static void cleanup_attributes(void *ctx) {
  at_destroy(&at_sensor_readings);
  at_destroy(&at_processed_values);
  at_destroy(&at_logger_clear);
  at_destroy(&at_logger_start);
  at_destroy(&at_logger_pause);
  at_destroy(&at_logger_resume);
  at_destroy(&at_airtouch_start_event);
  at_destroy(&at_airtouch_stop_event);
  at_destroy(&at_onbody_event);
  at_destroy(&at_offbody_event);
  at_destroy(&at_onbody_state);
  at_destroy(&at_temperature_compensation_mode);
  at_destroy(&at_logger_state);
  at_destroy(&at_system_plugged);
  at_destroy(&at_system_unplugged);
  at_destroy(&at_system_version);
  at_destroy(&at_system_available_sensor_readings);
  at_destroy(&at_system_available_processed_values);
}

void bp_deinit() {
  battery_state_service_unsubscribe();
  timer_suspend();
  cleanup_attributes(NULL);
  smartstrap_unsubscribe();
}

int bp_get_status() {
  return init_state == INITIALIZED;
}

const char *bp_get_version() {
  return bp_firmware_version;
}

uint16_t bp_get_available_sensor_readings_mask() {
  return available_sensor_readings_mask;
}

uint16_t bp_get_available_processed_values_mask() {
  return available_processed_values_mask;
}

void bp_subscribe(BackpackHandlers handlers) {
  bp_handlers = handlers;
  if (handlers.on_sensor_readings)
    at_subscribe(&at_sensor_readings);
  if (handlers.on_processed_values)
    at_subscribe(&at_processed_values);
  if (handlers.on_onbody_event)
    at_read(&at_onbody_state);
  if (bp_get_status())
    timer_resume();
}

void bp_init_attribute(struct BackpackAttribute *attribute,
                       SmartstrapServiceId service,
                       uint16_t flags, size_t len,
                       const char *desc, BackpackAttributeHandler handler) {
  SmartstrapAttributeId attr_id = (SmartstrapAttributeId) flags;
  at_init(attribute, service, attr_id, len, desc, handler);
}

void bp_subscribe_attribute(struct BackpackAttribute *at) {
  at_subscribe(at);
}

void bp_set_polling_interval(uint32_t interval_ms) {
  polling_interval_ms = interval_ms;
}

void bp_unsubscribe() {
  if (open_reads)
    DBG("Unsubscribing (%d open reads)", open_reads);
  timer_suspend();
  at_unsubscribe_all();
  bp_handlers = (BackpackHandlers) {
    .availability_did_change = NULL
  };
  log_interrupt_handler = NULL;
  polling_interval_ms = DEFAULT_POLL_INTERVAL_MS;
}

time_t bp_log_remaining() {
  if (log_status >= STATUS_LOG_CLEARED)
    return 0;
  time_t remaining = log_clear_time_end - time(NULL);
  if (remaining <= 0) {
    log_status = STATUS_LOG_CLEARED;
    remaining = 0;
    DBG("Log clearing completed");
  }
  return remaining;
}

void cancel_log_watchdog() {
  if (log_watchdog_timer) {
    app_timer_cancel(log_watchdog_timer);
    log_watchdog_timer = NULL;
  }
}

void schedule_log_watchdog() {
  if (log_watchdog_timer)
    cancel_log_watchdog();
  log_watchdog_timer = app_timer_register(LOGGER_CHECK_INTERVAL_MS,
                                          log_watchdog_timer_fired, NULL);
}

void check_log_state() {
  at_read(&at_logger_state);
}

void log_watchdog_timer_fired() {
  log_watchdog_timer = NULL;
  check_log_state();
  schedule_log_watchdog();
}

enum bp_log_status bp_log_get_status() {
  if (log_status == STATUS_LOG_CLEARING)
    bp_log_remaining();
  return log_status;
}

uint32_t bp_get_logged_values_mask() {
  return logged_values_mask;
}

time_t bp_log_clear() {
  cancel_log_watchdog();

  if (log_status == STATUS_LOG_CLEARED)
    return 0;
  if (log_status == STATUS_LOG_CLEARING)
    return bp_log_remaining();

  log_status = STATUS_LOG_CLEARING;
  log_clear_time_end = time(NULL) + BP_LOG_CLEAR_TIME;

  uint8_t *buf;
  size_t buflen;
  smartstrap_attribute_begin_write(at_logger_clear.attribute, &buf, &buflen);
  smartstrap_attribute_end_write(at_logger_clear.attribute, ATTR_LOGGER_CLEAR_LEN, false);
  DBG("Log clearing started");
  return BP_LOG_CLEAR_TIME;
}

static void bp_log_resume() {
  log_status = STATUS_LOG_STARTED;
  uint8_t *buf;
  size_t buflen;
  smartstrap_attribute_begin_write(at_logger_resume.attribute, &buf, &buflen);
  smartstrap_attribute_end_write(at_logger_resume.attribute,
                                 ATTR_LOGGER_RESUME_LEN, false);
  DBG("Logging resumed");
}

void bp_log_start() {
  schedule_log_watchdog();

  bp_log_remaining();
  if (log_status == STATUS_LOG_STOPPED) {
    bp_log_resume();
    return;
  }
  if (log_status != STATUS_LOG_CLEARED)
    return;
  struct log_start_msg *start_msg;
  size_t buflen;
  smartstrap_attribute_begin_write(at_logger_start.attribute,
                                   (uint8_t **)&start_msg, &buflen);
  if (buflen < sizeof(struct log_start_msg)) {
    ERR("Buffer for log start message too small");
    smartstrap_attribute_end_write(at_logger_start.attribute, 0, false);
    return;
  }
  log_status = STATUS_LOG_STARTED;
  time_t t;
  uint16_t ms = time_ms(&t, NULL);
  start_msg->start_time_ms = ((uint64_t) t) * 1000 + ms;
  start_msg->log_interval_ms = 100;
  start_msg->enabled_channels_mask = logged_values_mask;
  smartstrap_attribute_end_write(at_logger_start.attribute,
                                 sizeof(struct log_start_msg), false);
  DBG("Logging started with mask 0x%04x%04x", (uint16_t)(logged_values_mask >> 16),
                                              (uint16_t)(logged_values_mask & 0xffff));
}

void bp_log_stop() {
  if (log_status != STATUS_LOG_STARTED)
    return;
  log_status = STATUS_LOG_STOPPED;
  uint8_t *buf;
  size_t buflen = 1;
  smartstrap_attribute_begin_write(at_logger_pause.attribute, &buf, &buflen);
  smartstrap_attribute_end_write(at_logger_pause.attribute,
                                 ATTR_LOGGER_PAUSE_LEN, false);
  DBG("Logging stopped");
}

void bp_set_log_interrupt_handler(LogInterruptHandler handler) {
  log_interrupt_handler = handler;
}
