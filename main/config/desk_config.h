#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

#define DESK_WIFI_SSID_MAX     32
#define DESK_WIFI_PASS_MAX     64
#define DESK_OPENAI_URL_MAX    128
#define DESK_API_KEY_MAX       128
#define DESK_ELEVEN_KEY_MAX    128

#define DESK_OPENAI_URL_DEFAULT "https://api.openai.com/v1"

typedef enum {
    DESK_TTS_PROVIDER_OPENAI = 0,
    DESK_TTS_PROVIDER_ELEVENLABS = 1,
} desk_tts_provider_t;

typedef struct {
    char wifi_ssid[DESK_WIFI_SSID_MAX];
    char wifi_pass[DESK_WIFI_PASS_MAX];
    char openai_base_url[DESK_OPENAI_URL_MAX];
    char openai_api_key[DESK_API_KEY_MAX];
    desk_tts_provider_t tts_provider;
    char elevenlabs_api_key[DESK_ELEVEN_KEY_MAX];
} desk_config_t;

/** Init NVS flash (safe to call once from app_main). */
esp_err_t desk_config_nvs_init(void);

/** Load from NVS namespace "desk"; missing keys get empty / defaults. */
esp_err_t desk_config_load(desk_config_t *out);

/** Persist the full config blob to NVS. */
esp_err_t desk_config_save(const desk_config_t *cfg);

/** True when a non-empty Wi‑Fi SSID is stored (first-boot gate). */
bool desk_config_has_wifi(const desk_config_t *cfg);

/** Fill defaults without touching NVS. */
void desk_config_set_defaults(desk_config_t *cfg);

#ifdef __cplusplus
}
#endif
