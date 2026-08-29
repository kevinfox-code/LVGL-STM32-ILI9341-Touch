/**
 * XPT2046.c ― minimal polling driver
 * tuned for 12‑bit transfers on SPI, suitable for LVGL
 */
#include "XPT2046.h"
#include <stdio.h>          /* comment‑out after tuning */

/* ---------- command bytes ------------------------------------------- */
#define CMD_X   0xD0
#define CMD_Y   0x90
#define CMD_Z1  0xB0
#define CMD_Z2  0xC0

/* ---------- helpers -------------------------------------------------- */
#define CS_LOW()   HAL_GPIO_WritePin(XPT2046_CS_PORT, XPT2046_CS_PIN, GPIO_PIN_RESET)
#define CS_HIGH()  HAL_GPIO_WritePin(XPT2046_CS_PORT, XPT2046_CS_PIN, GPIO_PIN_SET)

#define PRESS_THRES   100 

static SPI_HandleTypeDef *xpt_spi = NULL; 


/* -------------------------------------------------------------------- */
static uint16_t spi_xfer(uint8_t cmd)
{
    uint8_t tx[3] = { cmd, 0, 0 };
    uint8_t rx[3] = { 0 };

    CS_LOW();
    HAL_SPI_TransmitReceive(xpt_spi, tx, rx, 3, HAL_MAX_DELAY);
    CS_HIGH();

    return ((rx[1] << 8) | rx[2]) >> 4;     /* 12‑bit sample */
}

static uint16_t read_pressure(void)
{
    uint16_t z1 = spi_xfer(CMD_Z1);
    uint16_t z2 = spi_xfer(CMD_Z2);
    return (z1 + 4095) - z2;                /* datasheet formula */
}

/* ---------- public API ---------------------------------------------- */
void XPT2046_Init(SPI_HandleTypeDef *spi)
{
    xpt_spi = spi;
    CS_HIGH();                              /* release bus       */
}

bool XPT2046_TouchDetected(void)
{
#ifdef XPT2046_IRQ_PIN
    return HAL_GPIO_ReadPin(XPT2046_IRQ_PORT, XPT2046_IRQ_PIN) == GPIO_PIN_RESET;
#else
    uint16_t z = read_pressure();
    /* printf("Z=%u\r\n", z); */            /* enable while tuning */
    return z > PRESS_THRES;
#endif
}

bool XPT2046_GetTouch(uint16_t *x, uint16_t *y)
{
    if(!XPT2046_TouchDetected()) return false;

    uint16_t x1 = spi_xfer(CMD_X);
    uint16_t y1 = spi_xfer(CMD_Y);
    uint16_t x2 = spi_xfer(CMD_X);
    uint16_t y2 = spi_xfer(CMD_Y);

    *x = (x1 + x2) >> 1;
    *y = (y1 + y2) >> 1;

    return true;
}
