#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

#define DESK_AUDIO_REC_HZ     16000
#define DESK_AUDIO_REC_CH     1
#define DESK_AUDIO_TTS_HZ     24000

/** Init ES8311 mic + speaker via Mosaico BSP. Soft-fails if codec missing. */
esp_err_t desk_audio_init(void);

bool desk_audio_ready(void);

/**
 * Record mono PCM16 @ 16 kHz for up to @p max_ms.
 * Polls @p cancel; stops early when *cancel is true.
 * Caller frees *out_pcm with free().
 */
esp_err_t desk_audio_record_pcm(uint32_t max_ms, volatile bool *cancel,
                                int16_t **out_pcm, size_t *out_bytes);

/** Play mono PCM16 (e.g. OpenAI TTS pcm @ 24 kHz). */
esp_err_t desk_audio_play_pcm(const int16_t *pcm, size_t bytes, int sample_hz);

#ifdef __cplusplus
}
#endif
