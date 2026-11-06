#include "openai_client.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cJSON.h"
#include "esp_crt_bundle.h"
#include "esp_http_client.h"
#include "esp_log.h"
#include "wifi_mgr.h"

static const char *TAG = "openai";

static desk_config_t s_cfg;
static bool s_have_cfg;

void openai_client_set_config(const desk_config_t *cfg)
{
    if (cfg) {
        s_cfg = *cfg;
        s_have_cfg = true;
    }
}

static void trim_slash(char *url)
{
    size_t n = strlen(url);
    while (n > 0 && url[n - 1] == '/') {
        url[--n] = '\0';
    }
}

static esp_err_t join_url(char *out, size_t out_len, const char *path)
{
    if (!s_have_cfg || !s_cfg.openai_base_url[0]) {
        return ESP_ERR_INVALID_STATE;
    }
    char base[DESK_OPENAI_URL_MAX];
    strncpy(base, s_cfg.openai_base_url, sizeof(base) - 1);
    base[sizeof(base) - 1] = '\0';
    trim_slash(base);
    int n = snprintf(out, out_len, "%s%s", base, path);
    return (n > 0 && (size_t)n < out_len) ? ESP_OK : ESP_ERR_INVALID_SIZE;
}

typedef struct {
    uint8_t *data;
    size_t len;
    size_t cap;
} buf_t;

static esp_err_t buf_append(buf_t *b, const void *data, int len)
{
    if (len <= 0) {
        return ESP_OK;
    }
    if (b->len + (size_t)len + 1 > b->cap) {
        size_t ncap = b->cap ? b->cap * 2 : 4096;
        while (ncap < b->len + (size_t)len + 1) {
            ncap *= 2;
        }
        uint8_t *p = realloc(b->data, ncap);
        if (!p) {
            return ESP_ERR_NO_MEM;
        }
        b->data = p;
        b->cap = ncap;
    }
    memcpy(b->data + b->len, data, (size_t)len);
    b->len += (size_t)len;
    b->data[b->len] = 0;
    return ESP_OK;
}

static openai_err_t map_http_status(int status)
{
    if (status == 401 || status == 403) {
        return OPENAI_ERR_BAD_KEY;
    }
    if (status < 0) {
        return OPENAI_ERR_OFFLINE;
    }
    if (status >= 200 && status < 300) {
        return OPENAI_OK;
    }
    return OPENAI_ERR_HTTP;
}

static esp_err_t http_event(esp_http_client_event_t *evt)
{
    buf_t *b = (buf_t *)evt->user_data;
    if (evt->event_id == HTTP_EVENT_ON_DATA && b) {
        return buf_append(b, evt->data, evt->data_len);
    }
    return ESP_OK;
}

static openai_err_t http_post(const char *url, const char *content_type,
                              const char *body, size_t body_len,
                              buf_t *resp, int *status_out)
{
    if (!wifi_mgr_sta_connected()) {
        return OPENAI_ERR_OFFLINE;
    }
    if (!s_have_cfg || !s_cfg.openai_api_key[0]) {
        return OPENAI_ERR_BAD_KEY;
    }

    esp_http_client_config_t cfg = {
        .url = url,
        .method = HTTP_METHOD_POST,
        .timeout_ms = 60000,
        .crt_bundle_attach = esp_crt_bundle_attach,
        .event_handler = http_event,
        .user_data = resp,
        .buffer_size = 2048,
        .buffer_size_tx = 2048,
    };
    esp_http_client_handle_t client = esp_http_client_init(&cfg);
    if (!client) {
        return OPENAI_ERR_NO_MEM;
    }

    char auth[DESK_API_KEY_MAX + 32];
    snprintf(auth, sizeof(auth), "Bearer %s", s_cfg.openai_api_key);
    esp_http_client_set_header(client, "Authorization", auth);
    esp_http_client_set_header(client, "Content-Type", content_type);
    esp_http_client_set_post_field(client, body, (int)body_len);

    esp_err_t err = esp_http_client_perform(client);
    int status = esp_http_client_get_status_code(client);
    if (status_out) {
        *status_out = status;
    }
    esp_http_client_cleanup(client);

    if (err == ESP_ERR_HTTP_CONNECT || err == ESP_ERR_TIMEOUT) {
        return OPENAI_ERR_OFFLINE;
    }
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "HTTP perform: %s", esp_err_to_name(err));
        return OPENAI_ERR_OFFLINE;
    }
    return map_http_status(status);
}

