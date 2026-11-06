#pragma once

#include "desk_config.h"
#include "face_emotion.h"

#include <stddef.h>
#include <stdint.h>

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

#define OPENAI_TEXT_MAX 768

typedef enum {
    OPENAI_OK = 0,
    OPENAI_ERR_OFFLINE,
    OPENAI_ERR_BAD_KEY,
    OPENAI_ERR_HTTP,
    OPENAI_ERR_PARSE,
    OPENAI_ERR_NO_MEM,
    OPENAI_ERR_PARAM,
} openai_err_t;

typedef struct {
    face_emotion_t emotion;
    float intensity;
    char text[OPENAI_TEXT_MAX];
} openai_chat_reply_t;

void openai_client_set_config(const desk_config_t *cfg);

/** Whisper-compatible transcription of mono PCM16 WAV payload. */
openai_err_t openai_stt_transcribe(const int16_t *pcm, size_t pcm_bytes,
                                   int sample_hz, char *out_text, size_t out_len);

/** Chat Completions with JSON {emotion,intensity,text}; EN/FR auto. */
openai_err_t openai_chat_turn(const char *user_text, openai_chat_reply_t *out);

/**
 * OpenAI-compatible TTS → PCM16 mono @ 24 kHz (format=pcm).
 * Caller frees *out_pcm. ElevenLabs is not used here (settings stub only).
 */
openai_err_t openai_tts_pcm(const char *text, int16_t **out_pcm, size_t *out_bytes);

#ifdef __cplusplus
}
#endif
