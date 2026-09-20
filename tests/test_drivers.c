#include "ILI9341.h"
#include "XPT2046.h"
#include "TouchCalibration.h"
#include "LCDController.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

SPI_HandleTypeDef hspi1, hspi2;
TIM_HandleTypeDef htim2;
static lv_display_t display;
static lv_obj_t textarea;
lv_obj_t *ui_TextArea1 = &textarea;
static void (*flush_cb)(lv_display_t *, const lv_area_t *, uint8_t *);
static void (*wait_cb)(lv_display_t *);
static HAL_StatusTypeDef transfer_result, dma_result, receive_result;
static uint32_t tick, byte_count, aborts, ready_calls, receive_calls;
static GPIO_PinState display_cs, touch_cs, dc;
static uint8_t commands[32], payload[32];
static unsigned command_count, payload_count;
static uint16_t samples[8];
static uint16_t dma_count;
static uint8_t *dma_source;
static int fail_receive_at, slider_value;
static char slider_text[64];

void Error_Handler(void) { assert(!"Unexpected fatal firmware error"); }
void HAL_GPIO_WritePin(void *port, uint16_t pin, GPIO_PinState state)
{
    if (port == GPIOA && pin == GPIO_PIN_9) display_cs = state;
    if (port == GPIOA && pin == GPIO_PIN_8) touch_cs = state;
    if (port == GPIOB && pin == GPIO_PIN_6) dc = state;
}
HAL_StatusTypeDef HAL_SPI_Transmit(SPI_HandleTypeDef *spi, uint8_t *bytes, uint16_t count, uint32_t timeout)
{
    assert(spi == &hspi1 && display_cs == GPIO_PIN_RESET && timeout <= 100);
    byte_count += count;
    if (!dc && command_count < sizeof(commands)) commands[command_count++] = bytes[0];
    if (dc && payload_count + count <= sizeof(payload)) {
        memcpy(payload + payload_count, bytes, count); payload_count += count;
    }
    return transfer_result;
}
HAL_StatusTypeDef HAL_SPI_Transmit_DMA(SPI_HandleTypeDef *spi, uint8_t *bytes, uint16_t count)
{
    assert(spi == &hspi1 && display_cs == GPIO_PIN_RESET);
    dma_count = count; dma_source = bytes;
    return dma_result;
}
HAL_StatusTypeDef HAL_SPI_Abort(SPI_HandleTypeDef *spi)
{
    assert(spi == &hspi1); ++aborts; return HAL_OK;
}
HAL_StatusTypeDef HAL_SPI_TransmitReceive(SPI_HandleTypeDef *spi, uint8_t *tx, uint8_t *rx, uint16_t count, uint32_t timeout)
{
    static const uint8_t expected[] = {0xB0,0xC0,0xD0,0x90,0xD0,0x90,0xB0,0xC0};
    assert(spi == &hspi2 && count == 3 && timeout == 10 && touch_cs == GPIO_PIN_RESET);
    assert(receive_calls < 8 && tx[0] == expected[receive_calls]);
    uint16_t wire = (uint16_t)(samples[receive_calls] << 3);
    rx[0] = 0; rx[1] = wire >> 8; rx[2] = wire & 255;
    ++receive_calls;
    return (int)receive_calls == fail_receive_at ? receive_result : HAL_OK;
}
uint32_t HAL_GetTick(void) { return tick++; }
void HAL_Delay(uint32_t delay) { tick += delay; }
lv_display_t *lv_display_create(int32_t w, int32_t h) { assert(w == 320 && h == 240); return &display; }
void lv_display_set_color_format(lv_display_t *d, int f) { assert(d == &display && f == LV_COLOR_FORMAT_RGB565); }
void lv_display_set_flush_cb(lv_display_t *d, void (*cb)(lv_display_t *, const lv_area_t *, uint8_t *)) { (void)d; flush_cb = cb; }
void lv_display_set_flush_wait_cb(lv_display_t *d, void (*cb)(lv_display_t *)) { (void)d; wait_cb = cb; }
void lv_display_set_buffers(lv_display_t *d, void *a, void *b, uint32_t n, int mode) { (void)d; (void)mode; assert(a && b && a != b && n == 6400); }
void lv_display_flush_ready(lv_display_t *d) { (void)d; ++ready_calls; }
lv_obj_t *lv_event_get_target(lv_event_t *e) { (void)e; return &textarea; }
int32_t lv_slider_get_value(lv_obj_t *o) { (void)o; return slider_value; }
void lv_textarea_set_text(lv_obj_t *o, const char *text) { assert(o == &textarea); snprintf(slider_text, sizeof(slider_text), "%s", text); }
void slider_changed(lv_event_t *);

