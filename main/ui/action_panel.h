#pragma once

#include "lvgl.h"

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*action_panel_cb_t)(void);

/**
 * Talk / Type controls shown while curious.
 * Default handlers are log stubs; host/firmware can replace via set_*_cb.
 */
void action_panel_create(lv_obj_t *parent);

void action_panel_show(bool visible);

void action_panel_set_status(const char *text);

void action_panel_set_talk_cb(action_panel_cb_t cb);

void action_panel_set_type_cb(action_panel_cb_t cb);

#ifdef __cplusplus
}
#endif
