/**
 * TouchController.c – LVGL ↔︎ XPT2046 glue (landscape, rotation 3)
 * Kev Fox – 2025‑06‑01
 */
#include "TouchController.h"
#include "XPT2046.h"
#include "main.h"

#include <stdio.h>
#include <inttypes.h>          /* PRIu16 / PRIi32 */

extern SPI_HandleTypeDef hspi2;    /* CubeMX creates this     */
extern UART_HandleTypeDef huart2;  /* for printf‑retargeting   */

int __io_putchar (int ch)                  /* called by _write() stub */
{
    /* transmit single byte, blocking */
    HAL_UART_Transmit(&huart2, (uint8_t *)&ch, 1, HAL_MAX_DELAY);
    return ch;
}

/* optional – makes scanf()/getchar() work */
int __io_getchar (void)
{
    uint8_t ch;
    HAL_UART_Receive(&huart2, &ch, 1, HAL_MAX_DELAY);
    return ch;
}


/*
Corner	raw‑X	raw‑Y
top‑left	≈ 273	≈ 242
top‑right	≈ 516	≈ 1336
bottom‑left	≈ 1872	≈ 238
bottom‑right	≈ 1752	≈ 1896

for the mapping is the extremes per axis:
raw‑Y left edge  ≈ 242   → put into RAW_Y_MIN
raw‑Y right edge ≈ 1896  → put into RAW_Y_MAX
raw‑X top edge  ≈ 240   → put into RAW_X_MIN
raw‑X bottom edge≈ 1872  → put into RAW_X_MAX
 */

#define RAW_X_MIN   262u
#define RAW_X_MAX  1872u
#define RAW_Y_MIN   230u
#define RAW_Y_MAX  1872u


/* ---------- helpers -------------------------------------------------- */

static inline uint16_t map_x(uint16_t raw)          /* 0‥239 */
{
    if(raw <= RAW_Y_MIN) return 0;
    if(raw >= RAW_Y_MAX) return 239;
    return (uint32_t)(raw - RAW_Y_MIN) * 240 / (RAW_Y_MAX - RAW_Y_MIN);
}

static inline uint16_t map_y(uint16_t raw)          /* 0‥319 */
{
    if(raw <= RAW_X_MIN) return 0;
    if(raw >= RAW_X_MAX) return 319;
    return (uint32_t)(raw - RAW_X_MIN) * 320 / (RAW_X_MAX - RAW_X_MIN);
}

/* ---------- forward -------------------------------------------------- */
static void touchpad_read(lv_indev_t *, lv_indev_data_t *);

/* ---------- init ----------------------------------------------------- */
void TouchController_Init(void)
{
    XPT2046_Init(&hspi2);                      /* start the driver    */

    lv_indev_t *indev = lv_indev_create();     /* register with LVGL  */
    lv_indev_set_type   (indev, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(indev, touchpad_read);
}

/* ---------- PENIRQ ISR ---------------------------------------------- *
 *  LVGL 9.2 has no lv_indev_schedule_read(); we just clear the EXTI
 *  flag and return – LVGL will poll in the main loop next time anyway.  */
void HAL_GPIO_EXTI_Callback(uint16_t pin)
{
    /* nothing to do – keep function so CubeIDE links it            */
    (void)pin;
}

/* ---------- LVGL read callback -------------------------------------- */
static void touchpad_read(lv_indev_t * indev, lv_indev_data_t * data)
{
    static uint16_t last_x = 0, last_y = 0;

    uint16_t x_raw, y_raw;
    if (XPT2046_GetTouch(&x_raw, &y_raw)) {
        last_x     = x_raw;
        last_y     = y_raw;
        data->state = LV_INDEV_STATE_PRESSED;

        /* *** only print while finger is down *** */
        printf("raw:%4" PRIu16 ",%4" PRIu16 "  →  px:%3u, py:%3u\r\n",
               last_x, last_y,
               (uint16_t)(239 - map_x(last_y)),     /* what we’ll send */
               (uint16_t)        map_y(last_x));
    } else {
        data->state = LV_INDEV_STATE_RELEASED;
    }

    /* swap axes & flip X for ILI9341 rotation(3) */
    data->point.x = map_x(last_y);    /* raw‑Y → pixel‑X */
    data->point.y = map_y(last_x);   /* raw‑X → pixel‑Y */
}