static void reset(void)
{
    transfer_result = dma_result = receive_result = HAL_OK;
    tick = byte_count = aborts = ready_calls = receive_calls = 0;
    command_count = payload_count = dma_count = 0; dma_source = NULL;
    display_cs = touch_cs = GPIO_PIN_SET; dc = GPIO_PIN_RESET;
    fail_receive_at = 0;
    const uint16_t values[] = {500,3500,4095,1000,4095,1002,500,3500};
    memcpy(samples, values, sizeof(values));
    XPT2046_Init(&hspi2);
}
static void test_touch_full_12_bit_decode(void)
{
    // Arrange
    reset(); uint16_t x = 0, y = 0;
    // Act
    bool pressed = XPT2046_GetTouch(&x, &y);
    // Assert
    assert(pressed && x == 4095 && y == 1001 && touch_cs == GPIO_PIN_SET);
}
static void test_touch_spi_failures_preserve_outputs(void)
{
    for (int error = HAL_ERROR; error <= HAL_TIMEOUT; ++error) {
        for (int call = 1; call <= 8; ++call) {
            // Arrange
            reset(); receive_result = error; fail_receive_at = call; uint16_t x = 17, y = 29;
            // Act
            bool pressed = XPT2046_GetTouch(&x, &y);
            // Assert
            assert(!pressed && x == 17 && y == 29 && touch_cs == GPIO_PIN_SET);
        }
    }
}
static void test_touch_release_and_null(void)
{
    // Arrange
    reset(); samples[0] = 0; samples[1] = 4095; uint16_t x = 17, y = 29;
    // Act
    bool pressed = XPT2046_GetTouch(&x, &y);
    // Assert
    assert(!pressed && x == 17 && y == 29);
    assert(!XPT2046_GetTouch(NULL, &y));
    XPT2046_Init(NULL);
    assert(!XPT2046_GetTouch(&x, &y));
}
static void test_display_dma_lifetime(void)
{
    // Arrange
    reset(); uint8_t pixels[6400] = {0}; lv_port_disp_init();
    const lv_area_t area = {0,0,319,9};
    // Act
    flush_cb(&display, &area, pixels);
    // Assert
    assert(ready_calls == 0 && dma_count == sizeof(pixels) && dma_source == pixels);
    assert(display_cs == GPIO_PIN_RESET);
    assert(ILI9341_DrawBitmapDMA(1,1,pixels) == HAL_BUSY);
    HAL_SPI_TxCpltCallback(&hspi1);
    wait_cb(&display);
    assert(display_cs == GPIO_PIN_SET && aborts == 0);
}
static void test_display_dma_timeout_and_error(void)
{
    for (int error = 0; error < 2; ++error) {
        // Arrange
        reset(); uint8_t pixels[2] = {0}; tick = UINT32_MAX - 50;
        assert(ILI9341_DrawBitmapDMA(1,1,pixels) == HAL_OK);
        if (error) HAL_SPI_ErrorCallback(&hspi1);
        else HAL_SPI_TxCpltCallback(&hspi2); /* Must not complete SPI1. */
        // Act
        HAL_StatusTypeDef result = ILI9341_WaitTransfer();
        // Assert
        assert(result == (error ? HAL_ERROR : HAL_TIMEOUT));
        assert(aborts == 1 && display_cs == GPIO_PIN_SET);
        assert(ILI9341_WaitTransfer() == HAL_OK);
    }
}
static void test_display_dma_start_failures(void)
{
    for (int error = HAL_ERROR; error <= HAL_TIMEOUT; ++error) {
        // Arrange
        reset(); dma_result = error; uint8_t pixels[2] = {0};
        // Act
        HAL_StatusTypeDef result = ILI9341_DrawBitmapDMA(1,1,pixels);
        // Assert
        assert(result == (HAL_StatusTypeDef)error && display_cs == GPIO_PIN_SET);
        assert(ILI9341_WaitTransfer() == HAL_OK);
    }
}
static void test_display_full_frame_and_rotation(void)
{
    // Arrange
    reset(); static uint8_t pixels[320*240*2]; assert(ILI9341_SetRotation(3) == HAL_OK);
    byte_count = 0;
    // Act
    HAL_StatusTypeDef result = ILI9341_DrawBitmap(320,240,pixels);
    // Assert
    assert(result == HAL_OK && byte_count == sizeof(pixels)+1 && display_cs == GPIO_PIN_SET);
    assert(ILI9341_DrawBitmapDMA(320,240,pixels) == HAL_ERROR);
    assert(ILI9341_DrawBitmap(0,240,NULL) == HAL_OK);
    assert(ILI9341_DrawBitmap(1,1,NULL) == HAL_ERROR);
    assert(ILI9341_SetWindow(0,0,319,239) == HAL_OK);
    assert(ILI9341_SetWindow(0,0,319,240) == HAL_ERROR);
    assert(ILI9341_SetRotation(4) == HAL_ERROR);
    assert(ILI9341_SetRotation(0) == HAL_OK);
    assert(ILI9341_SetWindow(0,0,239,319) == HAL_OK);
    assert(ILI9341_SetWindow(0,0,240,319) == HAL_ERROR);
}
static void test_display_fill_window_bytes(void)
{
    // Arrange
    reset(); assert(ILI9341_SetRotation(3) == HAL_OK);
    payload_count = command_count = byte_count = 0;
    // Act
    HAL_StatusTypeDef result = ILI9341_FillScreen(0xF800);
    // Assert
    const uint8_t expected[] = {0,0,1,63,0,0,0,239};
    assert(result == HAL_OK && payload_count == sizeof(expected));
    assert(memcmp(payload, expected, sizeof(expected)) == 0);
    assert(byte_count == 320U*240U*2U + 11U);
}
static void test_calibration_inset_and_reversed_axes(void)
{
    // Arrange
    TouchAxis a = {500,3500,20,299};
    // Act
    int32_t midpoint = TouchAxis_Map(&a,2000,320);
    // Assert
    assert(midpoint == 159 && TouchAxis_Map(&a,500,320) == 20);
    assert(TouchAxis_Map(&a,3500,320) == 299 && TouchAxis_Map(&a,4095,320) == 319);
    assert(TouchAxis_Map(&a,0,320) == 0);
    assert(TouchAxis_Set(&a,3500,500,20,299));
    assert(TouchAxis_Map(&a,3500,320) == 20 && TouchAxis_Map(&a,500,320) == 299);
    assert(!TouchAxis_Set(&a,500,501,20,299) && a.raw_start == 3500);
}
#ifdef TEST_SLIDER
static void test_slider_bounded_format_and_duty(void)
{
    for (int value = -10; value <= 110; ++value) {
        // Arrange
        reset(); slider_value = value; htim2.arr = 999;
        // Act
        slider_changed(NULL);
        // Assert
        char expected[64]; snprintf(expected, sizeof(expected), "Slider Value: %d", value);
        assert(strcmp(slider_text,expected) == 0);
        int clamped = value < 0 ? 0 : value > 100 ? 100 : value;
        assert(htim2.compare == (uint32_t)clamped * 10U);
    }
}
#endif
int main(void)
{
    test_touch_full_12_bit_decode(); test_touch_spi_failures_preserve_outputs(); test_touch_release_and_null();
    test_display_dma_lifetime(); test_display_dma_timeout_and_error(); test_display_dma_start_failures();
    test_display_full_frame_and_rotation(); test_display_fill_window_bytes(); test_calibration_inset_and_reversed_axes();
#ifdef TEST_SLIDER
    test_slider_bounded_format_and_duty();
#endif
    puts("PASS: touch, calibration, display/DMA and applicable PWM regressions");
}
