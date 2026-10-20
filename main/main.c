#include "../components/vl530x/platform/inc/vl53l0x_i2c_platform.h"
#include "driver/gpio.h"
#include "driver/i2c_master.h"
#include "driver/i2c_types.h"
#include "esp_err.h"
#include "freertos/idf_additions.h"
#include "portmacro.h"
#include "vl53l0x_api.h"
#include "vl53l0x_platform.h"
#include <stdio.h>
#include <uchar.h>

void print_range_status(
    VL53L0X_RangingMeasurementData_t *pRangingMeasurementData) {
  char buf[VL53L0X_MAX_STRING_LENGTH];
  uint8_t RangeStatus;

  /*
   * New Range status: data is valid when pRangingMeasurementData->RangeStatus =
   * 0
   */

  RangeStatus = pRangingMeasurementData->RangeStatus;

  VL53L0X_GetRangeStatusString(RangeStatus, buf);
  printf("Range status: %i : %s\n", RangeStatus, buf);
}

void ranging_test(VL53L0X_Dev_t *pMyDevice) {
  esp_err_t status;
  uint8_t pVhvSettings;
  uint8_t pPhaseCal;
  uint32_t refSpadCount;
  uint8_t isApertureSpads;
  VL53L0X_RangingMeasurementData_t pRangingMeasurementData;
  FixPoint1616_t pLimitCheckCurrent;
  VL53L0X_DeviceInfo_t info;

  printf("-----BEGINNING RANGING TEST-----\n");

  status = VL53L0X_StaticInit(pMyDevice);

  status = VL53L0X_GetDeviceInfo(pMyDevice, &info);

  printf("Status: %d\n", status);
  printf("Name: %s\n", info.Name);
  printf("Type: %s\n", info.Type);
  printf("ProductId: %s\n", info.ProductId);
  printf("Revision: %d.%d\n", info.ProductRevisionMajor,
         info.ProductRevisionMinor);

  if (status == ESP_OK) {
    printf("----INIT DONE----\n");
    status =
        VL53L0X_PerformRefCalibration(pMyDevice, &pVhvSettings, &pPhaseCal);
    printf("%d", status);
  }
  if (status == ESP_OK) {
    printf("----CALIBRATION DONE----\n");
    status = VL53L0X_PerformRefSpadManagement(pMyDevice, &refSpadCount,
                                              &isApertureSpads);
    printf("%d", status);
  }

  if (status == ESP_OK) {
    printf("----SPAD MANAGEMENT DONE----\n");
    VL53L0X_DeviceModes pDeviceMode;
    status = VL53L0X_GetDeviceMode(pMyDevice, &pDeviceMode);
    printf("%d", pDeviceMode);
    status =
        VL53L0X_SetDeviceMode(pMyDevice, VL53L0X_DEVICEMODE_SINGLE_RANGING);
  }

  if (status == VL53L0X_ERROR_NONE) {

    // no need to do this when we use VL53L0X_PerformSingleRangingMeasurement
    printf("Call of VL53L0X_SetDeviceMode\n");
    status = VL53L0X_SetDeviceMode(
        pMyDevice,
        VL53L0X_DEVICEMODE_SINGLE_RANGING); // Setup in single ranging mode
  }

  // Enable/Disable Sigma and Signal check
  if (status == VL53L0X_ERROR_NONE) {
    status = VL53L0X_SetLimitCheckEnable(
        pMyDevice, VL53L0X_CHECKENABLE_SIGMA_FINAL_RANGE, 1);
  }
  if (status == VL53L0X_ERROR_NONE) {
    status = VL53L0X_SetLimitCheckEnable(
        pMyDevice, VL53L0X_CHECKENABLE_SIGNAL_RATE_FINAL_RANGE, 1);
  }

  if (status == VL53L0X_ERROR_NONE) {
    status = VL53L0X_SetLimitCheckEnable(
        pMyDevice, VL53L0X_CHECKENABLE_RANGE_IGNORE_THRESHOLD, 1);
  }

  if (status == VL53L0X_ERROR_NONE) {
    status = VL53L0X_SetLimitCheckValue(
        pMyDevice, VL53L0X_CHECKENABLE_RANGE_IGNORE_THRESHOLD,
        (FixPoint1616_t)(1.5 * 0.023 * 65536));
  }

  if (status == ESP_OK) {
    for (int i = 0; i < 10; i++) {
      printf("Call of VL53L0X_PerformSingleRangingMeasurement\n");
      status = VL53L0X_PerformSingleRangingMeasurement(
          pMyDevice, &pRangingMeasurementData);

      printf("RANGING STATUS: %d\n", status);
      print_range_status(&pRangingMeasurementData);

      VL53L0X_GetLimitCheckCurrent(pMyDevice,
                                   VL53L0X_CHECKENABLE_RANGE_IGNORE_THRESHOLD,
                                   &pLimitCheckCurrent);

      printf("RANGE IGNORE THRESHOLD: %f\n\n",
             (float)pLimitCheckCurrent / 65536.0);

      if (status != ESP_OK)
        break;

      printf("Measured distance: %i cm\n\n",
             pRangingMeasurementData.RangeMilliMeter / 10);

      vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
  }
}

void app_main(void) {
  i2c_master_bus_handle_t bus_handle = {0};
  i2c_master_dev_handle_t dev_handle = {0};
  VL53L0X_Dev_t MyDevice;
  VL53L0X_Dev_t *pMyDevice = &MyDevice;

  VL53L0X_DeviceInfo_t info = {0};

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
  uint8_t model;
  err = i2c_master_probe(bus_handle, 0x29, 1000);
  printf("Resultado 0x29: %s (%d)\n", esp_err_to_name(err), err);

  pMyDevice->handle = dev_handle;
  pMyDevice->I2cDevAddr = 0x29;
  pMyDevice->comms_type = I2C;

  if (err == ESP_OK) {
    err = VL53L0X_DataInit(pMyDevice);
  }

  if (err == ESP_OK) {

    VL53L0X_GetDeviceInfo(pMyDevice, &info);
  }
  printf("Info: %s \n", info.Name);
  printf("Type: %s \n", info.Type);
  printf("MODEL: %s\n", info.ProductId);
  printf("revision: %d\n", info.ProductRevisionMajor);

  ranging_test(pMyDevice);
}
