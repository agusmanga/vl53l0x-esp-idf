/*
 * COPYRIGHT (C) STMicroelectronics 2015. All rights reserved.
 *
 * This software is the confidential and proprietary information of
 * STMicroelectronics ("Confidential Information").  You shall not
 * disclose such Confidential Information and shall use it only in
 * accordance with the terms of the license agreement you entered into
 * with STMicroelectronics
 *
 * Programming Golden Rule: Keep it Simple!
 *
 */

/*!
 * \file   VL53L0X_platform.c
 * \brief  Code function defintions for Ewok Platform Layer
 *
 */

#include "driver/i2c_types.h"
#include "esp_err.h"
#include "hal/i2c_types.h"
#include "portmacro.h"
#include <stdint.h>
#include <stdio.h> // sprintf(), vsnprintf(), printf()

#ifdef _MSC_VER
#define snprintf _snprintf
#endif

#include "vl53l0x_def.h"
#include "vl53l0x_i2c_platform.h"

#include "driver/i2c_master.h"
#include "freertos/FreeRTOS.h"
#include "freertos/portmacro.h"

#include "freertos/task.h"

#include "vl53l0x_platform_log.h"

#ifdef VL53L0X_LOG_ENABLE
#define trace_print(level, ...)                                                \
  trace_print_module_function(TRACE_MODULE_PLATFORM, level,                    \
                              TRACE_FUNCTION_NONE, ##__VA_ARGS__)
#define trace_i2c(...)                                                         \
  trace_print_module_function(TRACE_MODULE_NONE, TRACE_LEVEL_NONE,             \
                              TRACE_FUNCTION_I2C, ##__VA_ARGS__)
#endif

#define MIN_COMMS_VERSION_MAJOR 1
#define MIN_COMMS_VERSION_MINOR 8
#define MIN_COMMS_VERSION_BUILD 1
#define MIN_COMMS_VERSION_REVISION 0

esp_err_t VL53L0X_comms_initialise(uint8_t comms_type, uint16_t comms_speed_khz,
                                   i2c_master_bus_handle_t bus_handle,
                                   i2c_master_dev_handle_t *dev_handle) {
  static const int32_t cmax_devices = 4;

  esp_err_t status = ESP_OK;
  int32_t argc = 1;

  i2c_device_config_t dev_config = {
      I2C_ADDR_BIT_LEN_7, 0x29, comms_speed_khz * 1000, 0, {1}};

  if (status == ESP_OK) {
    status = i2c_master_bus_add_device(bus_handle, &dev_config, dev_handle);
  }

  return status;
}

esp_err_t VL53L0X_comms_close(i2c_master_dev_handle_t handle) {
  esp_err_t status = ESP_OK;

  status = i2c_master_bus_rm_device(handle);

  if (status != ESP_OK) {
    printf("ESP ERROR: %c", *esp_err_to_name(status));
  }

  return status;
}

esp_err_t VL53L0X_write_multi(i2c_master_dev_handle_t handle, uint8_t index,
                              uint8_t *pdata, int32_t count) {
  esp_err_t status = ESP_OK;
#ifdef VL53L0X_LOG_ENABLE
  int32_t i = 0;
  char value_as_str[VL53L0X_MAX_STRING_LENGTH_PLT];
  char *pvalue_as_str;

  pvalue_as_str = value_as_str;

  for (i = 0; i < count; i++) {
    sprintf(pvalue_as_str, "%02X", *(pdata + i));

    pvalue_as_str += 2;
  }
  trace_i2c("Write reg : 0x%04X, Val : 0x%s\n", index, value_as_str);
#endif

  status = i2c_master_transmit(handle, pdata, count,
                               1000); // TODO: define ms for timeout

  if (status != ESP_OK) {
    printf("ESP ERROR: %c",
           *esp_err_to_name(status)); // TODO: Abstract error logging
  }

  return status;
}

esp_err_t VL53L0X_read_multi(i2c_master_dev_handle_t handle, uint8_t index,
                             uint8_t *pdata, int32_t count) {
  int32_t status = ESP_OK;

#ifdef VL53L0X_LOG_ENABLE
  int32_t i = 0;
  char value_as_str[VL53L0X_MAX_STRING_LENGTH_PLT];
  char *pvalue_as_str;
#endif

  // read 8-bit standard V2W8 write (not paging mode)
  status = i2c_master_receive(handle, pdata, count, 1000);

  if (status != ESP_OK) {
    printf("ESP ERROR: %c",
           *esp_err_to_name(status)); // TODO: Abstract error logging
  }

#ifdef VL53L0X_LOG_ENABLE
  // Build  value as string;
  pvalue_as_str = value_as_str;

  for (i = 0; i < count; i++) {
    sprintf(pvalue_as_str, "%02X", *(pdata + i));
    pvalue_as_str += 2;
  }

  trace_i2c("Read  reg : 0x%04X, Val : 0x%s\n", index, value_as_str);
#endif

  return status;
}

esp_err_t VL53L0X_write_byte(i2c_master_dev_handle_t handle, uint8_t index,
                             uint8_t data) {
  int32_t status = ESP_OK;
  const int32_t cbyte_count = 1;

  status = VL53L0X_write_multi(handle, index, &data, cbyte_count);

  return status;
}

esp_err_t VL53L0X_write_word(i2c_master_dev_handle_t handle, uint8_t index,
                             uint16_t data) {
  int32_t status = ESP_OK;

  uint8_t buffer[BYTES_PER_WORD];

  // Split 16-bit word into MS and LS uint8_t
  buffer[0] = (uint8_t)(data >> 8);
  buffer[1] = (uint8_t)(data & 0x00FF);

  status = VL53L0X_write_multi(handle, index, buffer, BYTES_PER_WORD);

  return status;
}

esp_err_t VL53L0X_write_dword(i2c_master_dev_handle_t handle, uint8_t index,
                              uint32_t data) {
  int32_t status = ESP_OK;
  uint8_t buffer[BYTES_PER_DWORD];

  // Split 32-bit word into MS ... LS bytes
  buffer[0] = (uint8_t)(data >> 24);
  buffer[1] = (uint8_t)((data & 0x00FF0000) >> 16);
  buffer[2] = (uint8_t)((data & 0x0000FF00) >> 8);
  buffer[3] = (uint8_t)(data & 0x000000FF);

  status = VL53L0X_write_multi(handle, index, buffer, BYTES_PER_DWORD);

  return status;
}

esp_err_t VL53L0X_read_byte(i2c_master_dev_handle_t handle, uint8_t index,
                            uint8_t *pdata) {
  int32_t status = ESP_OK;
  int32_t cbyte_count = 1;

  status = VL53L0X_read_multi(handle, index, pdata, cbyte_count);

  return status;
}

esp_err_t VL53L0X_read_word(i2c_master_dev_handle_t handle, uint8_t index,
                            uint16_t *pdata) {
  int32_t status = ESP_OK;
  uint8_t buffer[BYTES_PER_WORD];

  status = VL53L0X_read_multi(handle, index, buffer, BYTES_PER_WORD);
  *pdata = ((uint16_t)buffer[0] << 8) + (uint16_t)buffer[1];

  return status;
}

esp_err_t VL53L0X_read_dword(i2c_master_dev_handle_t handle, uint8_t index,
                             uint32_t *pdata) {
  int32_t status = ESP_OK;
  uint8_t buffer[BYTES_PER_DWORD];

  status = VL53L0X_read_multi(handle, index, buffer, BYTES_PER_DWORD);
  *pdata = ((uint32_t)buffer[0] << 24) + ((uint32_t)buffer[1] << 16) +
           ((uint32_t)buffer[2] << 8) + (uint32_t)buffer[3];

  return status;
}

esp_err_t VL53L0X_platform_wait_us(int32_t wait_us) {
  esp_err_t status = ESP_OK;
  float wait_ms = (float)wait_us / 1000.0f;
  const TickType_t xDelayMS = wait_ms / portTICK_PERIOD_MS;

  vTaskDelay(xDelayMS);

#ifdef VL53L0X_LOG_ENABLE
  trace_i2c("Wait us : %6d\n", wait_us);
#endif

  return status;
}

esp_err_t VL53L0X_wait_ms(int32_t wait_ms) {
  esp_err_t status = ESP_OK;

  const TickType_t xDelayMS = wait_ms / portTICK_PERIOD_MS;

  vTaskDelay(xDelayMS);

#ifdef VL53L0X_LOG_ENABLE
  trace_i2c("Wait ms : %6d\n", wait_ms);
#endif

  return status;
}

esp_err_t VL53L0X_set_gpio(uint8_t level) {
  int32_t status = ESP_OK;
  // status = VL53L0X_set_gpio_sv(level);
#ifdef VL53L0X_LOG_ENABLE
  trace_i2c("// Set GPIO = %d;\n", level);
#endif

  return status;
}

int32_t VL53L0X_get_gpio(uint8_t *plevel) {
  int32_t status = ESP_OK;
#ifdef VL53L0X_LOG_ENABLE
  trace_i2c("// Get GPIO = %d;\n", *plevel);
#endif
  return status;
}

int32_t VL53L0X_release_gpio(void) {
  int32_t status = ESP_OK;
#ifdef VL53L0X_LOG_ENABLE
  trace_i2c("// Releasing force on GPIO\n");
#endif
  return status;
}

int32_t VL53L0X_cycle_power(void) {
  int32_t status = ESP_OK;
#ifdef VL53L0X_LOG_ENABLE
  trace_i2c("// cycle sensor power\n");
#endif
  // status = RANGING_SENSOR_COMMS_Cycle_Sensor_Power();

  if (status != ESP_OK) {
    // RANGING_SENSOR_COMMS_Get_Error_Text(debug_string);
  }

  return status;
}

int32_t VL53L0X_get_timer_frequency(int32_t *ptimer_freq_hz) {
  *ptimer_freq_hz = 0;
  return 1;
}

int32_t VL53L0X_get_timer_value(int32_t *ptimer_count) {
  *ptimer_count = 0;
  return 1;
}
