#pragma once

#include "lvgl.h"

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Talk / Type controls shown while curious. */
void action_panel_create(lv_obj_t *parent);

void action_panel_show(bool visible);

/** Update status line (LVGL lock required). */
void action_panel_set_status(const char *text);

#ifdef __cplusplus
}
#endif
