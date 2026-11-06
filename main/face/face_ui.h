#pragma once

#include "face_emotion.h"
#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Build the full-screen pet face on @p parent (usually the active screen). */
void face_ui_create(lv_obj_t *parent);

/** Crossfade / retarget idle ↔ curious animations. */
void face_ui_set_emotion(face_emotion_t emotion);

face_emotion_t face_ui_get_emotion(void);

#ifdef __cplusplus
}
#endif
