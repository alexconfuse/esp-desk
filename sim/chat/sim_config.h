#pragma once

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    SIM_LANG_AUTO = 0,
    SIM_LANG_EN,
    SIM_LANG_FR,
} sim_lang_t;

typedef enum {
    SIM_TTS_OPENAI = 0,
    SIM_TTS_ELEVENLABS = 1,
} sim_tts_provider_t;

typedef struct {
    sim_lang_t lang;
    sim_tts_provider_t tts_provider;
    /** When true and OPENAI_API_KEY is set (+ curl built-in), call live chat. */
    bool prefer_live;
    char openai_base_url[128];
    char openai_api_key[160];
    char elevenlabs_api_key[160];
} sim_config_t;

void sim_config_init(sim_config_t *cfg);

/** Reload API keys / prefer_live from environment (safe anytime). */
void sim_config_reload_env(sim_config_t *cfg);

bool sim_config_has_openai_key(const sim_config_t *cfg);

const char *sim_lang_name(sim_lang_t lang);

const char *sim_tts_name(sim_tts_provider_t tts);

#ifdef __cplusplus
}
#endif
