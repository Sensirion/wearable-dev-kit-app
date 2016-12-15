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

#ifndef BACKPACK_H
#define BACKPACK_H

#include <pebble.h>
#define DOT(a, b, c) a ## . ## b ## . ## c
#define __STR(t) #t
#define __VERSION(maj, min, pat) DOT(maj, min, pat)
#define VERSIONSTR(t) __STR(t)

#define BACKPACK_LIB_MAJOR 1
#define BACKPACK_LIB_MINOR 0
#define BACKPACK_LIB_PATCH 0
#define BACKPACK_LIB_VERSION VERSIONSTR(__VERSION(BACKPACK_LIB_MAJOR, BACKPACK_LIB_MINOR, BACKPACK_LIB_PATCH))

#define BACKPACK_TIMEOUT 200
#define DEFAULT_POLL_INTERVAL_MS 500
#define DESTROY_RETRY_INTERVAL_MS 10

static const size_t ATTR_EVENT_LEN = sizeof(int32_t);

/* Services */
static const SmartstrapServiceId SERVICE_SENSOR_READINGS    = 0x1001;
static const SmartstrapServiceId SERVICE_PROCESSED_VALUES   = 0x1002;
static const SmartstrapServiceId SERVICE_LOGGER             = 0x1003;
static const SmartstrapServiceId SERVICE_SYSTEM             = 0x1004;

/* Sensor Readings Service Attributes */
static const SmartstrapAttributeId ATTR_SENSOR_READINGS_TEMPERATURE = 1<<0;
static const size_t ATTR_SENSOR_READINGS_TEMPERATURE_LEN = sizeof(int32_t);

static const SmartstrapAttributeId ATTR_SENSOR_READINGS_HUMIDITY    = 1<<1;
static const size_t ATTR_SENSOR_READINGS_HUMIDITY_LEN = sizeof(int32_t);

static const SmartstrapAttributeId ATTR_SENSOR_READINGS_SKIN_TEMPERATURE = 1<<2;
static const size_t ATTR_SENSOR_READINGS_SKIN_TEMPERATURE_LEN = sizeof(int32_t);

static const SmartstrapAttributeId ATTR_SENSOR_READINGS_SKIN_HUMIDITY = 1<<3;
static const size_t ATTR_SENSOR_READINGS_SKIN_HUMIDITY_LEN = sizeof(int32_t);

static const SmartstrapAttributeId ATTR_SENSOR_READINGS_RESERVED0    = 1<<4;
static const SmartstrapAttributeId ATTR_SENSOR_READINGS_RESERVED1    = 1<<5;
static const size_t ATTR_SENSOR_READINGS_RESERVED_LEN = sizeof(uint32_t);

static const SmartstrapAttributeId ATTR_SENSOR_READINGS_ACCEL_X     = 1<<8;
static const SmartstrapAttributeId ATTR_SENSOR_READINGS_ACCEL_Y     = 1<<9;
static const SmartstrapAttributeId ATTR_SENSOR_READINGS_ACCEL_Z     = 1<<10;
static const size_t ATTR_SENSOR_READINGS_ACCEL_LEN = sizeof(int16_t);

static const SmartstrapAttributeId ATTR_SENSOR_READINGS_GYRO_X      = 1<<11;
static const SmartstrapAttributeId ATTR_SENSOR_READINGS_GYRO_Y      = 1<<12;
static const SmartstrapAttributeId ATTR_SENSOR_READINGS_GYRO_Z      = 1<<13;
static const size_t ATTR_SENSOR_READINGS_GYRO_LEN = sizeof(int16_t);

static const SmartstrapAttributeId ATTR_SENSOR_READINGS_MPU6500_TEMPERATURE = 1<<14;
static const size_t ATTR_SENSOR_READINGS_MPU6500_TEMPERATURE_LEN = sizeof(int16_t);

/* Processed Values Service Attributes */
static const SmartstrapAttributeId ATTR_PROCESSED_VALUES_SKIN_TEMPERATURE = 1<<0;
static const size_t ATTR_PROCESSED_VALUES_SKIN_TEMPERATURE_LEN = sizeof(float);

static const SmartstrapAttributeId ATTR_PROCESSED_VALUES_APPARENT_TEMPERATURE = 1<<1;
static const size_t ATTR_PROCESSED_VALUES_APPARENT_TEMPERATURE_LEN = sizeof(float);

