#pragma once

#include "face_emotion.h"
#include "sim_config.h"

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    face_emotion_t emotion;
    char text[320];
    bool used_live;
} sim_llm_reply_t;

/**
 * Produce a buddy reply for @p user_text.
 * Mock by default; if prefer_live + key (+ curl), tries OpenAI-compatible chat.
 */
void sim_llm_reply(const sim_config_t *cfg, const char *user_text, sim_llm_reply_t *out);

/** Mock STT phrase for Talk (no mic). */
void sim_llm_mock_stt(const sim_config_t *cfg, char *out, size_t out_len);

/** Detect dominant reply language for Auto mode. */
sim_lang_t sim_llm_detect_lang(const char *text);

#ifdef __cplusplus
}
#endif
