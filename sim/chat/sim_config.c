#include "sim_config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

void sim_config_init(sim_config_t *cfg)
{
    memset(cfg, 0, sizeof(*cfg));
    cfg->lang = SIM_LANG_AUTO;
    cfg->tts_provider = SIM_TTS_OPENAI;
    cfg->prefer_live = false;
    snprintf(cfg->openai_base_url, sizeof(cfg->openai_base_url),
             "https://api.openai.com/v1");
    sim_config_reload_env(cfg);
}

void sim_config_reload_env(sim_config_t *cfg)
{
    const char *key = getenv("OPENAI_API_KEY");
    if (key && key[0]) {
        snprintf(cfg->openai_api_key, sizeof(cfg->openai_api_key), "%s", key);
    }

    const char *base = getenv("OPENAI_BASE_URL");
    if (base && base[0]) {
        snprintf(cfg->openai_base_url, sizeof(cfg->openai_base_url), "%s", base);
    }

    const char *el = getenv("ELEVENLABS_API_KEY");
    if (el && el[0]) {
        snprintf(cfg->elevenlabs_api_key, sizeof(cfg->elevenlabs_api_key), "%s", el);
    }

    const char *live = getenv("DESK_BUDDY_SIM_LIVE");
    if (live && (live[0] == '1' || live[0] == 'y' || live[0] == 'Y' || live[0] == 't' || live[0] == 'T')) {
        cfg->prefer_live = true;
    }

    const char *lang = getenv("DESK_BUDDY_SIM_LANG");
    if (lang) {
        if (!strcasecmp(lang, "en")) {
            cfg->lang = SIM_LANG_EN;
        } else if (!strcasecmp(lang, "fr")) {
            cfg->lang = SIM_LANG_FR;
        } else if (!strcasecmp(lang, "auto")) {
            cfg->lang = SIM_LANG_AUTO;
        }
    }
}

bool sim_config_has_openai_key(const sim_config_t *cfg)
{
    return cfg && cfg->openai_api_key[0] != '\0';
}

const char *sim_lang_name(sim_lang_t lang)
{
    switch (lang) {
    case SIM_LANG_EN: return "EN";
    case SIM_LANG_FR: return "FR";
    default: return "Auto";
    }
}

const char *sim_tts_name(sim_tts_provider_t tts)
{
    return (tts == SIM_TTS_ELEVENLABS) ? "ElevenLabs" : "OpenAI";
}
