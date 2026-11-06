#pragma once

#include "face_emotion.h"
#include "lvgl.h"

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*face_ui_tap_cb_t)(void);

/** Build the full-screen pet face on @p parent (usually the active screen). */
void face_ui_create(lv_obj_t *parent);

/** Set emotion (caller must hold LVGL/BSP display lock). */
void face_ui_set_emotion(face_emotion_t emotion);

/** Thread-safe: takes bsp_display_lock, sets emotion, unlocks. */
void face_ui_set_emotion_locked(face_emotion_t emotion);

face_emotion_t face_ui_get_emotion(void);

/** Map LLM emotion token → enum; unknown → curious. */
face_emotion_t face_ui_emotion_from_str(const char *name);

/** Reply / status caption under the face (LVGL lock required). */
void face_ui_set_caption(const char *text);

void face_ui_set_caption_locked(const char *text);

/** Optional tap hook (e.g. cancel listen). Runs in LVGL context. */
void face_ui_set_tap_callback(face_ui_tap_cb_t cb);

/** Pulse mouth while speaking (lipsync-lite); no-op if no mouth. */
void face_ui_set_speaking_pulse(bool on);

#ifdef __cplusplus
}
#endif
