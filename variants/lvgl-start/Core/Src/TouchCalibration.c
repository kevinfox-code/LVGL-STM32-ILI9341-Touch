#include "TouchCalibration.h"

bool TouchAxis_Set(TouchAxis *axis, int32_t raw_start, int32_t raw_end,
                   int32_t pixel_start, int32_t pixel_end)
{
    int32_t span = raw_end - raw_start;
    if (!axis || raw_start < 0 || raw_start > 4095 || raw_end < 0 || raw_end > 4095 ||
        (span > -100 && span < 100) || pixel_start < 0 || pixel_end <= pixel_start)
        return false;
    *axis = (TouchAxis){raw_start, raw_end, pixel_start, pixel_end};
    return true;
}

int32_t TouchAxis_Map(const TouchAxis *axis, uint16_t raw, int32_t resolution)
{
    if (!axis || resolution <= 1 || axis->raw_end == axis->raw_start) return 0;
    int32_t pixel = axis->pixel_start +
        (int32_t)(((int64_t)raw - axis->raw_start) * (axis->pixel_end - axis->pixel_start) /
                  (axis->raw_end - axis->raw_start));
    if (pixel < 0) return 0;
    if (pixel >= resolution) return resolution - 1;
    return pixel;
}
