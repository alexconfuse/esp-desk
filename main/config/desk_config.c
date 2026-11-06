#include "desk_config.h"

#include <string.h>

#include "esp_log.h"
#include "nvs.h"
#include "nvs_flash.h"

static const char *TAG = "desk_config";
static const char *NVS_NS = "desk";

static void copy_str(char *dst, size_t dst_len, const char *src)
{
    if (!dst || dst_len == 0) {
        return;
    }
    if (!src) {
        dst[0] = '\0';
        return;
    }
    strncpy(dst, src, dst_len - 1);
    dst[dst_len - 1] = '\0';
}

void desk_config_set_defaults(desk_config_t *cfg)
{
    if (!cfg) {
        return;
    }
    memset(cfg, 0, sizeof(*cfg));
    copy_str(cfg->openai_base_url, sizeof(cfg->openai_base_url), DESK_OPENAI_URL_DEFAULT);
    cfg->tts_provider = DESK_TTS_PROVIDER_OPENAI;
}

esp_err_t desk_config_nvs_init(void)
{
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    return err;
}

static esp_err_t load_str(nvs_handle_t h, const char *key, char *dst, size_t dst_len)
{
    size_t len = dst_len;
    esp_err_t err = nvs_get_str(h, key, dst, &len);
    if (err == ESP_ERR_NVS_NOT_FOUND) {
        dst[0] = '\0';
        return ESP_OK;
    }
    return err;
}

esp_err_t desk_config_load(desk_config_t *out)
{
    if (!out) {
        return ESP_ERR_INVALID_ARG;
    }
    desk_config_set_defaults(out);

    nvs_handle_t h;
    esp_err_t err = nvs_open(NVS_NS, NVS_READONLY, &h);
    if (err == ESP_ERR_NVS_NOT_FOUND) {
        ESP_LOGI(TAG, "No desk NVS namespace yet — using defaults");
        return ESP_OK;
    }
    if (err != ESP_OK) {
        return err;
    }

    err = load_str(h, "wifi_ssid", out->wifi_ssid, sizeof(out->wifi_ssid));
    if (err == ESP_OK) {
        err = load_str(h, "wifi_pass", out->wifi_pass, sizeof(out->wifi_pass));
    }
    if (err == ESP_OK) {
        err = load_str(h, "openai_url", out->openai_base_url, sizeof(out->openai_base_url));
        if (out->openai_base_url[0] == '\0') {
            copy_str(out->openai_base_url, sizeof(out->openai_base_url), DESK_OPENAI_URL_DEFAULT);
        }
    }
    if (err == ESP_OK) {
        err = load_str(h, "openai_key", out->openai_api_key, sizeof(out->openai_api_key));
    }
    if (err == ESP_OK) {
        err = load_str(h, "eleven_key", out->elevenlabs_api_key, sizeof(out->elevenlabs_api_key));
    }
    if (err == ESP_OK) {
        uint8_t prov = 0;
        esp_err_t perr = nvs_get_u8(h, "tts_prov", &prov);
        if (perr == ESP_OK) {
            out->tts_provider = (prov == DESK_TTS_PROVIDER_ELEVENLABS)
                                    ? DESK_TTS_PROVIDER_ELEVENLABS
                                    : DESK_TTS_PROVIDER_OPENAI;
        } else if (perr != ESP_ERR_NVS_NOT_FOUND) {
            err = perr;
        }
    }

    nvs_close(h);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Load failed: %s", esp_err_to_name(err));
    } else {
        ESP_LOGI(TAG, "Loaded (ssid=%s openai_key=%s eleven_key=%s tts=%s)",
                 out->wifi_ssid[0] ? out->wifi_ssid : "(none)",
                 out->openai_api_key[0] ? "set" : "empty",
                 out->elevenlabs_api_key[0] ? "set" : "empty",
                 out->tts_provider == DESK_TTS_PROVIDER_ELEVENLABS ? "elevenlabs" : "openai");
    }
    return err;
}

esp_err_t desk_config_save(const desk_config_t *cfg)
{
    if (!cfg) {
        return ESP_ERR_INVALID_ARG;
    }

    nvs_handle_t h;
    esp_err_t err = nvs_open(NVS_NS, NVS_READWRITE, &h);
    if (err != ESP_OK) {
        return err;
    }

    err = nvs_set_str(h, "wifi_ssid", cfg->wifi_ssid);
    if (err == ESP_OK) {
        err = nvs_set_str(h, "wifi_pass", cfg->wifi_pass);
    }
    if (err == ESP_OK) {
        err = nvs_set_str(h, "openai_url", cfg->openai_base_url);
    }
    if (err == ESP_OK) {
        err = nvs_set_str(h, "openai_key", cfg->openai_api_key);
    }
    if (err == ESP_OK) {
        err = nvs_set_str(h, "eleven_key", cfg->elevenlabs_api_key);
    }
    if (err == ESP_OK) {
        err = nvs_set_u8(h, "tts_prov", (uint8_t)cfg->tts_provider);
    }
    if (err == ESP_OK) {
        err = nvs_commit(h);
    }
    nvs_close(h);

    if (err == ESP_OK) {
        ESP_LOGI(TAG, "Saved desk config to NVS");
    } else {
        ESP_LOGE(TAG, "Save failed: %s", esp_err_to_name(err));
    }
    return err;
}

bool desk_config_has_wifi(const desk_config_t *cfg)
{
    return cfg && cfg->wifi_ssid[0] != '\0';
}
