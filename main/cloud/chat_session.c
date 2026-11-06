#include "chat_session.h"

#include <string.h>

#include "action_panel.h"
#include "bsp/esp_mosaico.h"
#include "desk_audio.h"
#include "esp_log.h"
#include "face_ui.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "openai_client.h"
#include "wifi_mgr.h"

static const char *TAG = "chat_session";

#define LISTEN_MS          4500
#define TEXT_MAX           400

typedef enum {
    CHAT_MSG_TALK = 1,
    CHAT_MSG_TEXT,
    CHAT_MSG_CANCEL,
} chat_msg_type_t;

typedef struct {
    chat_msg_type_t type;
    char text[TEXT_MAX];
} chat_msg_t;

static QueueHandle_t s_q;
static const desk_config_t *s_cfg;
static volatile bool s_busy;
static volatile bool s_listening;
static volatile bool s_cancel_listen;

static void show_status(const char *s)
{
    if (bsp_display_lock(-1)) {
        face_ui_set_caption(s);
        action_panel_set_status(s);
        bsp_display_unlock();
    }
}

static void apply_openai_err(openai_err_t err)
{
    if (err == OPENAI_ERR_BAD_KEY) {
        face_ui_set_emotion_locked(FACE_EMOTION_BAD_KEY);
        show_status("Bad API key — check settings");
    } else if (err == OPENAI_ERR_OFFLINE) {
        face_ui_set_emotion_locked(FACE_EMOTION_OFFLINE);
        show_status("Offline — join Wi‑Fi first");
    } else {
        face_ui_set_emotion_locked(FACE_EMOTION_SAD);
        show_status("Something went wrong");
    }
}

static void settle_idle(void)
{
    vTaskDelay(pdMS_TO_TICKS(1200));
    face_ui_set_emotion_locked(FACE_EMOTION_IDLE);
    face_ui_set_caption_locked("");
}

static void run_pipeline_from_text(const char *user_text, bool play_tts)
{
    if (!user_text || !user_text[0]) {
        face_ui_set_emotion_locked(FACE_EMOTION_CURIOUS);
        show_status("I didn't catch that");
        settle_idle();
        return;
    }

    if (!wifi_mgr_sta_connected()) {
        apply_openai_err(OPENAI_ERR_OFFLINE);
        settle_idle();
        return;
    }
    if (!s_cfg || !s_cfg->openai_api_key[0]) {
        apply_openai_err(OPENAI_ERR_BAD_KEY);
        settle_idle();
        return;
    }

    face_ui_set_emotion_locked(FACE_EMOTION_THINKING);
    show_status("Thinking…");

    openai_chat_reply_t reply;
    openai_err_t err = openai_chat_turn(user_text, &reply);
    if (err != OPENAI_OK) {
        apply_openai_err(err);
        settle_idle();
        return;
    }

    face_ui_set_emotion_locked(reply.emotion);
    show_status(reply.text);

    if (play_tts) {
        face_ui_set_emotion_locked(FACE_EMOTION_SPEAKING);
        int16_t *pcm = NULL;
        size_t pcm_bytes = 0;
        err = openai_tts_pcm(reply.text, &pcm, &pcm_bytes);
        if (err == OPENAI_OK && pcm && desk_audio_ready()) {
            desk_audio_play_pcm(pcm, pcm_bytes, DESK_AUDIO_TTS_HZ);
        } else if (err != OPENAI_OK) {
            ESP_LOGW(TAG, "TTS failed (%d) — text still shown", (int)err);
        }
        free(pcm);
        /* Linger on LLM emotion briefly after speech. */
        face_ui_set_emotion_locked(reply.emotion);
    }

    settle_idle();
}

