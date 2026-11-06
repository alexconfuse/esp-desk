/**
 * Desk Buddy PC/host simulator — LVGL 9 + SDL2.
 *
 * Compiles the same face_ui / action_panel sources as the firmware, with
 * thin host stubs for esp_log / esp_random. No mic, SoftAP, STT/TTS, or BSP.
 *
 * Controls:
 *   Click / tap face  → idle → curious (+ Talk/Type stubs)
 *   Esc               → back to idle (sim-only convenience)
 *   Close window      → quit
 */

#define SDL_MAIN_HANDLED
#include "SDL.h"

#include "face_ui.h"
#include "lvgl.h"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#ifndef DESK_BUDDY_SIM_HOR
#define DESK_BUDDY_SIM_HOR 480
#endif
#ifndef DESK_BUDDY_SIM_VER
#define DESK_BUDDY_SIM_VER 480
#endif

static void on_sim_key(lv_event_t *e)
{
    if (lv_event_get_key(e) != LV_KEY_ESC) {
        return;
    }
    if (face_ui_get_emotion() != FACE_EMOTION_IDLE) {
        printf("[I][sim] Esc → idle\n");
        face_ui_set_emotion(FACE_EMOTION_IDLE);
    }
}

int main(int argc, char **argv)
{
    (void)argc;
    (void)argv;

    srand((unsigned)time(NULL));

    printf("Desk Buddy face simulator (LVGL + SDL)\n");
    printf("  Tap face → curious; Esc → idle; close window to quit.\n");
    printf("  Simulated: face UI + Talk/Type stubs.\n");
    printf("  Not simulated: mic, STT/TTS, SoftAP, Wi-Fi, cloud, BSP display/touch.\n");

    lv_init();

    lv_display_t *disp = lv_sdl_window_create(DESK_BUDDY_SIM_HOR, DESK_BUDDY_SIM_VER);
    if (!disp) {
        fprintf(stderr, "lv_sdl_window_create failed (is a display available?)\n");
        return 1;
    }
    lv_sdl_window_set_title(disp, "Desk Buddy — face sim");

    lv_indev_t *mouse = lv_sdl_mouse_create();
    (void)mouse;
    lv_indev_t *keyboard = lv_sdl_keyboard_create();

    lv_obj_t *scr = lv_screen_active();
    face_ui_create(scr);

    /* Sim-only: Esc returns to idle so the wake loop is easy to re-exercise. */
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
        SDL_Delay(delay_ms);
    }

    return 0;
}
