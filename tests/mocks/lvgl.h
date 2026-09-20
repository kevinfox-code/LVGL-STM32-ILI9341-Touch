#ifndef MOCK_LVGL_H
#define MOCK_LVGL_H
#include <stdint.h>
#include <stddef.h>
typedef struct { int unused; } lv_display_t;
typedef struct { int unused; } lv_obj_t;
typedef struct { int unused; } lv_event_t;
typedef struct { int32_t x1,y1,x2,y2; } lv_area_t;
#define LV_COLOR_FORMAT_RGB565 1
#define LV_ATTRIBUTE_MEM_ALIGN
#define LV_DISPLAY_RENDER_MODE_PARTIAL 0
extern lv_obj_t *ui_TextArea1;
lv_display_t *lv_display_create(int32_t, int32_t);
void lv_display_set_color_format(lv_display_t *, int);
void lv_display_set_flush_cb(lv_display_t *, void (*)(lv_display_t *, const lv_area_t *, uint8_t *));
void lv_display_set_flush_wait_cb(lv_display_t *, void (*)(lv_display_t *));
void lv_display_set_buffers(lv_display_t *, void *, void *, uint32_t, int);
void lv_display_flush_ready(lv_display_t *);
lv_obj_t *lv_event_get_target(lv_event_t *);
int32_t lv_slider_get_value(lv_obj_t *);
void lv_textarea_set_text(lv_obj_t *, const char *);
#endif
