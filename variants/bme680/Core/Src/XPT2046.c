#include "XPT2046.h"

#define CMD_X 0xD0
#define CMD_Y 0x90
#define CMD_Z1 0xB0
#define CMD_Z2 0xC0
#define PRESS_THRESHOLD 200U
#define SPI_TIMEOUT_MS 10U
static SPI_HandleTypeDef *xpt_spi;

static bool read_sample(uint8_t command, uint16_t *sample)
{
    uint8_t tx[3] = {command, 0, 0}, rx[3] = {0};
    if (!xpt_spi) return false;
    HAL_GPIO_WritePin(XPT2046_CS_PORT, XPT2046_CS_PIN, GPIO_PIN_RESET);
    HAL_StatusTypeDef result = HAL_SPI_TransmitReceive(xpt_spi, tx, rx, 3, SPI_TIMEOUT_MS);
    HAL_GPIO_WritePin(XPT2046_CS_PORT, XPT2046_CS_PIN, GPIO_PIN_SET);
    if (result != HAL_OK) return false;
    /* 24-clock transfer: one null bit, 12 data bits, three trailing bits. */
    *sample = ((((uint16_t)rx[1] << 8) | rx[2]) >> 3) & 0x0fff;
    return true;
}

void XPT2046_Init(SPI_HandleTypeDef *spi)
{
    xpt_spi = spi;
    HAL_GPIO_WritePin(XPT2046_CS_PORT, XPT2046_CS_PIN, GPIO_PIN_SET);
}

bool XPT2046_TouchDetected(void)
{
    uint16_t z1, z2;
    if (!read_sample(CMD_Z1, &z1) || !read_sample(CMD_Z2, &z2)) return false;
    return z1 != 0 && (uint32_t)z1 + 4095U - z2 > PRESS_THRESHOLD;
}

bool XPT2046_GetTouch(uint16_t *x, uint16_t *y)
{
    uint16_t x1, y1, x2, y2;
    if (!x || !y || !XPT2046_TouchDetected()) return false;
    if (!read_sample(CMD_X, &x1) || !read_sample(CMD_Y, &y1) ||
        !read_sample(CMD_X, &x2) || !read_sample(CMD_Y, &y2) ||
        !XPT2046_TouchDetected()) return false;
    *x = ((uint32_t)x1 + x2) / 2U;
    *y = ((uint32_t)y1 + y2) / 2U;
    return true;
}
