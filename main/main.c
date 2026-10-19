#include "../components/vl530x/platform/inc/vl53l0x_i2c_platform.h"
#include "driver/gpio.h"
#include "driver/i2c_master.h"
#include "driver/i2c_types.h"
#include "esp_err.h"
#include "vl53l0x_api.h"
#include "vl53l0x_platform.h"
#include <stdio.h>
#include <uchar.h>

void app_main(void) {
  i2c_master_bus_handle_t bus_handle;
  i2c_master_dev_handle_t dev_handle = NULL;
  VL53L0X_Dev_t MyDevice;
  VL53L0X_Dev_t *pMyDevice = &MyDevice;
  VL53L0X_Version_t *pVersion = NULL;

  VL53L0X_DeviceInfo_t info;

  i2c_master_bus_config_t i2c_mst_config = {
      .clk_source = I2C_CLK_SRC_DEFAULT,
      .i2c_port = I2C_NUM_0,
      .scl_io_num = GPIO_NUM_22,
      .sda_io_num = GPIO_NUM_21,
      .glitch_ignore_cnt = 7,
      .flags.enable_internal_pullup = true,
  };

  ESP_ERROR_CHECK(i2c_new_master_bus(&i2c_mst_config, &bus_handle));
  esp_err_t err = i2c_master_probe(bus_handle, 0x29, 1000);
  if (err == ESP_OK) {
    ESP_ERROR_CHECK(
        VL53L0X_comms_initialise(I2C, 400, bus_handle, &dev_handle));
  }
  err = i2c_master_probe(bus_handle, 0x29, 1000);
  printf("Resultado 0x29: %s (%d)\n", esp_err_to_name(err), err);

  pMyDevice->handle = dev_handle;
  pMyDevice->I2cDevAddr = 0x52;
  pMyDevice->comms_type = I2C;
  err = VL53L0X_StaticInit(pMyDevice);
  if (err == ESP_OK) {
    VL53L0X_State *pPalState = NULL;
    err = VL53L0X_GetPalState(pMyDevice, pPalState);
    printf("STATE = %u", *pPalState);
  }

  if (err == ESP_OK) {
    err = VL53L0X_DataInit(pMyDevice);
  }

  if (err == ESP_OK) {

    VL53L0X_GetDeviceInfo(pMyDevice, &info);
  }
  printf("hola\n");
  printf("Info: %s \n", info.Name);
}
