#ifndef TOUCH_CALIBRATION_H
#define TOUCH_CALIBRATION_H
#include <stdint.h>
#include <stdbool.h>
typedef struct {
    int32_t raw_start, raw_end;
    int32_t pixel_start, pixel_end;
} TouchAxis;
/** Map either raw axis direction and clamp to [0, resolution-1]. */
int32_t TouchAxis_Map(const TouchAxis *axis, uint16_t raw, int32_t resolution);
/** Reject a degenerate axis without changing the previous calibration. */
bool TouchAxis_Set(TouchAxis *axis, int32_t raw_start, int32_t raw_end,
                   int32_t pixel_start, int32_t pixel_end);
#endif