static void write_wav_header(uint8_t *hdr, size_t pcm_bytes, int hz)
{
    uint32_t byte_rate = (uint32_t)hz * 2;
    uint32_t data_sz = (uint32_t)pcm_bytes;
    uint32_t riff_sz = 36 + data_sz;
    memcpy(hdr, "RIFF", 4);
    hdr[4] = riff_sz; hdr[5] = riff_sz >> 8; hdr[6] = riff_sz >> 16; hdr[7] = riff_sz >> 24;
    memcpy(hdr + 8, "WAVEfmt ", 8);
    hdr[16] = 16; hdr[17] = 0; hdr[18] = 0; hdr[19] = 0; /* chunk size */
    hdr[20] = 1; hdr[21] = 0;                             /* PCM */
    hdr[22] = 1; hdr[23] = 0;                             /* mono */
    hdr[24] = hz; hdr[25] = hz >> 8; hdr[26] = hz >> 16; hdr[27] = hz >> 24;
    hdr[28] = byte_rate; hdr[29] = byte_rate >> 8; hdr[30] = byte_rate >> 16; hdr[31] = byte_rate >> 24;
    hdr[32] = 2; hdr[33] = 0; /* block align */
    hdr[34] = 16; hdr[35] = 0; /* bits */
    memcpy(hdr + 36, "data", 4);
    hdr[40] = data_sz; hdr[41] = data_sz >> 8; hdr[42] = data_sz >> 16; hdr[43] = data_sz >> 24;
}

openai_err_t openai_stt_transcribe(const int16_t *pcm, size_t pcm_bytes,
                                   int sample_hz, char *out_text, size_t out_len)
{
    if (!pcm || !out_text || out_len == 0) {
        return OPENAI_ERR_PARAM;
    }
    out_text[0] = '\0';

    char url[DESK_OPENAI_URL_MAX + 64];
    if (join_url(url, sizeof(url), "/audio/transcriptions") != ESP_OK) {
        return OPENAI_ERR_PARAM;
    }

    const char *boundary = "----DeskBuddyBoundary7a3f";
    char head[512];
    int head_len = snprintf(head, sizeof(head),
                            "--%s\r\n"
                            "Content-Disposition: form-data; name=\"model\"\r\n\r\n"
                            "whisper-1\r\n"
                            "--%s\r\n"
                            "Content-Disposition: form-data; name=\"response_format\"\r\n\r\n"
                            "json\r\n"
                            "--%s\r\n"
                            "Content-Disposition: form-data; name=\"file\"; filename=\"speech.wav\"\r\n"
                            "Content-Type: audio/wav\r\n\r\n",
                            boundary, boundary, boundary);
    char mid[128];
    /* Optional language omitted → provider auto-detect (EN/FR). */
    int mid_len = snprintf(mid, sizeof(mid),
                           "\r\n--%s--\r\n", boundary);

    size_t wav_bytes = 44 + pcm_bytes;
    size_t total = (size_t)head_len + wav_bytes + (size_t)mid_len;
    uint8_t *body = malloc(total);
    if (!body) {
        return OPENAI_ERR_NO_MEM;
    }
    memcpy(body, head, (size_t)head_len);
    write_wav_header(body + head_len, pcm_bytes, sample_hz);
    memcpy(body + head_len + 44, pcm, pcm_bytes);
    memcpy(body + head_len + wav_bytes, mid, (size_t)mid_len);

    char ctype[128];
    snprintf(ctype, sizeof(ctype), "multipart/form-data; boundary=%s", boundary);

    buf_t resp = {0};
    int status = 0;
    openai_err_t oerr = http_post(url, ctype, (const char *)body, total, &resp, &status);
    free(body);
    if (oerr != OPENAI_OK) {
        ESP_LOGE(TAG, "STT HTTP status=%d err=%d", status, (int)oerr);
        free(resp.data);
        return oerr;
    }

    cJSON *root = cJSON_Parse((const char *)resp.data);
    free(resp.data);
    if (!root) {
        return OPENAI_ERR_PARSE;
    }
    const cJSON *text = cJSON_GetObjectItemCaseSensitive(root, "text");
    if (cJSON_IsString(text) && text->valuestring) {
        strncpy(out_text, text->valuestring, out_len - 1);
        out_text[out_len - 1] = '\0';
    }
    cJSON_Delete(root);
    ESP_LOGI(TAG, "STT: \"%s\"", out_text);
    return out_text[0] ? OPENAI_OK : OPENAI_ERR_PARSE;
}

static const char *SYSTEM_PROMPT =
    "You are Desk Buddy, a warm square-faced desk pet. "
    "Reply in the SAME language as the user (English or French; if mixed, follow the dominant clause). "
    "Respond with ONLY a JSON object: "
    "{\"emotion\":\"happy|sad|curious|annoyed|surprised|thinking|sleepy|speaking\","
    "\"intensity\":0.0-1.0,\"text\":\"short spoken reply\"}. "
    "Keep text to one or two short sentences. Be expressive but kind.";

