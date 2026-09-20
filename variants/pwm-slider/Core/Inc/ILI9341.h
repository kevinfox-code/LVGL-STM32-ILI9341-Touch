/* Single foreground owner; SPI1 DMA callbacks are owned by this driver. */
#ifndef ILI9341_H
#define ILI9341_H
#include "stm32f4xx_hal.h"
#include <stdbool.h>
#ifdef __cplusplus
extern "C" {
#endif
#define ILI9341_SPI hspi1
#define ILI9341_DC_PORT GPIOB
#define ILI9341_DC_PIN GPIO_PIN_6
#define ILI9341_RESET_PORT GPIOC
#define ILI9341_RESET_PIN GPIO_PIN_7
#define ILI9341_CS_PORT GPIOA
#define ILI9341_CS_PIN GPIO_PIN_9
#define ILI9341_WIDTH 240
#define ILI9341_HEIGHT 320
HAL_StatusTypeDef ILI9341_Init(void);
HAL_StatusTypeDef ILI9341_FillScreen(uint16_t color);
HAL_StatusTypeDef ILI9341_DrawPixel(uint16_t x, uint16_t y, uint16_t color);
HAL_StatusTypeDef ILI9341_SetRotation(uint8_t rotation);
HAL_StatusTypeDef ILI9341_WriteCommand(uint8_t cmd);
HAL_StatusTypeDef ILI9341_WriteData(uint8_t data);
HAL_StatusTypeDef ILI9341_WriteData16(uint16_t data);
HAL_StatusTypeDef ILI9341_SetWindow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1);
/** Send RGB565 in wire byte order; larger blocking transfers are split safely. */
HAL_StatusTypeDef ILI9341_DrawBitmap(uint16_t w, uint16_t h, uint8_t *pixels);
/** Start <=65534 bytes; source must remain valid until WaitTransfer returns. */
HAL_StatusTypeDef ILI9341_DrawBitmapDMA(uint16_t w, uint16_t h, uint8_t *pixels);
/** Foreground only. Wait up to 100 ms, then abort DMA before releasing the source. */
HAL_StatusTypeDef ILI9341_WaitTransfer(void);
#ifdef __cplusplus
}
#endif
#endif
