#include "settings_http.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "desk_config.h"
#include "esp_http_server.h"
#include "esp_log.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "mdns.h"
#include "wifi_mgr.h"

static const char *TAG = "settings_http";

static httpd_handle_t s_server;
static desk_config_t s_cfg;

/* ---- tiny application/x-www-form-urlencoded helpers ---- */

static int hex_val(char c)
{
    if (c >= '0' && c <= '9') {
        return c - '0';
    }
    if (c >= 'a' && c <= 'f') {
        return c - 'a' + 10;
    }
    if (c >= 'A' && c <= 'F') {
        return c - 'A' + 10;
    }
    return -1;
}

static void url_decode(char *s)
{
    char *src = s;
    char *dst = s;
    while (*src) {
        if (*src == '+') {
            *dst++ = ' ';
            src++;
        } else if (*src == '%' && hex_val(src[1]) >= 0 && hex_val(src[2]) >= 0) {
            *dst++ = (char)((hex_val(src[1]) << 4) | hex_val(src[2]));
            src += 3;
        } else {
            *dst++ = *src++;
        }
    }
    *dst = '\0';
}

static bool form_get(const char *body, const char *key, char *out, size_t out_len)
{
    if (!body || !key || !out || out_len == 0) {
        return false;
    }
    size_t klen = strlen(key);
    const char *p = body;
    while (p && *p) {
        const char *amp = strchr(p, '&');
        size_t pair_len = amp ? (size_t)(amp - p) : strlen(p);
        if (pair_len > klen && p[klen] == '=' && strncmp(p, key, klen) == 0) {
            size_t vlen = pair_len - klen - 1;
            if (vlen >= out_len) {
                vlen = out_len - 1;
            }
            memcpy(out, p + klen + 1, vlen);
            out[vlen] = '\0';
            url_decode(out);
            return true;
        }
        p = amp ? amp + 1 : NULL;
    }
    out[0] = '\0';
    return false;
}

static void html_escape_attr(const char *in, char *out, size_t out_len)
{
    size_t j = 0;
    for (size_t i = 0; in && in[i] && j + 1 < out_len; i++) {
        char c = in[i];
        if (c == '"' || c == '<' || c == '>' || c == '&') {
            if (j + 5 >= out_len) {
                break;
            }
            out[j++] = '?';
        } else {
            out[j++] = c;
        }
    }
    out[j] = '\0';
}

static esp_err_t send_html(httpd_req_t *req, const char *html)
{
    httpd_resp_set_type(req, "text/html; charset=utf-8");
    httpd_resp_set_hdr(req, "Cache-Control", "no-store");
    return httpd_resp_send(req, html, HTTPD_RESP_USE_STRLEN);
}

static esp_err_t redirect_root(httpd_req_t *req)
{
    httpd_resp_set_status(req, "302 Found");
    httpd_resp_set_hdr(req, "Location", "/");
    return httpd_resp_send(req, NULL, 0);
}

