#include "TouchController.h"
#include "TouchCalibration.h"
#include "XPT2046.h"
#include "main.h"
#include <stdio.h>

extern SPI_HandleTypeDef hspi2;
extern UART_HandleTypeDef huart2;
/* Original panel measurements converted from the former 11-bit decoding. */
static TouchAxis horizontal = {460, 3744, 0, 319};
static TouchAxis vertical = {524, 3744, 0, 239};
static bool calibrating;
#define CALIBRATION_TIMEOUT_MS 10000U

int __io_putchar(int ch)
{
    uint8_t byte = (uint8_t)ch;
    return HAL_UART_Transmit(&huart2, &byte, 1, 20) == HAL_OK ? ch : EOF;
}

static void touchpad_read(lv_indev_t *indev, lv_indev_data_t *data)
{
    (void)indev;
    static int32_t last_x, last_y;
    uint16_t raw_x, raw_y;
    data->state = LV_INDEV_STATE_RELEASED;
    if (!calibrating && XPT2046_GetTouch(&raw_x, &raw_y)) {
        last_x = TouchAxis_Map(&horizontal, raw_y, lv_display_get_horizontal_resolution(NULL));
        last_y = TouchAxis_Map(&vertical, raw_x, lv_display_get_vertical_resolution(NULL));
        data->state = LV_INDEV_STATE_PRESSED;
    }
    data->point.x = last_x;
    data->point.y = last_y;
}

void TouchController_Init(void)
{
    XPT2046_Init(&hspi2);
    lv_indev_t *input = lv_indev_create();
    if (!input) Error_Handler();
    lv_indev_set_type(input, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(input, touchpad_read);
}

static bool wait_for_touch(bool pressed, uint16_t *x, uint16_t *y)
{
    uint32_t start = HAL_GetTick();
    do {
        lv_timer_handler();
        if (XPT2046_GetTouch(x, y) == pressed) return true;
        HAL_Delay(5);
    } while ((uint32_t)(HAL_GetTick() - start) < CALIBRATION_TIMEOUT_MS);
    return false;
}

bool touch_calibrate(void)
{
    lv_obj_t *previous = lv_screen_active();
    int32_t w = lv_display_get_horizontal_resolution(NULL);
    int32_t h = lv_display_get_vertical_resolution(NULL);
    if (!previous || w <= 41 || h <= 41) return false;
    lv_obj_t *screen = lv_obj_create(NULL);
    if (!screen) return false;
    lv_obj_remove_flag(screen, LV_OBJ_FLAG_SCROLLABLE);
    lv_screen_load(screen);
    calibrating = true;
    const lv_point_t corners[4] = {{20,20}, {w-21,20}, {w-21,h-21}, {20,h-21}};
    uint16_t raw_x[4], raw_y[4], x, y;
    bool success = wait_for_touch(false, &x, &y);
    for (unsigned i = 0; success && i < 4; ++i) {
        lv_obj_clean(screen);
        lv_obj_t *cross = lv_label_create(screen);
        lv_label_set_text(cross, "+");
        lv_obj_set_style_text_color(cross, lv_color_hex(0x00ff00), 0);
        lv_obj_update_layout(cross);
        lv_obj_set_pos(cross, corners[i].x - lv_obj_get_width(cross)/2,
                             corners[i].y - lv_obj_get_height(cross)/2);
        lv_obj_t *label = lv_label_create(screen);
        lv_label_set_text(label, "Touch the green +");
        lv_obj_align(label, LV_ALIGN_CENTER, 0, 40);
        success = wait_for_touch(true, &raw_x[i], &raw_y[i]);
        if (success) success = wait_for_touch(false, &x, &y);
    }
    if (success) {
        TouchAxis next_x, next_y;
        success = TouchAxis_Set(&next_x, (raw_y[0]+raw_y[3])/2, (raw_y[1]+raw_y[2])/2, 20, w-21) &&
                  TouchAxis_Set(&next_y, (raw_x[0]+raw_x[1])/2, (raw_x[2]+raw_x[3])/2, 20, h-21);
        if (success) { horizontal = next_x; vertical = next_y; }
    }
    lv_screen_load(previous);
    lv_obj_delete(screen);
    calibrating = false;
    return success;
}