static const SmartstrapAttributeId ATTR_PROCESSED_VALUES_FEELLIKE_TEMPERATURE = 1<<2;
static const size_t ATTR_PROCESSED_VALUES_FEELLIKE_TEMPERATURE_LEN = sizeof(float);

static const SmartstrapAttributeId ATTR_PROCESSED_VALUES_HUMIDEX = 1<<3;
static const size_t ATTR_PROCESSED_VALUES_HUMIDEX_LEN = sizeof(float);

static const SmartstrapAttributeId ATTR_PROCESSED_VALUES_TEMPERATURE_COMPENSATION_MODE = 1<<4;
static const size_t ATTR_PROCESSED_VALUES_TEMPERATURE_COMPENSATION_MODE_LEN = sizeof(uint8_t);

static const SmartstrapAttributeId ATTR_PROCESSED_VALUES_TRANSPIRATION = 1<<5;
static const size_t ATTR_PROCESSED_VALUES_TRANSPIRATION_LEN = sizeof(float);

static const SmartstrapAttributeId ATTR_PROCESSED_VALUES_ONBODY_STATE = 1<<6;
static const size_t ATTR_PROCESSED_VALUES_ONBODY_STATE_LEN = sizeof(uint8_t);

static const SmartstrapAttributeId ATTR_PROCESSED_VALUES_AIRTOUCH_START_EVENT = 0x8001;
static const SmartstrapAttributeId ATTR_PROCESSED_VALUES_AIRTOUCH_STOP_EVENT = 0x8002;

static const SmartstrapAttributeId ATTR_TEMPERATURE_COMPENSATION_MODE = 0x8003;
static const size_t ATTR_TEMPERATURE_COMPENSATION_MODE_LEN = 2 * sizeof(uint8_t);

static const SmartstrapAttributeId ATTR_PROCESSED_VALUES_ONBODY_EVENT = 0x8004;
static const SmartstrapAttributeId ATTR_PROCESSED_VALUES_OFFBODY_EVENT = 0x8005;

/* Logger Service Attributes */
static const SmartstrapAttributeId ATTR_LOGGER_CLEAR    = 0x0001;
static const size_t ATTR_LOGGER_CLEAR_LEN = 1;
static const SmartstrapAttributeId ATTR_LOGGER_START    = 0x0002;
static const size_t ATTR_LOGGER_START_LEN = 16;
static const SmartstrapAttributeId ATTR_LOGGER_PAUSE    = 0x0003;
static const size_t ATTR_LOGGER_PAUSE_LEN = 1;
static const SmartstrapAttributeId ATTR_LOGGER_RESUME   = 0x0004;
static const size_t ATTR_LOGGER_RESUME_LEN = 1;
static const SmartstrapAttributeId ATTR_LOGGER_ENTRIES  = 0x0005;
static const SmartstrapAttributeId ATTR_LOGGER_STATE    = 0x0006;
static const size_t ATTR_LOGGER_STATE_LEN = sizeof(uint8_t);

/* System Service Attributes */
static const SmartstrapAttributeId ATTR_SYSTEM_PLUGGED    = 0x0002;
static const size_t ATTR_SYSTEM_PLUGGED_LEN = 1;
static const SmartstrapAttributeId ATTR_SYSTEM_UNPLUGGED = 0x0003;
static const size_t ATTR_SYSTEM_UNPLUGGED_LEN = 1;
static const SmartstrapAttributeId ATTR_SYSTEM_VERSION    = 0x0004;
static const size_t ATTR_SYSTEM_VERSION_MAX_LEN = 60;
static const SmartstrapAttributeId ATTR_SYSTEM_AVAILABLE_SENSOR_READINGS_MASK = 0x0005;
static const size_t ATTR_SYSTEM_AVAILABLE_SENSOR_READINGS_MASK_LEN = 2;
static const SmartstrapAttributeId ATTR_SYSTEM_AVAILABLE_PROCESSED_VALUES_MASK = 0x0006;
static const size_t ATTR_SYSTEM_AVAILABLE_PROCESSED_VALUES_MASK_LEN = 2;