static esp_err_t render_form(httpd_req_t *req)
{
    char ssid_e[DESK_WIFI_SSID_MAX * 2];
    char url_e[DESK_OPENAI_URL_MAX * 2];
    char ap_ip[16] = {0};
    char sta_ip[16] = {0};
    wifi_mgr_get_ap_ip(ap_ip, sizeof(ap_ip));
    wifi_mgr_get_sta_ip(sta_ip, sizeof(sta_ip));
    html_escape_attr(s_cfg.wifi_ssid, ssid_e, sizeof(ssid_e));
    html_escape_attr(s_cfg.openai_base_url, url_e, sizeof(url_e));

    const char *mode =
        wifi_mgr_get_mode() == WIFI_MGR_MODE_STA ? "STA (home Wi‑Fi)" :
        wifi_mgr_get_mode() == WIFI_MGR_MODE_SOFTAP ? "SoftAP setup" : "…";

    /* Keys are never echoed back — blank means “keep existing”. */
    char *page = NULL;
    size_t cap = 6144;
    page = calloc(1, cap);
    if (!page) {
        return httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "oom");
    }

    snprintf(page, cap,
             "<!doctype html><html lang=\"en\"><head><meta charset=\"utf-8\">"
             "<meta name=\"viewport\" content=\"width=device-width,initial-scale=1\">"
             "<title>Desk Buddy · Settings</title>"
             "<style>"
             "body{font-family:system-ui,sans-serif;margin:1.5rem;background:#0b1016;color:#e8eef4;}"
             "h1{font-size:1.4rem;margin:0 0 .25rem} .sub{color:#9aa7b5;margin-bottom:1.25rem}"
             "label{display:block;margin:.85rem 0 .35rem;color:#c5d0da;font-size:.9rem}"
             "input,select{width:100%%;max-width:28rem;box-sizing:border-box;padding:.55rem .65rem;"
             "border-radius:8px;border:1px solid #2a3340;background:#141b24;color:#f2f5f8}"
             "button{margin-top:1.25rem;padding:.65rem 1.2rem;border:0;border-radius:10px;"
             "background:#3d8bfd;color:#fff;font-weight:600;cursor:pointer}"
             ".hint{color:#7f8b99;font-size:.8rem;margin-top:.25rem}"
             ".box{max-width:28rem;padding:1rem 0}"
             "</style></head><body><div class=\"box\">"
             "<h1>Desk Buddy</h1>"
             "<p class=\"sub\">Wi‑Fi + API keys · Réglages Wi‑Fi et clés API<br>"
             "Mode: %s · AP %s · STA %s</p>"
             "<form method=\"POST\" action=\"/save\" autocomplete=\"off\">"
             "<label>Wi‑Fi SSID</label>"
             "<input name=\"wifi_ssid\" value=\"%s\" maxlength=\"31\" required>"
             "<label>Wi‑Fi password / Mot de passe</label>"
             "<input name=\"wifi_pass\" type=\"password\" maxlength=\"63\" "
             "placeholder=\"(unchanged if blank / inchangé si vide)\">"
             "<label>OpenAI-compatible base URL</label>"
             "<input name=\"openai_url\" value=\"%s\" maxlength=\"127\" required>"
             "<label>OpenAI-compatible API key</label>"
             "<input name=\"openai_key\" type=\"password\" maxlength=\"127\" "
             "placeholder=\"%s\">"
             "<label>TTS provider (stub) / Fournisseur TTS</label>"
             "<select name=\"tts_prov\">"
             "<option value=\"openai\" %s>OpenAI-compatible</option>"
             "<option value=\"elevenlabs\" %s>ElevenLabs (stored, unused yet)</option>"
             "</select>"
             "<p class=\"hint\">ElevenLabs path is saved only — no TTS calls in this build.</p>"
             "<label>ElevenLabs API key</label>"
             "<input name=\"eleven_key\" type=\"password\" maxlength=\"127\" "
             "placeholder=\"%s\">"
             "<button type=\"submit\">Save &amp; reboot · Enregistrer</button>"
             "</form>"
             "<p class=\"hint\">After join: http://desk-buddy.local/ or device IP. "
             "No LLM / STT / TTS cloud calls yet.</p>"
             "</div></body></html>",
             mode,
             ap_ip[0] ? ap_ip : "—",
             sta_ip[0] ? sta_ip : "—",
             ssid_e,
             url_e,
             s_cfg.openai_api_key[0] ? "(set — leave blank to keep)" : "(not set)",
             s_cfg.tts_provider == DESK_TTS_PROVIDER_OPENAI ? "selected" : "",
             s_cfg.tts_provider == DESK_TTS_PROVIDER_ELEVENLABS ? "selected" : "",
             s_cfg.elevenlabs_api_key[0] ? "(set — leave blank to keep)" : "(not set)");

    esp_err_t err = send_html(req, page);
    free(page);
    return err;
}

static esp_err_t root_get(httpd_req_t *req)
{
    return render_form(req);
}

static esp_err_t captive_get(httpd_req_t *req)
{
    return redirect_root(req);
}

