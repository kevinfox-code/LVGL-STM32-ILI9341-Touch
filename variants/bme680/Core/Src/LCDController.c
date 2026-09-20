#include "LCDController.h"
#include "ILI9341.h"
#include "main.h"
#include <stdbool.h>

#define DISPLAY_WIDTH 320
#define DISPLAY_HEIGHT 240
#define BUFFER_ROWS 10
static bool flush_enabled = true;

static void disp_flush_wait(lv_display_t *display)
{
    (void)display;
    if (ILI9341_WaitTransfer() != HAL_OK) Error_Handler();
    /* LVGL clears its flushing flag after this callback returns. */
}

static void disp_flush(lv_display_t *display, const lv_area_t *area, uint8_t *pixels)
{
    if (!flush_enabled) {
        lv_display_flush_ready(display);
        return;
    }
    if (ILI9341_SetWindow(area->x1, area->y1, area->x2, area->y2) != HAL_OK ||
        ILI9341_DrawBitmapDMA(area->x2 - area->x1 + 1,
                             area->y2 - area->y1 + 1, pixels) != HAL_OK) {
        Error_Handler();
    }
    /* The wait callback retains the buffer until DMA has stopped reading it. */
}

void lv_port_disp_init(void)
{
    if (ILI9341_Init() != HAL_OK) Error_Handler();
    lv_display_t *display = lv_display_create(DISPLAY_WIDTH, DISPLAY_HEIGHT);
    if (!display) Error_Handler();
    lv_display_set_color_format(display, LV_COLOR_FORMAT_RGB565);
    lv_display_set_flush_cb(display, disp_flush);
    lv_display_set_flush_wait_cb(display, disp_flush_wait);
    LV_ATTRIBUTE_MEM_ALIGN static uint8_t buffer1[DISPLAY_WIDTH * BUFFER_ROWS * 2];
    LV_ATTRIBUTE_MEM_ALIGN static uint8_t buffer2[DISPLAY_WIDTH * BUFFER_ROWS * 2];
    lv_display_set_buffers(display, buffer1, buffer2, sizeof(buffer1), LV_DISPLAY_RENDER_MODE_PARTIAL);
}

void disp_enable_update(void) { flush_enabled = true; }
void disp_disable_update(void) { flush_enabled = false; }
