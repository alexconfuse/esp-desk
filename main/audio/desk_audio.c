#include "desk_audio.h"

#include <stdlib.h>
#include <string.h>

#include "bsp/esp_mosaico.h"
#include "esp_codec_dev.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "desk_audio";

static esp_codec_dev_handle_t s_spk;
static esp_codec_dev_handle_t s_mic;
static bool s_ready;

esp_err_t desk_audio_init(void)
{
    if (s_ready) {
        return ESP_OK;
    }

    esp_err_t err = bsp_audio_init(NULL);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "bsp_audio_init failed (%s) — Talk mic/spk disabled", esp_err_to_name(err));
        return err;
    }

    s_spk = bsp_audio_codec_speaker_init();
    s_mic = bsp_audio_codec_microphone_init();
    if (!s_spk || !s_mic) {
        ESP_LOGW(TAG, "codec handles missing — audio soft-fail");
        return ESP_FAIL;
    }

    esp_codec_dev_sample_info_t spk_fs = {
        .bits_per_sample = 16,
        .channel = 1,
        .sample_rate = DESK_AUDIO_TTS_HZ,
    };
    err = esp_codec_dev_open(s_spk, &spk_fs);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "speaker open: %s", esp_err_to_name(err));
        return err;
    }
    esp_codec_dev_set_out_vol(s_spk, 70);

    esp_codec_dev_sample_info_t mic_fs = {
        .bits_per_sample = 16,
        .channel = DESK_AUDIO_REC_CH,
        .sample_rate = DESK_AUDIO_REC_HZ,
    };
    err = esp_codec_dev_open(s_mic, &mic_fs);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "mic open: %s", esp_err_to_name(err));
        return err;
    }
    esp_codec_dev_set_in_gain(s_mic, 30.0f);

    s_ready = true;
    ESP_LOGI(TAG, "Audio ready (mic %d Hz, spk %d Hz)", DESK_AUDIO_REC_HZ, DESK_AUDIO_TTS_HZ);
    return ESP_OK;
}

bool desk_audio_ready(void)
{
    return s_ready;
}

esp_err_t desk_audio_record_pcm(uint32_t max_ms, volatile bool *cancel,
                                int16_t **out_pcm, size_t *out_bytes)
{
    if (!s_ready || !out_pcm || !out_bytes) {
        return ESP_ERR_INVALID_STATE;
    }
    *out_pcm = NULL;
    *out_bytes = 0;

    size_t total = (size_t)DESK_AUDIO_REC_HZ * (max_ms / 1000) * sizeof(int16_t);
    if (max_ms % 1000) {
        total += (size_t)DESK_AUDIO_REC_HZ * sizeof(int16_t);
    }
    if (total < DESK_AUDIO_REC_HZ) {
        total = DESK_AUDIO_REC_HZ * sizeof(int16_t);
    }

    int16_t *buf = calloc(1, total);
    if (!buf) {
        return ESP_ERR_NO_MEM;
    }

    const size_t chunk = 512 * sizeof(int16_t);
    size_t got = 0;
    int64_t start = esp_timer_get_time();
    int64_t limit_us = (int64_t)max_ms * 1000;

    while (got + chunk <= total) {
        if (cancel && *cancel) {
            ESP_LOGI(TAG, "Record cancelled");
            break;
        }
        if (esp_timer_get_time() - start >= limit_us) {
            break;
        }
        esp_err_t err = esp_codec_dev_read(s_mic, ((uint8_t *)buf) + got, chunk);
        if (err != ESP_OK) {
            ESP_LOGW(TAG, "mic read: %s", esp_err_to_name(err));
            break;
        }
        got += chunk;
    }

    if (got < DESK_AUDIO_REC_HZ * sizeof(int16_t) / 4) {
        free(buf);
        return ESP_FAIL;
    }
    *out_pcm = buf;
    *out_bytes = got;
    ESP_LOGI(TAG, "Recorded %u bytes", (unsigned)got);
    return ESP_OK;
}

esp_err_t desk_audio_play_pcm(const int16_t *pcm, size_t bytes, int sample_hz)
{
    if (!s_ready || !pcm || bytes == 0) {
        return ESP_ERR_INVALID_STATE;
    }
    if (sample_hz > 0 && sample_hz != DESK_AUDIO_TTS_HZ) {
        esp_codec_dev_sample_info_t fs = {
            .bits_per_sample = 16,
            .channel = 1,
            .sample_rate = sample_hz,
        };
        esp_codec_dev_close(s_spk);
        esp_codec_dev_open(s_spk, &fs);
        esp_codec_dev_set_out_vol(s_spk, 70);
    }

    const size_t chunk = 1024 * sizeof(int16_t);
    size_t off = 0;
    while (off < bytes) {
        size_t n = bytes - off;
        if (n > chunk) {
            n = chunk;
        }
        esp_err_t err = esp_codec_dev_write(s_spk, ((const uint8_t *)pcm) + off, n);
        if (err != ESP_OK) {
            ESP_LOGW(TAG, "spk write: %s", esp_err_to_name(err));
            return err;
        }
        off += n;
    }
    return ESP_OK;
}
