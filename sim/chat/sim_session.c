#include "sim_session.h"

#include "action_panel.h"
#include "face_ui.h"
#include "lvgl.h"
#include "sim_llm.h"

#include <stdio.h>
#include <string.h>

typedef enum {
    STEP_IDLE = 0,
    STEP_LISTEN,
    STEP_THINK,
    STEP_SPEAK,
    STEP_LINGER,
    STEP_SETTLE,
} sim_step_t;

typedef struct {
    sim_config_t *cfg;
    sim_step_t step;
    bool busy;
    bool listening;
    bool cancel_listen;
    bool from_talk;
    char user_text[320];
    sim_llm_reply_t reply;
    lv_timer_t *timer;
    uint32_t speak_left_ms;
} sim_session_t;

static sim_session_t s_sess;

static void show_status(const char *s)
{
    face_ui_set_caption(s);
    action_panel_set_status(s);
}

static void schedule(uint32_t ms);

static void finish_turn(void)
{
    s_sess.busy = false;
    s_sess.listening = false;
    s_sess.step = STEP_IDLE;
    s_sess.cancel_listen = false;
    s_sess.user_text[0] = '\0';
    memset(&s_sess.reply, 0, sizeof(s_sess.reply));
    if (s_sess.timer) {
        lv_timer_pause(s_sess.timer);
    }
}

static void schedule(uint32_t ms)
{
    if (!s_sess.timer) {
        return;
    }
    lv_timer_set_period(s_sess.timer, ms);
    lv_timer_reset(s_sess.timer);
    lv_timer_resume(s_sess.timer);
}

static void enter_think(void)
{
    s_sess.step = STEP_THINK;
    s_sess.listening = false;
    face_ui_set_emotion(FACE_EMOTION_THINKING);
    show_status(s_sess.from_talk ? "Transcribing... / thinking..." : "Thinking...");
    schedule(700);
}

static void enter_speak(void)
{
    sim_llm_reply(s_sess.cfg, s_sess.user_text, &s_sess.reply);

    face_ui_set_emotion(s_sess.reply.emotion);
    show_status(s_sess.reply.text);

    const char *tts = sim_tts_name(s_sess.cfg->tts_provider);
    if (s_sess.cfg->tts_provider == SIM_TTS_ELEVENLABS) {
        printf("[I][sim_session] TTS stub ElevenLabs (no audio) — emotion style ignored on host\n");
    } else {
        printf("[I][sim_session] TTS stub OpenAI-compatible (no audio)\n");
    }
    (void)tts;

    /* Speaking animation duration ~ text length. */
    size_t n = strlen(s_sess.reply.text);
    s_sess.speak_left_ms = 900u + (uint32_t)n * 35u;
    if (s_sess.speak_left_ms > 4500u) {
        s_sess.speak_left_ms = 4500u;
    }

    s_sess.step = STEP_SPEAK;
    face_ui_set_emotion(FACE_EMOTION_SPEAKING);
    char buf[360];
    snprintf(buf, sizeof(buf), "%s", s_sess.reply.text);
    show_status(buf);
    schedule(120);
}

static void on_timer(lv_timer_t *t)
{
    (void)t;
    /* Pause until the next schedule() — keep the timer alive across turns. */
    lv_timer_pause(s_sess.timer);

    switch (s_sess.step) {
    case STEP_LISTEN:
        if (s_sess.cancel_listen) {
            face_ui_set_emotion(FACE_EMOTION_CURIOUS);
            show_status("Cancelled");
            s_sess.step = STEP_SETTLE;
            schedule(500);
            return;
        }
        sim_llm_mock_stt(s_sess.cfg, s_sess.user_text, sizeof(s_sess.user_text));
        printf("[I][sim_session] mock STT → \"%s\"\n", s_sess.user_text);
        {
            char buf[360];
            snprintf(buf, sizeof(buf), "Heard: %s", s_sess.user_text);
            show_status(buf);
        }
        enter_think();
        break;

    case STEP_THINK:
        enter_speak();
        break;

    case STEP_SPEAK:
        if (s_sess.speak_left_ms <= 120) {
            face_ui_set_emotion(s_sess.reply.emotion);
            s_sess.step = STEP_LINGER;
            schedule(900);
        } else {
            s_sess.speak_left_ms -= 120;
            schedule(120);
        }
        break;

    case STEP_LINGER:
        s_sess.step = STEP_SETTLE;
        schedule(400);
        break;

    case STEP_SETTLE:
        face_ui_set_emotion(FACE_EMOTION_IDLE);
        face_ui_set_caption("");
        action_panel_set_status("Tap face woke me - pick Talk or Type");
        finish_turn();
        break;

    default:
        finish_turn();
        break;
    }
}

void sim_session_init(sim_config_t *cfg)
{
    memset(&s_sess, 0, sizeof(s_sess));
    s_sess.cfg = cfg;
    s_sess.timer = lv_timer_create(on_timer, 1000, NULL);
    lv_timer_pause(s_sess.timer);
}

bool sim_session_request_talk(void)
{
    if (s_sess.busy) {
        return false;
    }
    s_sess.busy = true;
    s_sess.from_talk = true;
    s_sess.cancel_listen = false;
    s_sess.listening = true;
    s_sess.step = STEP_LISTEN;
    face_ui_set_emotion(FACE_EMOTION_LISTENING);
    show_status("Listening... (mock STT) - tap face to cancel");
    schedule(1800);
    return true;
}

bool sim_session_request_text(const char *text)
{
    if (s_sess.busy) {
        return false;
    }
    if (!text || !text[0]) {
        return false;
    }
    s_sess.busy = true;
    s_sess.from_talk = false;
    s_sess.listening = false;
    s_sess.cancel_listen = false;
    snprintf(s_sess.user_text, sizeof(s_sess.user_text), "%s", text);
    enter_think();
    return true;
}

void sim_session_cancel_listen(void)
{
    if (s_sess.listening) {
        s_sess.cancel_listen = true;
        printf("[I][sim_session] cancel listen\n");
    }
}

bool sim_session_is_busy(void)
{
    return s_sess.busy;
}

bool sim_session_is_listening(void)
{
    return s_sess.listening;
}

sim_config_t *sim_session_config(void)
{
    return s_sess.cfg;
}
