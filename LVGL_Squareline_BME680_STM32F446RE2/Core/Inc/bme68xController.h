// ---------- BME68xController.h ----------
#ifndef BME68XCONTROLLER_H
#define BME68XCONTROLLER_H

#include "bme68x.h"
#include "stm32f4xx_hal.h"

// Handle type for BME68x peripheral
typedef struct {
    I2C_HandleTypeDef *hi2c;    // HAL I2C handle
    uint8_t address;            // 7-bit I2C address (0x76 or 0x77)
    struct bme68x_dev dev;      // Bosch sensor device struct
    struct bme68x_conf conf;    // Current sensor configuration
    struct bme68x_heatr_conf heatr_conf; // Heater configuration
} BME68x_HandleTypeDef;


int8_t BME68x_Init(BME68x_HandleTypeDef *h, I2C_HandleTypeDef *hi2c, uint8_t address);

/**
 * @brief Configure oversampling, IIR filter, and heater
 * @param h            Pointer to BME68x handle
 * @param os_temp      Temperature oversampling (BME68X_OS_x)
 * @param os_pres      Pressure oversampling
 * @param os_hum       Humidity oversampling
 * @param filter       IIR filter size
 * @param heatr_temp   Heater temperature (°C)
 * @param heatr_dur    Heater duration (ms)
 * @return BME68X_OK on success, error code on failure
 */
int8_t BME68x_Config(BME68x_HandleTypeDef *h,
                     uint8_t os_temp, uint8_t os_pres, uint8_t os_hum,
                     uint8_t filter,
                     uint16_t heatr_temp, uint16_t heatr_dur);

/**
 * @brief Trigger a forced-mode measurement and read data
 * @param h         Pointer to BME68x handle
 * @param data      Output data struct (single measurement)
 * @param n_fields  Number of valid data fields (0 or 1)
 * @return BME68X_OK on success, error code on failure
 */
int8_t BME68x_ReadData(BME68x_HandleTypeDef *h,
                       struct bme68x_data *data, uint8_t *n_fields);

/**
 * @brief Compute measurement duration in microseconds for given mode
 * @param h         Pointer to BME68x handle
 * @param op_mode   Operation mode (e.g., BME68X_FORCED_MODE)
 * @return Duration in microseconds
 */
uint32_t BME68x_CalcMeasurementDuration(BME68x_HandleTypeDef *h,
                                        uint8_t op_mode);

#endif // BME68XCONTROLLER_H
