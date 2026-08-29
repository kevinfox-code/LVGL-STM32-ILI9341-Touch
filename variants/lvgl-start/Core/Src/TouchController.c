/**
 * TouchController.c ― glue layer between LVGL and the XPT2046 driver
 */
#include "TouchController.h"
#include "XPT2046.h"
#include "main.h"          /* for hspi2, pin names */

extern SPI_HandleTypeDef hspi2;

/* ---------- helpers -------------------------------------------------- */
static inline uint16_t map_adc(uint16_t raw, uint16_t pix)
{
    return ((uint32_t)raw * pix) / 4095u;
}

/* ---------- forward -------------------------------------------------- */
static void touchpad_read(lv_indev_t *indev, lv_indev_data_t *data);

/* ---------- init ----------------------------------------------------- */
void TouchController_Init(void)
{
    XPT2046_Init(&hspi2);

    lv_indev_t *indev = lv_indev_create();
    lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(indev, touchpad_read);
}

/* ---------- optional PENIRQ ISR (not required for polling) ---------- */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
#ifdef XPT2046_IRQ_PIN
    if(GPIO_Pin == XPT2046_IRQ_PIN) {
        /* No lv_indev_schedule_read() in LVGL 9.2; polling is fine.   */
    }
#endif
}

/* ---------- LVGL read callback -------------------------------------- */
static void touchpad_read(lv_indev_t *indev, lv_indev_data_t *data)
{
    static uint16_t last_x = 0, last_y = 0;

    uint16_t x_raw, y_raw;
    if (XPT2046_GetTouch(&x_raw, &y_raw)) {
        last_x     = x_raw;
        last_y     = y_raw;
        data->state = LV_INDEV_STATE_PRESSED;
    } else {
        data->state = LV_INDEV_STATE_RELEASED;
    }

    /* ILI9341 landscape SetRotation(3): swap + flip X */
    data->point.x = 240 - map_adc(last_y, 240);   /* 0…239 */
    data->point.y =          map_adc(last_x, 320); /* 0…319 */
}
