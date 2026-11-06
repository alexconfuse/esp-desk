#pragma once

#include "lvgl.h"
#include "sim_config.h"

#ifdef __cplusplus
extern "C" {
#endif

void sim_settings_ui_create(lv_obj_t *parent, sim_config_t *cfg);

void sim_settings_ui_show(void);

void sim_settings_ui_hide(void);

/** Small always-visible gear control (top-right). */
void sim_settings_ui_create_gear(lv_obj_t *parent);

#ifdef __cplusplus
}
#endif