static void handle_talk(void)
{
    if (!wifi_mgr_sta_connected()) {
        apply_openai_err(OPENAI_ERR_OFFLINE);
        settle_idle();
        return;
    }
    if (!s_cfg || !s_cfg->openai_api_key[0]) {
        apply_openai_err(OPENAI_ERR_BAD_KEY);
        settle_idle();
        return;
    }
    if (!desk_audio_ready()) {
        face_ui_set_emotion_locked(FACE_EMOTION_SAD);
        show_status("Mic unavailable — use Type / web chat");
        settle_idle();
        return;
    }

    s_cancel_listen = false;
    s_listening = true;
    face_ui_set_emotion_locked(FACE_EMOTION_LISTENING);
    show_status("Listening… tap to cancel");

    int16_t *pcm = NULL;
    size_t pcm_bytes = 0;
    esp_err_t aerr = desk_audio_record_pcm(LISTEN_MS, &s_cancel_listen, &pcm, &pcm_bytes);
    s_listening = false;

    if (s_cancel_listen) {
        free(pcm);
        face_ui_set_emotion_locked(FACE_EMOTION_CURIOUS);
        show_status("Cancelled");
        vTaskDelay(pdMS_TO_TICKS(600));
        face_ui_set_emotion_locked(FACE_EMOTION_IDLE);
        face_ui_set_caption_locked("");
        return;
    }
    if (aerr != ESP_OK || !pcm) {
        free(pcm);
        face_ui_set_emotion_locked(FACE_EMOTION_SAD);
        show_status("Couldn't hear you");
        settle_idle();
        return;
    }

    face_ui_set_emotion_locked(FACE_EMOTION_THINKING);
    show_status("Transcribing…");

    char user_text[OPENAI_TEXT_MAX];
    openai_err_t err = openai_stt_transcribe(pcm, pcm_bytes, DESK_AUDIO_REC_HZ,
                                             user_text, sizeof(user_text));
    free(pcm);
    if (err != OPENAI_OK) {
        apply_openai_err(err);
        settle_idle();
        return;
    }

    run_pipeline_from_text(user_text, true);
}

static void chat_task(void *arg)
{
    (void)arg;
    openai_client_set_config(s_cfg);
    desk_audio_init(); /* soft-fail OK */

    chat_msg_t msg;
    while (xQueueReceive(s_q, &msg, portMAX_DELAY) == pdTRUE) {
        if (msg.type == CHAT_MSG_CANCEL) {
            s_cancel_listen = true;
            continue;
        }
        s_busy = true;
        if (msg.type == CHAT_MSG_TALK) {
            handle_talk();
        } else if (msg.type == CHAT_MSG_TEXT) {
            run_pipeline_from_text(msg.text, true);
        }
        s_busy = false;
    }
}

static void on_face_tap_extra(void)
{
    if (chat_session_is_listening()) {
        ESP_LOGI(TAG, "Second tap — cancel listen");
        chat_session_cancel_listen();
    }
}

esp_err_t chat_session_start(const desk_config_t *cfg)
{
    s_cfg = cfg;
    s_q = xQueueCreate(4, sizeof(chat_msg_t));
    if (!s_q) {
        return ESP_ERR_NO_MEM;
    }
    face_ui_set_tap_callback(on_face_tap_extra);
    BaseType_t ok = xTaskCreate(chat_task, "chat", 16384, NULL, 5, NULL);
    return (ok == pdPASS) ? ESP_OK : ESP_ERR_NO_MEM;
}

esp_err_t chat_session_request_talk(void)
{
    if (!s_q) {
        return ESP_ERR_INVALID_STATE;
    }
    if (s_busy) {
        return ESP_ERR_INVALID_STATE;
    }
    chat_msg_t m = {.type = CHAT_MSG_TALK};
    return (xQueueSend(s_q, &m, 0) == pdTRUE) ? ESP_OK : ESP_ERR_NO_MEM;
}

esp_err_t chat_session_request_text(const char *text)
{
    if (!s_q || !text || !text[0]) {
        return ESP_ERR_INVALID_ARG;
    }
    if (s_busy) {
        return ESP_ERR_INVALID_STATE;
    }
    chat_msg_t m = {.type = CHAT_MSG_TEXT};
    strncpy(m.text, text, sizeof(m.text) - 1);
    return (xQueueSend(s_q, &m, 0) == pdTRUE) ? ESP_OK : ESP_ERR_NO_MEM;
}

void chat_session_cancel_listen(void)
{
    s_cancel_listen = true;
    if (s_q) {
        chat_msg_t m = {.type = CHAT_MSG_CANCEL};
        xQueueSend(s_q, &m, 0);
    }
}

bool chat_session_is_busy(void)
{
    return s_busy;
}

bool chat_session_is_listening(void)
{
    return s_listening;
}
