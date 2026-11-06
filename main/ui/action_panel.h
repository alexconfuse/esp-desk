#pragma once

#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Talk / Type stub controls shown while curious.
 * Buttons only log + flash a label — no mic, STT, or cloud.
 */
void action_panel_create(lv_obj_t *parent);

void action_panel_show(bool visible);

#ifdef __cplusplus
}
#endif
