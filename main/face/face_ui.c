#include "face_ui.h"

#include "action_panel.h"

#include "esp_log.h"
#include "esp_random.h"

static const char *TAG = "face_ui";

#define FACE_BG           0x0B1016
#define FACE_EYE_WHITE    0xF4F7FA
#define FACE_PUPIL        0x121820
#define FACE_CHEEK        0x3A4A5C

typedef struct {
    lv_obj_t *root;
    lv_obj_t *face;
    lv_obj_t *left_eye;
    lv_obj_t *right_eye;
    lv_obj_t *left_pupil;
    lv_obj_t *right_pupil;
    lv_obj_t *left_cheek;
    lv_obj_t *right_cheek;
    face_emotion_t emotion;
    lv_timer_t *blink_timer;
    lv_anim_t breathe_anim;
} face_ui_t;

static face_ui_t s_face;

static void set_eye_open(lv_obj_t *eye, int32_t open_h)
{
    lv_obj_set_height(eye, open_h);
}

static void blink_once(void)
{
    const int32_t open_h = (s_face.emotion == FACE_EMOTION_CURIOUS) ? 92 : 78;
    const int32_t shut_h = 8;

    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_exec_cb(&a, (lv_anim_exec_xcb_t)set_eye_open);
    lv_anim_set_values(&a, open_h, shut_h);
    lv_anim_set_duration(&a, 90);
    lv_anim_set_playback_duration(&a, 110);
    lv_anim_set_repeat_count(&a, 1);

    lv_anim_set_var(&a, s_face.left_eye);
    lv_anim_start(&a);
    lv_anim_set_var(&a, s_face.right_eye);
    lv_anim_start(&a);
}

static void blink_timer_cb(lv_timer_t *timer)
{
    (void)timer;
    if (s_face.emotion != FACE_EMOTION_IDLE) {
        return;
    }
    blink_once();
    /* Irregular blink cadence feels more alive than a fixed period. */
    uint32_t next_ms = 1800 + (esp_random() % 3200);
    lv_timer_set_period(s_face.blink_timer, next_ms);
}

static void breathe_exec(void *obj, int32_t v)
{
    lv_obj_set_style_transform_scale((lv_obj_t *)obj, v, 0);
}

static void start_breathe(bool curious)
{
    lv_anim_delete(s_face.face, breathe_exec);

    lv_anim_init(&s_face.breathe_anim);
    lv_anim_set_var(&s_face.breathe_anim, s_face.face);
    lv_anim_set_exec_cb(&s_face.breathe_anim, breathe_exec);
    /* LVGL scale uses 256 = 100%. */
    lv_anim_set_values(&s_face.breathe_anim,
                       curious ? 248 : 246,
                       curious ? 268 : 262);
    lv_anim_set_duration(&s_face.breathe_anim, curious ? 1400 : 2200);
    lv_anim_set_playback_duration(&s_face.breathe_anim, curious ? 1400 : 2200);
    lv_anim_set_repeat_count(&s_face.breathe_anim, LV_ANIM_REPEAT_INFINITE);
    lv_anim_set_path_cb(&s_face.breathe_anim, lv_anim_path_ease_in_out);
    lv_anim_start(&s_face.breathe_anim);
}

static void place_pupils(face_emotion_t emotion)
{
    /* Idle: soft center. Curious: look slightly up-right. */
    int32_t dx = (emotion == FACE_EMOTION_CURIOUS) ? 10 : 0;
    int32_t dy = (emotion == FACE_EMOTION_CURIOUS) ? -12 : 0;
    lv_obj_align(s_face.left_pupil, LV_ALIGN_CENTER, dx, dy);
    lv_obj_align(s_face.right_pupil, LV_ALIGN_CENTER, dx, dy);
}

