#ifndef TOUCH_CONTROLLER_H_
#define TOUCH_CONTROLLER_H_
#include "lvgl.h"
#include <stdbool.h>
#ifdef __cplusplus
extern "C" {
#endif
void TouchController_Init(void);
/** Optional foreground calibration. Each press/release times out after 10 s.
 * Updates RAM mapping on success, restores the previous screen on every exit.
 * Call outside LVGL callbacks; false retains the previous calibration.
 */
bool touch_calibrate(void);
#ifdef __cplusplus
}
#endif
#endif