openai_err_t openai_chat_turn(const char *user_text, openai_chat_reply_t *out)
{
    if (!user_text || !out) {
        return OPENAI_ERR_PARAM;
    }
    memset(out, 0, sizeof(*out));
    out->emotion = FACE_EMOTION_CURIOUS;
    out->intensity = 0.5f;

    char url[DESK_OPENAI_URL_MAX + 64];
    if (join_url(url, sizeof(url), "/chat/completions") != ESP_OK) {
        return OPENAI_ERR_PARAM;
    }

    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "model", "gpt-4o-mini");
    cJSON_AddNumberToObject(root, "temperature", 0.7);
    cJSON *fmt = cJSON_AddObjectToObject(root, "response_format");
    cJSON_AddStringToObject(fmt, "type", "json_object");
    cJSON *msgs = cJSON_AddArrayToObject(root, "messages");
    cJSON *sys = cJSON_CreateObject();
    cJSON_AddStringToObject(sys, "role", "system");
    cJSON_AddStringToObject(sys, "content", SYSTEM_PROMPT);
    cJSON_AddItemToArray(msgs, sys);
    cJSON *usr = cJSON_CreateObject();
    cJSON_AddStringToObject(usr, "role", "user");
    cJSON_AddStringToObject(usr, "content", user_text);
    cJSON_AddItemToArray(msgs, usr);

    char *body = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    if (!body) {
        return OPENAI_ERR_NO_MEM;
    }

    buf_t resp = {0};
    int status = 0;
    openai_err_t oerr = http_post(url, "application/json", body, strlen(body), &resp, &status);
    free(body);
    if (oerr != OPENAI_OK) {
        ESP_LOGE(TAG, "Chat HTTP status=%d err=%d", status, (int)oerr);
        free(resp.data);
        return oerr;
    }

    cJSON *j = cJSON_Parse((const char *)resp.data);
    free(resp.data);
    if (!j) {
        return OPENAI_ERR_PARSE;
    }

    const char *content = NULL;
    cJSON *choices = cJSON_GetObjectItemCaseSensitive(j, "choices");
    if (cJSON_IsArray(choices) && cJSON_GetArraySize(choices) > 0) {
        cJSON *msg = cJSON_GetObjectItemCaseSensitive(cJSON_GetArrayItem(choices, 0), "message");
        cJSON *c = cJSON_GetObjectItemCaseSensitive(msg, "content");
        if (cJSON_IsString(c)) {
            content = c->valuestring;
        }
    }
    if (!content) {
        cJSON_Delete(j);
        return OPENAI_ERR_PARSE;
    }

    cJSON *payload = cJSON_Parse(content);
    if (!payload) {
        /* Sometimes models wrap JSON in fences — copy text as-is. */
        strncpy(out->text, content, sizeof(out->text) - 1);
        cJSON_Delete(j);
        return OPENAI_OK;
    }
    cJSON *em = cJSON_GetObjectItemCaseSensitive(payload, "emotion");
    cJSON *tx = cJSON_GetObjectItemCaseSensitive(payload, "text");
    cJSON *it = cJSON_GetObjectItemCaseSensitive(payload, "intensity");
    if (cJSON_IsString(em)) {
        out->emotion = face_ui_emotion_from_str(em->valuestring);
    }
    if (cJSON_IsString(tx) && tx->valuestring) {
        strncpy(out->text, tx->valuestring, sizeof(out->text) - 1);
    }
    if (cJSON_IsNumber(it)) {
        out->intensity = (float)it->valuedouble;
    }
    cJSON_Delete(payload);
    cJSON_Delete(j);
    ESP_LOGI(TAG, "Chat emotion=%d text=\"%s\"", (int)out->emotion, out->text);
    return out->text[0] ? OPENAI_OK : OPENAI_ERR_PARSE;
}

openai_err_t openai_tts_pcm(const char *text, int16_t **out_pcm, size_t *out_bytes)
{
    if (!text || !out_pcm || !out_bytes) {
        return OPENAI_ERR_PARAM;
    }
    *out_pcm = NULL;
    *out_bytes = 0;

    /* MVP TTS path is OpenAI-compatible only (ElevenLabs remains a settings stub). */
    char url[DESK_OPENAI_URL_MAX + 64];
    if (join_url(url, sizeof(url), "/audio/speech") != ESP_OK) {
        return OPENAI_ERR_PARAM;
    }

    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "model", "tts-1");
    cJSON_AddStringToObject(root, "voice", "alloy");
    cJSON_AddStringToObject(root, "input", text);
    cJSON_AddStringToObject(root, "response_format", "pcm");
    char *body = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    if (!body) {
        return OPENAI_ERR_NO_MEM;
    }

    buf_t resp = {0};
    int status = 0;
    openai_err_t oerr = http_post(url, "application/json", body, strlen(body), &resp, &status);
    free(body);
    if (oerr != OPENAI_OK) {
        ESP_LOGE(TAG, "TTS HTTP status=%d err=%d", status, (int)oerr);
        free(resp.data);
        return oerr;
    }
    if (resp.len < 64) {
        free(resp.data);
        return OPENAI_ERR_PARSE;
    }
    *out_pcm = (int16_t *)resp.data;
    *out_bytes = resp.len;
    ESP_LOGI(TAG, "TTS pcm %u bytes", (unsigned)resp.len);
    return OPENAI_OK;
}
