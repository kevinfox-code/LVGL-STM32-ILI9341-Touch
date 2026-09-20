#include "bme68xController.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static uint32_t tick, delays, starts, reads;
static int8_t start_result, read_result;
static HAL_StatusTypeDef bus_result;
static I2C_HandleTypeDef bus;
static BME68x_HandleTypeDef sensor;
uint32_t HAL_GetTick(void) { return tick; }
void HAL_Delay(uint32_t ms) { delays += ms; }
HAL_StatusTypeDef HAL_I2C_Mem_Read(I2C_HandleTypeDef *h, uint16_t address, uint16_t reg,
                                 uint16_t size, uint8_t *data, uint16_t len, uint32_t timeout)
{
    (void)reg; (void)size; (void)data; (void)len;
    assert(h == &bus && address == 0xee && timeout == 100); return bus_result;
}
HAL_StatusTypeDef HAL_I2C_Mem_Write(I2C_HandleTypeDef *h, uint16_t address, uint16_t reg,
                                  uint16_t size, uint8_t *data, uint16_t len, uint32_t timeout)
{ return HAL_I2C_Mem_Read(h,address,reg,size,data,len,timeout); }
int8_t bme68x_init(struct bme68x_dev *dev) { (void)dev; return BME68X_OK; }
int8_t bme68x_get_conf(struct bme68x_conf *conf, struct bme68x_dev *dev)
{ (void)dev; memset(conf,0,sizeof(*conf)); return BME68X_OK; }
int8_t bme68x_set_conf(struct bme68x_conf *conf, struct bme68x_dev *dev)
{ (void)conf; (void)dev; return BME68X_OK; }
int8_t bme68x_set_heatr_conf(uint8_t mode, const struct bme68x_heatr_conf *conf, struct bme68x_dev *dev)
{ (void)mode; (void)conf; (void)dev; return BME68X_OK; }
int8_t bme68x_set_op_mode(uint8_t mode, struct bme68x_dev *dev)
{ (void)dev; assert(mode == BME68X_FORCED_MODE); ++starts; return start_result; }
uint32_t bme68x_get_meas_dur(uint8_t mode, struct bme68x_conf *conf, struct bme68x_dev *dev)
{ (void)mode; (void)conf; (void)dev; return 20500; }
int8_t bme68x_get_data(uint8_t mode, struct bme68x_data *data, uint8_t *fields, struct bme68x_dev *dev)
{ (void)mode; (void)dev; ++reads; *fields = read_result == BME68X_OK; data->temperature = 25; return read_result; }
static void reset(void)
{
    tick = delays = starts = reads = 0; start_result = read_result = BME68X_OK; bus_result = HAL_OK;
    assert(BME68x_Init(&sensor,&bus,BME68X_I2C_ADDR_HIGH) == BME68X_OK);
    assert(BME68x_Config(&sensor,2,2,1,3,320,150) == BME68X_OK);
}
static void test_sensor_conversion_keeps_foreground_responsive(void)
{
    // Arrange
    reset(); struct bme68x_data data; uint8_t fields = 99;
    tick = UINT32_MAX - 100;
    // Act
    int8_t result = BME68x_ReadData(&sensor,&data,&fields);
    // Assert
    assert(result == BME68X_W_NO_NEW_DATA && fields == 0 && starts == 1 && delays == 0);
    tick += 170;
    assert(BME68x_ReadData(&sensor,&data,&fields) == BME68X_W_NO_NEW_DATA && reads == 0);
    ++tick;
    assert(BME68x_ReadData(&sensor,&data,&fields) == BME68X_OK && fields == 1 && reads == 1);
    assert(!sensor.measurement_pending && delays == 0);
}
static void test_sensor_start_failure_can_retry(void)
{
    // Arrange
    reset(); start_result = BME68X_E_COM_FAIL; struct bme68x_data data; uint8_t fields = 99;
    // Act
    int8_t result = BME68x_ReadData(&sensor,&data,&fields);
    // Assert
    assert(result == BME68X_E_COM_FAIL && fields == 0 && !sensor.measurement_pending);
    start_result = BME68X_OK;
    assert(BME68x_ReadData(&sensor,&data,&fields) == BME68X_W_NO_NEW_DATA);
}
static void test_sensor_bus_errors_are_bounded(void)
{
    for (int error = HAL_ERROR; error <= HAL_TIMEOUT; ++error) {
        // Arrange
        reset(); bus_result = (HAL_StatusTypeDef)error; uint8_t value = 0;
        // Act
        int8_t result = sensor.dev.read(0,&value,1,sensor.dev.intf_ptr);
        // Assert
        assert(result == BME68X_E_COM_FAIL);
        assert(sensor.dev.write(0,&value,1,sensor.dev.intf_ptr) == BME68X_E_COM_FAIL);
    }
}
static void test_sensor_read_failure_and_invalid_output(void)
{
    // Arrange
    reset(); struct bme68x_data data; uint8_t fields = 99;
    assert(BME68x_ReadData(&sensor,&data,&fields) == BME68X_W_NO_NEW_DATA);
    tick = 171; read_result = BME68X_E_COM_FAIL;
    // Act
    int8_t result = BME68x_ReadData(&sensor,&data,&fields);
    // Assert
    assert(result == BME68X_E_COM_FAIL && fields == 0 && !sensor.measurement_pending);
    assert(BME68x_ReadData(&sensor,&data,NULL) == BME68X_E_NULL_PTR);
    assert(BME68x_ReadData(NULL,&data,&fields) == BME68X_E_NULL_PTR && fields == 0);
}
int main(void)
{
    test_sensor_conversion_keeps_foreground_responsive(); test_sensor_start_failure_can_retry();
    test_sensor_bus_errors_are_bounded(); test_sensor_read_failure_and_invalid_output();
    puts("PASS: BME680 nonblocking conversion, tick wrap and bus failures");
}
