/*
 * bme68xController.c
 *
 *  Created on: Jun 8, 2025
 *      Author: kevinfox
 *
 *  This module provides a lightweight HAL wrapper around the Bosch BME68x API,
 *  exposing init, config, and read routines for I2C-based BME68x sensors.
 *
 *  Usage:
 *    1. Include header in main
 *         #include "bme68xController.h"
 *
 *    2. Declare handle
 *         static BME68x_HandleTypeDef hbme;
 *
 *    3. Initialize in main()
 *         if (BME68x_Init(&hbme, &hi2c1, BME68X_I2C_ADDR_HIGH) != BME68X_OK) Error_Handler();
 *
 *    4. Configure over-sampling, I2R filter + heater
 *         if (BME68x_Config(&hbme,
 *                           BME68X_OS_4X, BME68X_OS_4X, BME68X_OS_2X,
 *                           BME68X_FILTER_SIZE_3,
 *                           320, 150) != BME68X_OK) Error_Handler();
 *
 *    5. in loop
 *         struct bme68x_data data;
 *         uint8_t n_fields;
 *	  if (BME68x_ReadData(&hbme, &data, &n_fields) == BME68X_OK && n_fields) {
 *		  printf("T: %.1f°C  H: %.1f%%  P: %.1fhPa  G: %luΩ\r\n",
 *		         data.temperature,
 *		         data.humidity,
 *		         data.pressure / 100.0f, //Pressure is in Pa, convert to hPa
 *		         (unsigned long)data.gas_resistance);
 */
#include "bme68xController.h"
#include <string.h>

// Internal I2C read callback
static int8_t bme68x_read(uint8_t reg_addr, uint8_t *reg_data, uint32_t len, void *intf_ptr) {
    BME68x_HandleTypeDef *h = (BME68x_HandleTypeDef*)intf_ptr;
    return (HAL_I2C_Mem_Read(h->hi2c,
                             (uint16_t)(h->address << 1),
                             reg_addr,
                             I2C_MEMADD_SIZE_8BIT,
                             reg_data,
                             len,
                             HAL_MAX_DELAY) == HAL_OK)
           ? BME68X_OK : BME68X_E_COM_FAIL;
}

// Internal I2C write callback
static int8_t bme68x_write(uint8_t reg_addr, const uint8_t *reg_data, uint32_t len, void *intf_ptr) {
    BME68x_HandleTypeDef *h = (BME68x_HandleTypeDef*)intf_ptr;
    return (HAL_I2C_Mem_Write(h->hi2c,
                              (uint16_t)(h->address << 1),
                              reg_addr,
                              I2C_MEMADD_SIZE_8BIT,
                              (uint8_t*)reg_data,
                              len,
                              HAL_MAX_DELAY) == HAL_OK)
           ? BME68X_OK : BME68X_E_COM_FAIL;
}

// Delay callback: converts microseconds to ms
static void bme68x_delay_us(uint32_t period_us, void *intf_ptr) {
    HAL_Delay((period_us + 999) / 1000);
}

int8_t BME68x_Init(BME68x_HandleTypeDef *h, I2C_HandleTypeDef *hi2c, uint8_t address) {
    memset(h, 0, sizeof(*h));
    h->hi2c    = hi2c;
    h->address = address;
    h->dev.intf     = BME68X_I2C_INTF;
    h->dev.intf_ptr = h;
    h->dev.read     = bme68x_read;
    h->dev.write    = bme68x_write;
    h->dev.delay_us = bme68x_delay_us;
    // set default ambient temperature for heater compensation (25°C)
    h->dev.amb_temp = 25;
    return bme68x_init(&h->dev);
}

int8_t BME68x_Config(BME68x_HandleTypeDef *h,
                     uint8_t os_temp, uint8_t os_pres, uint8_t os_hum,
                     uint8_t filter,
                     uint16_t heatr_temp, uint16_t heatr_dur) {
    int8_t rslt = bme68x_get_conf(&h->conf, &h->dev);
    if (rslt != BME68X_OK) return rslt;
    h->conf.os_temp = os_temp;
    h->conf.os_pres = os_pres;
    h->conf.os_hum  = os_hum;
    h->conf.filter  = filter;
    rslt = bme68x_set_conf(&h->conf, &h->dev);
    if (rslt != BME68X_OK) return rslt;
    memset(&h->heatr_conf, 0, sizeof(h->heatr_conf));
    h->heatr_conf.enable     = BME68X_ENABLE;
    h->heatr_conf.heatr_temp = heatr_temp;
    h->heatr_conf.heatr_dur  = heatr_dur;
    return bme68x_set_heatr_conf(BME68X_FORCED_MODE, &h->heatr_conf, &h->dev);
}

int8_t BME68x_ReadData(BME68x_HandleTypeDef *h,
                       struct bme68x_data *data, uint8_t *n_fields) {
    int8_t rslt;
    uint8_t fields = 0;
    rslt = bme68x_set_op_mode(BME68X_FORCED_MODE, &h->dev);
    if (rslt != BME68X_OK) { *n_fields = 0; return rslt; }
    uint32_t wait_us = bme68x_get_meas_dur(BME68X_FORCED_MODE, &h->conf, &h->dev)
                     + (h->heatr_conf.heatr_dur * 1000);
    HAL_Delay((wait_us + 999) / 1000);
    rslt = bme68x_get_data(BME68X_FORCED_MODE, data, &fields, &h->dev);
    if (rslt != BME68X_OK || fields == 0) { *n_fields = 0; return rslt; }
    *n_fields = fields;
    return BME68X_OK;
}

uint32_t BME68x_CalcMeasurementDuration(BME68x_HandleTypeDef *h, uint8_t op_mode) {
    return bme68x_get_meas_dur(op_mode, &h->conf, &h->dev);
}
