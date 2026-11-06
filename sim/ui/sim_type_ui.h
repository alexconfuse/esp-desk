#pragma once

#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*sim_type_submit_cb_t)(const char *text);

void sim_type_ui_create(lv_obj_t *parent, sim_type_submit_cb_t on_submit);

void sim_type_ui_show(void);

void sim_type_ui_hide(void);

#ifdef __cplusplus
}
#endif