static esp_err_t save_post(httpd_req_t *req)
{
    int total = req->content_len;
    if (total <= 0 || total > 4096) {
        return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "bad body");
    }
    char *body = calloc(1, (size_t)total + 1);
    if (!body) {
        return httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "oom");
    }
    int received = 0;
    while (received < total) {
        int r = httpd_req_recv(req, body + received, total - received);
        if (r <= 0) {
            free(body);
            return httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "recv");
        }
        received += r;
    }

    desk_config_t next = s_cfg;
    char tmp[DESK_API_KEY_MAX];

    form_get(body, "wifi_ssid", next.wifi_ssid, sizeof(next.wifi_ssid));
    if (form_get(body, "wifi_pass", tmp, sizeof(tmp)) && tmp[0]) {
        strncpy(next.wifi_pass, tmp, sizeof(next.wifi_pass) - 1);
    }
    form_get(body, "openai_url", next.openai_base_url, sizeof(next.openai_base_url));
    if (form_get(body, "openai_key", tmp, sizeof(tmp)) && tmp[0]) {
        strncpy(next.openai_api_key, tmp, sizeof(next.openai_api_key) - 1);
    }
    if (form_get(body, "eleven_key", tmp, sizeof(tmp)) && tmp[0]) {
        strncpy(next.elevenlabs_api_key, tmp, sizeof(next.elevenlabs_api_key) - 1);
    }
    char prov[32] = {0};
    form_get(body, "tts_prov", prov, sizeof(prov));
    next.tts_provider = (strcmp(prov, "elevenlabs") == 0)
                            ? DESK_TTS_PROVIDER_ELEVENLABS
                            : DESK_TTS_PROVIDER_OPENAI;
    free(body);

    if (next.wifi_ssid[0] == '\0') {
        return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "ssid required");
    }
    if (next.openai_base_url[0] == '\0') {
        strncpy(next.openai_base_url, DESK_OPENAI_URL_DEFAULT, sizeof(next.openai_base_url) - 1);
    }

    esp_err_t err = desk_config_save(&next);
    if (err != ESP_OK) {
        return httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "nvs save failed");
    }
    s_cfg = next;

    const char *ok =
        "<!doctype html><html><body style=\"font-family:system-ui;background:#0b1016;color:#e8eef4;"
        "padding:2rem\"><h1>Saved</h1><p>Rebooting to apply Wi‑Fi… / Redémarrage…</p>"
        "<p>Then open <code>http://desk-buddy.local/</code> or the device IP.</p></body></html>";
    send_html(req, ok);
    ESP_LOGI(TAG, "Settings saved — reboot in 800ms (no cloud calls)");
    vTaskDelay(pdMS_TO_TICKS(800));
    esp_restart();
    return ESP_OK;
}

esp_err_t settings_http_start(const desk_config_t *initial)
{
    if (initial) {
        s_cfg = *initial;
    } else {
        desk_config_set_defaults(&s_cfg);
    }

    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.server_port = 80;
    config.max_uri_handlers = 16;
    config.lru_purge_enable = true;

    esp_err_t err = httpd_start(&s_server, &config);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "httpd_start: %s", esp_err_to_name(err));
        return err;
    }

    const httpd_uri_t root = {.uri = "/", .method = HTTP_GET, .handler = root_get};
    const httpd_uri_t save = {.uri = "/save", .method = HTTP_POST, .handler = save_post};
    /* Common captive-portal probe URLs → redirect to form */
    const httpd_uri_t c1 = {.uri = "/generate_204", .method = HTTP_GET, .handler = captive_get};
    const httpd_uri_t c2 = {.uri = "/gen_204", .method = HTTP_GET, .handler = captive_get};
    const httpd_uri_t c3 = {.uri = "/hotspot-detect.html", .method = HTTP_GET, .handler = captive_get};
    const httpd_uri_t c4 = {.uri = "/library/test/success.html", .method = HTTP_GET, .handler = captive_get};
    const httpd_uri_t c5 = {.uri = "/ncsi.txt", .method = HTTP_GET, .handler = captive_get};
    const httpd_uri_t c6 = {.uri = "/connecttest.txt", .method = HTTP_GET, .handler = captive_get};

    httpd_register_uri_handler(s_server, &root);
    httpd_register_uri_handler(s_server, &save);
    httpd_register_uri_handler(s_server, &c1);
    httpd_register_uri_handler(s_server, &c2);
    httpd_register_uri_handler(s_server, &c3);
    httpd_register_uri_handler(s_server, &c4);
    httpd_register_uri_handler(s_server, &c5);
    httpd_register_uri_handler(s_server, &c6);

    ESP_LOGI(TAG, "Settings HTTP on :80");
    return ESP_OK;
}

esp_err_t settings_mdns_start(void)
{
    esp_err_t err = mdns_init();
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "mdns_init: %s", esp_err_to_name(err));
        return err;
    }
    mdns_hostname_set("desk-buddy");
    mdns_instance_name_set("Desk Buddy");
    mdns_service_add(NULL, "_http", "_tcp", 80, NULL, 0);
    ESP_LOGI(TAG, "mDNS http://desk-buddy.local/");
    return ESP_OK;
}