typedef struct {
  /** Pebble service availability passthrough */
  void (*availability_did_change)(SmartstrapServiceId service_id, bool is_available);
  /** Backpack is connected and its version and capabilities are known */
  void (*on_connection_state_changed)(bool is_connected);
  /** New sensor readings are available */
  void (*on_sensor_readings)(int32_t t_c, int32_t rh, int32_t t_skin, int16_t reserved0, int16_t reserved1);
  /** New processed values are available */
  void (*on_processed_values)(float t_skin, float t_fl, float t_apparent, float t_humidex);
  /** An airtouch event is triggered */
  void (*on_airtouch_event)(bool start);
  /** The subscription triggers an initial event to report the state */
  void (*on_onbody_event)(bool onbody);
} BackpackHandlers;

typedef void (*BackpackAttributeHandler)(const uint8_t *data, size_t length,
                                         SmartstrapAttributeId id);

typedef void (*TemperatureCompensationModeHandler)(uint8_t currentMode,
                                                   uint8_t numbereOfmodes);

/** Opaque struct for subscribed backpack attributes */
struct BackpackAttribute {
  SmartstrapAttribute *attribute;
  const char *desc;
  BackpackAttributeHandler handler;
  int id;
  volatile bool open_read;
};

/** Initialize backpack module */
int bp_init();
/** Get backpack status: 0 if disconnected, 1 if connected */
int bp_get_status();
/** Get backpack version string */
const char *bp_get_version();
/** Get a bitmask of available sensor readings */
uint16_t bp_get_available_sensor_readings_mask();
/** Get a bitmask of available processed values */
uint16_t bp_get_available_processed_values_mask();

void bp_subscribe(BackpackHandlers handlers);
/**
 * Create a custom backpack attribute.
 * Destroy the attribute with bp_destroy_attribute.
 */
void bp_init_attribute(struct BackpackAttribute *attribute,
                       SmartstrapServiceId service,
                       uint16_t flags, size_t len,
                       const char *desc, BackpackAttributeHandler handler);

/** Start polling a custom attribute */
void bp_subscribe_attribute(struct BackpackAttribute *at);

void bp_set_temperature_compensation_mode(uint8_t mode,
                                          TemperatureCompensationModeHandler handler);

/** Set polling interval in ms */
void bp_set_polling_interval(uint32_t interval_ms);
/** Unsubscribe from all backpack event handlers and reset polling interval */
void bp_unsubscribe();
/**
 * Try cleanup of backpack attribute.
 * Keep trying each event tick until success: The recommended interval is
 * DESTROY_RETRY_INTERVAL_MS.
 */
bool bp_destroy_attribute(struct BackpackAttribute *at);
void bp_deinit();
/**
 * Read a value of known type from buffer at given offset
 * An error is printed when trying to read past the end of the buffer.
 *
 * @param data start of read buffer
 * @param len length of read buffer
 * @param offset offset from start of read buffer
 * @param result [out] output buffer
 * @param type_len type length
 * @param desc (for debugging) description of value to read
 * @return success value (false when attempting to read beyond buffer)
 */
bool bp_readval(const uint8_t *data, size_t len, int *offset, void *result,
                size_t type_len, const char *desc);
/* Logging functions */
/** Time in s (with safety margin) to erase the log */
#define BP_LOG_CLEAR_TIME 70

enum bp_log_status {
  /*
   * REORDERING WARNING:
   * Code makes use of relative comparison operators
   */
  STATUS_LOG_DIRTY,
  STATUS_LOG_CLEARING,
  STATUS_LOG_CLEARED,
  STATUS_LOG_STARTED,
  STATUS_LOG_STOPPED
};

typedef void (*LogInterruptHandler)();

/** Get the current log status */
enum bp_log_status bp_log_get_status();
/**
 * Clear the current log if dirty.
 * This method may be called to get the remaining clear time.
 * Returns the remaining time to clear the log, 0 when cleared.
 */
time_t bp_log_clear();
/** Start logging */
void bp_log_start();
/** Stop logging */
void bp_log_stop();
/**
 * Returns a bitmask of logged values
 * The lower 16 bit correspond to the sensor readings while
 * the upper 16 bit correspond to the processed values.
 */
uint32_t bp_get_logged_values_mask();
/** Get remaining time until log cleared */
time_t bp_log_remaining();
/** Register a handler to be called on unwanted logging interruptions */
void bp_set_log_interrupt_handler(LogInterruptHandler handler);

#endif /* BACKPACK_H */