static lv_obj_t *make_eye(lv_obj_t *parent, int32_t x_ofs)
{
    lv_obj_t *eye = lv_obj_create(parent);
    lv_obj_remove_style_all(eye);
    lv_obj_set_size(eye, 88, 78);
    lv_obj_set_style_radius(eye, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(eye, lv_color_hex(FACE_EYE_WHITE), 0);
    lv_obj_set_style_bg_opa(eye, LV_OPA_COVER, 0);
    lv_obj_set_style_clip_corner(eye, true, 0);
    lv_obj_align(eye, LV_ALIGN_CENTER, x_ofs, -18);
    lv_obj_clear_flag(eye, LV_OBJ_FLAG_SCROLLABLE);
    return eye;
}

static lv_obj_t *make_pupil(lv_obj_t *eye)
{
    lv_obj_t *pupil = lv_obj_create(eye);
    lv_obj_remove_style_all(pupil);
    lv_obj_set_size(pupil, 34, 34);
    lv_obj_set_style_radius(pupil, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(pupil, lv_color_hex(FACE_PUPIL), 0);
    lv_obj_set_style_bg_opa(pupil, LV_OPA_COVER, 0);
    lv_obj_align(pupil, LV_ALIGN_CENTER, 0, 0);
    lv_obj_clear_flag(pupil, LV_OBJ_FLAG_SCROLLABLE);
    return pupil;
}

static lv_obj_t *make_cheek(lv_obj_t *parent, int32_t x_ofs)
{
    lv_obj_t *cheek = lv_obj_create(parent);
    lv_obj_remove_style_all(cheek);
    lv_obj_set_size(cheek, 42, 18);
    lv_obj_set_style_radius(cheek, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(cheek, lv_color_hex(FACE_CHEEK), 0);
    lv_obj_set_style_bg_opa(cheek, LV_OPA_40, 0);
    lv_obj_align(cheek, LV_ALIGN_CENTER, x_ofs, 58);
    lv_obj_clear_flag(cheek, LV_OBJ_FLAG_SCROLLABLE);
    return cheek;
}

static void on_face_tap(lv_event_t *e)
{
    (void)e;
    if (face_ui_get_emotion() == FACE_EMOTION_IDLE) {
        ESP_LOGI(TAG, "Tap wake → curious");
        face_ui_set_emotion(FACE_EMOTION_CURIOUS);
    }
}

void face_ui_create(lv_obj_t *parent)
{
    s_face.root = parent;
    s_face.emotion = FACE_EMOTION_IDLE;

    lv_obj_set_style_bg_color(parent, lv_color_hex(FACE_BG), 0);
    lv_obj_set_style_bg_opa(parent, LV_OPA_COVER, 0);
    lv_obj_clear_flag(parent, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(parent, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(parent, on_face_tap, LV_EVENT_CLICKED, NULL);

    s_face.face = lv_obj_create(parent);
    lv_obj_remove_style_all(s_face.face);
    lv_obj_set_size(s_face.face, 320, 320);
    lv_obj_center(s_face.face);
    lv_obj_set_style_transform_pivot_x(s_face.face, 160, 0);
    lv_obj_set_style_transform_pivot_y(s_face.face, 160, 0);
    lv_obj_clear_flag(s_face.face, LV_OBJ_FLAG_SCROLLABLE);
    /* Let taps bubble to the screen. */
    lv_obj_clear_flag(s_face.face, LV_OBJ_FLAG_CLICKABLE);

    s_face.left_eye = make_eye(s_face.face, -70);
    s_face.right_eye = make_eye(s_face.face, 70);
    s_face.left_pupil = make_pupil(s_face.left_eye);
    s_face.right_pupil = make_pupil(s_face.right_eye);
    s_face.left_cheek = make_cheek(s_face.face, -78);
    s_face.right_cheek = make_cheek(s_face.face, 78);

    action_panel_create(parent);

    s_face.blink_timer = lv_timer_create(blink_timer_cb, 2400, NULL);
    start_breathe(false);
    place_pupils(FACE_EMOTION_IDLE);

    ESP_LOGI(TAG, "Face UI ready (idle blink + breathe)");
}

void face_ui_set_emotion(face_emotion_t emotion)
{
    if (s_face.emotion == emotion) {
        return;
    }
    s_face.emotion = emotion;

    const int32_t open_h = (emotion == FACE_EMOTION_CURIOUS) ? 92 : 78;
    const int32_t open_w = (emotion == FACE_EMOTION_CURIOUS) ? 96 : 88;
    lv_obj_set_size(s_face.left_eye, open_w, open_h);
    lv_obj_set_size(s_face.right_eye, open_w, open_h);
    place_pupils(emotion);
    start_breathe(emotion == FACE_EMOTION_CURIOUS);

    const bool curious = (emotion == FACE_EMOTION_CURIOUS);
    lv_obj_set_style_bg_opa(s_face.left_cheek, curious ? LV_OPA_60 : LV_OPA_40, 0);
    lv_obj_set_style_bg_opa(s_face.right_cheek, curious ? LV_OPA_60 : LV_OPA_40, 0);
    action_panel_show(curious);

    ESP_LOGI(TAG, "Emotion → %s", curious ? "curious" : "idle");
}

face_emotion_t face_ui_get_emotion(void)
{
    return s_face.emotion;
}
