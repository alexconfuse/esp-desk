/**
 * Desk Buddy PC/host simulator — full face loop with mock chat pipeline.
 *
 * Reuses main/face + main/ui. Host-only session fills Talk/Type:
 *   listening → thinking → speaking + emotion → idle
 * Mock STT/LLM/TTS by default; optional live OpenAI-compatible chat via env.
 */

#define SDL_MAIN_HANDLED
#include "SDL.h"

#include "action_panel.h"
#include "face_ui.h"
#include "lvgl.h"
#include "sim_config.h"
#include "sim_session.h"
#include "sim_settings_ui.h"
#include "sim_type_ui.h"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#ifndef DESK_BUDDY_SIM_HOR
#define DESK_BUDDY_SIM_HOR 480
#endif
#ifndef DESK_BUDDY_SIM_VER
#define DESK_BUDDY_SIM_VER 480
#endif

static sim_config_t s_cfg;

static void on_type_submit(const char *text)
{
    if (!sim_session_request_text(text)) {
        action_panel_set_status("Busy - wait for this turn");
    }
}

static void on_talk(void)
{
    if (!sim_session_request_talk()) {
        action_panel_set_status("Busy - wait for this turn");
    }
}

static void on_type(void)
{
    if (sim_session_is_busy()) {
        action_panel_set_status("Busy - wait for this turn");
        return;
    }
    sim_type_ui_show();
}

static void on_face_tap_extra(void)
{
    if (sim_session_is_listening()) {
        sim_session_cancel_listen();
    }
}

static void on_sim_key(lv_event_t *e)
{
    if (lv_event_get_key(e) != LV_KEY_ESC) {
        return;
    }
    if (sim_session_is_busy()) {
        if (sim_session_is_listening()) {
            sim_session_cancel_listen();
        }
        return;
    }
    if (face_ui_get_emotion() != FACE_EMOTION_IDLE) {
        printf("[I][sim] Esc → idle\n");
        face_ui_set_emotion(FACE_EMOTION_IDLE);
        face_ui_set_caption("");
    }
}

typedef struct {
    int stage;
    uint32_t elapsed_ms;
    bool enabled;
} sim_demo_t;

static sim_demo_t s_demo;

static void demo_tick(uint32_t dt_ms)
{
    if (!s_demo.enabled) {
        return;
    }
    s_demo.elapsed_ms += dt_ms;

    /* Auto-drive one Type turn for headless smoke / screenshots. */
    if (s_demo.stage == 0 && s_demo.elapsed_ms >= 400) {
        face_ui_set_emotion(FACE_EMOTION_CURIOUS);
        s_demo.stage = 1;
    } else if (s_demo.stage == 1 && s_demo.elapsed_ms >= 900) {
        sim_session_request_text("Bonjour! Comment ca va?");
        s_demo.stage = 2;
    } else if (s_demo.stage == 2 && !sim_session_is_busy() && s_demo.elapsed_ms >= 2000) {
        printf("[I][sim] demo loop finished — exiting\n");
        exit(0);
    } else if (s_demo.elapsed_ms > 20000) {
        fprintf(stderr, "[E][sim] demo timed out\n");
        exit(2);
    }
}

int main(int argc, char **argv)
{
    (void)argc;
    (void)argv;

    srand((unsigned)time(NULL));
    sim_config_init(&s_cfg);

    const char *demo = getenv("DESK_BUDDY_SIM_DEMO");
    s_demo.enabled = (demo && demo[0] == '1');

    printf("Desk Buddy full PC simulator (LVGL + SDL)\n");
    printf("  Tap face → curious → Talk / Type → listen/think/speak loop.\n");
    printf("  Mode: %s | lang=%s | TTS=%s\n",
           s_cfg.prefer_live ? "prefer live (if key+curl)" : "mock-by-default",
           sim_lang_name(s_cfg.lang),
           sim_tts_name(s_cfg.tts_provider));
    printf("  Esc → idle (when idle turn); Set button → host settings.\n");
    if (s_demo.enabled) {
        printf("  DESK_BUDDY_SIM_DEMO=1 — auto Type turn then exit.\n");
    }

    lv_init();

    lv_display_t *disp = lv_sdl_window_create(DESK_BUDDY_SIM_HOR, DESK_BUDDY_SIM_VER);
    if (!disp) {
        fprintf(stderr, "lv_sdl_window_create failed (is a display available?)\n");
        return 1;
    }
    lv_sdl_window_set_title(disp, "Desk Buddy — full PC sim");

    lv_indev_t *mouse = lv_sdl_mouse_create();
    (void)mouse;
    lv_indev_t *keyboard = lv_sdl_keyboard_create();

    lv_obj_t *scr = lv_screen_active();
    face_ui_create(scr);
    face_ui_set_tap_callback(on_face_tap_extra);

    sim_session_init(&s_cfg);
    action_panel_set_talk_cb(on_talk);
    action_panel_set_type_cb(on_type);

    sim_type_ui_create(scr, on_type_submit);
    sim_settings_ui_create(scr, &s_cfg);
    sim_settings_ui_create_gear(scr);

    lv_obj_add_flag(scr, LV_OBJ_FLAG_CLICKABLE);
    lv_group_t *keys = lv_group_create();
    lv_group_add_obj(keys, scr);
    lv_obj_add_event_cb(scr, on_sim_key, LV_EVENT_KEY, NULL);
    if (keyboard) {
        lv_indev_set_group(keyboard, keys);
    }

    while (1) {
        uint32_t delay_ms = lv_timer_handler();
        if (delay_ms > 50) {
            delay_ms = 50;
        }
        if (delay_ms < 1) {
            delay_ms = 1;
        }
        demo_tick(delay_ms);
        SDL_Delay(delay_ms);
    }

    return 0;
}
