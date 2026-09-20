/* XPT2046 pressure polling; PENIRQ wiring is not required.
 * (c) Kevin Fox 2025
 */
#ifndef XPT2046_H_
#define XPT2046_H_
#include "stm32f4xx_hal.h"
#include <stdbool.h>
#ifdef __cplusplus
extern "C" {
#endif
#define XPT2046_CS_PORT GPIOA
#define XPT2046_CS_PIN GPIO_PIN_8
void XPT2046_Init(SPI_HandleTypeDef *spi);
bool XPT2046_TouchDetected(void);
/** Read full 12-bit coordinates; on failure outputs remain unchanged. */
bool XPT2046_GetTouch(uint16_t *x, uint16_t *y);
#ifdef __cplusplus
}
#endif
#endif
