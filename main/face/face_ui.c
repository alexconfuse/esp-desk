#include "face_ui.h"

#include "action_panel.h"

#include "esp_log.h"
#include "esp_random.h"

#include <ctype.h>
#include <string.h>

static const char *TAG = "face_ui";

#define FACE_BG           0x0B1016
#define FACE_EYE_WHITE    0xF4F7FA
#define FACE_PUPIL        0x121820
#define FACE_CHEEK        0x3A4A5C
#define FACE_MOUTH        0xE8EEF4
#define FACE_ERR          0xC44B4B

typedef struct {
    lv_obj_t *root;
    lv_obj_t *face;
    lv_obj_t *left_eye;
    lv_obj_t *right_eye;
    lv_obj_t *left_pupil;
    lv_obj_t *right_pupil;
    lv_obj_t *left_cheek;
    lv_obj_t *right_cheek;
    lv_obj_t *mouth;
    lv_obj_t *caption;
    face_emotion_t emotion;
    lv_timer_t *blink_timer;
    lv_timer_t *speak_timer;
    lv_anim_t breathe_anim;
    face_ui_tap_cb_t tap_cb;
} face_ui_t;

static face_ui_t s_face;

static int ascii_casecmp(const char *a, const char *b)
{
    while (*a && *b) {
        int ca = tolower((unsigned char)*a++);
        int cb = tolower((unsigned char)*b++);
        if (ca != cb) {
            return ca - cb;
        }
    }
    return (unsigned char)*a - (unsigned char)*b;
}

static const char *emotion_name(face_emotion_t e)
{
    switch (e) {
    case FACE_EMOTION_IDLE: return "idle";
    case FACE_EMOTION_CURIOUS: return "curious";
    case FACE_EMOTION_LISTENING: return "listening";
    case FACE_EMOTION_THINKING: return "thinking";
    case FACE_EMOTION_SPEAKING: return "speaking";
    case FACE_EMOTION_HAPPY: return "happy";
    case FACE_EMOTION_SAD: return "sad";
    case FACE_EMOTION_ANNOYED: return "annoyed";
    case FACE_EMOTION_SURPRISED: return "surprised";
    case FACE_EMOTION_SLEEPY: return "sleepy";
    case FACE_EMOTION_OFFLINE: return "offline";
    case FACE_EMOTION_BAD_KEY: return "bad_key";
    default: return "?";
    }
}

face_emotion_t face_ui_emotion_from_str(const char *name)
{
    if (!name || !name[0]) {
        return FACE_EMOTION_CURIOUS;
    }
    if (!ascii_casecmp(name, "idle")) return FACE_EMOTION_IDLE;
    if (!ascii_casecmp(name, "curious")) return FACE_EMOTION_CURIOUS;
    if (!ascii_casecmp(name, "listening")) return FACE_EMOTION_LISTENING;
    if (!ascii_casecmp(name, "thinking")) return FACE_EMOTION_THINKING;
    if (!ascii_casecmp(name, "speaking")) return FACE_EMOTION_SPEAKING;
    if (!ascii_casecmp(name, "happy")) return FACE_EMOTION_HAPPY;
    if (!ascii_casecmp(name, "sad")) return FACE_EMOTION_SAD;
    if (!ascii_casecmp(name, "annoyed") || !ascii_casecmp(name, "angry")) return FACE_EMOTION_ANNOYED;
    if (!ascii_casecmp(name, "surprised")) return FACE_EMOTION_SURPRISED;
    if (!ascii_casecmp(name, "sleepy")) return FACE_EMOTION_SLEEPY;
    if (!ascii_casecmp(name, "offline")) return FACE_EMOTION_OFFLINE;
    if (!ascii_casecmp(name, "bad_key") || !ascii_casecmp(name, "error")) return FACE_EMOTION_BAD_KEY;
    return FACE_EMOTION_CURIOUS;
}

static void set_eye_open(lv_obj_t *eye, int32_t open_h)
{
    lv_obj_set_height(eye, open_h);
}

static void eye_metrics(face_emotion_t e, int32_t *w, int32_t *h, int32_t *pdx, int32_t *pdy)
{
    *w = 88;
    *h = 78;
    *pdx = 0;
    *pdy = 0;
    switch (e) {
    case FACE_EMOTION_CURIOUS:
    case FACE_EMOTION_LISTENING:
        *w = 96; *h = 92; *pdx = 10; *pdy = -12;
        break;
    case FACE_EMOTION_THINKING:
        *w = 90; *h = 70; *pdx = -8; *pdy = -16;
        break;
    case FACE_EMOTION_SPEAKING:
    case FACE_EMOTION_HAPPY:
        *w = 94; *h = 86; *pdy = 4;
        break;
    case FACE_EMOTION_SAD:
    case FACE_EMOTION_OFFLINE:
        *w = 86; *h = 64; *pdy = 14;
        break;
    case FACE_EMOTION_ANNOYED:
    case FACE_EMOTION_BAD_KEY:
        *w = 92; *h = 50; *pdy = -6;
        break;
    case FACE_EMOTION_SURPRISED:
        *w = 100; *h = 100;
        break;
    case FACE_EMOTION_SLEEPY:
        *w = 88; *h = 28;
        break;
    default:
        break;
    }
}

static void blink_once(void)
{
    int32_t w, h, dx, dy;
    eye_metrics(s_face.emotion, &w, &h, &dx, &dy);
    const int32_t shut_h = 8;

    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_exec_cb(&a, (lv_anim_exec_xcb_t)set_eye_open);
    lv_anim_set_values(&a, h, shut_h);
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
    if (s_face.emotion != FACE_EMOTION_IDLE && s_face.emotion != FACE_EMOTION_CURIOUS) {
        return;
    }
    blink_once();
    lv_timer_set_period(s_face.blink_timer, 1800 + (esp_random() % 3200));
}

static void breathe_exec(void *obj, int32_t v)
{
    lv_obj_set_style_transform_scale((lv_obj_t *)obj, v, 0);
}

static void start_breathe(face_emotion_t e)
{
    lv_anim_delete(s_face.face, breathe_exec);
    int lo = 246, hi = 262, ms = 2200;
    if (e == FACE_EMOTION_CURIOUS || e == FACE_EMOTION_LISTENING) {
        lo = 248; hi = 268; ms = 1400;
    } else if (e == FACE_EMOTION_THINKING) {
        lo = 250; hi = 258; ms = 900;
    } else if (e == FACE_EMOTION_SPEAKING || e == FACE_EMOTION_HAPPY) {
        lo = 252; hi = 270; ms = 700;
    } else if (e == FACE_EMOTION_OFFLINE || e == FACE_EMOTION_BAD_KEY || e == FACE_EMOTION_SAD) {
        lo = 244; hi = 250; ms = 2800;
    }
    lv_anim_init(&s_face.breathe_anim);
    lv_anim_set_var(&s_face.breathe_anim, s_face.face);
    lv_anim_set_exec_cb(&s_face.breathe_anim, breathe_exec);
    lv_anim_set_values(&s_face.breathe_anim, lo, hi);
    lv_anim_set_duration(&s_face.breathe_anim, ms);
    lv_anim_set_playback_duration(&s_face.breathe_anim, ms);
    lv_anim_set_repeat_count(&s_face.breathe_anim, LV_ANIM_REPEAT_INFINITE);
    lv_anim_set_path_cb(&s_face.breathe_anim, lv_anim_path_ease_in_out);
    lv_anim_start(&s_face.breathe_anim);
}

static void speak_timer_cb(lv_timer_t *t)
{
    (void)t;
    if (!s_face.mouth || s_face.emotion != FACE_EMOTION_SPEAKING) {
        return;
    }
    int32_t h = 10 + (int32_t)(esp_random() % 22);
    lv_obj_set_height(s_face.mouth, h);
}

static void apply_mouth(face_emotion_t e)
{
    if (!s_face.mouth) {
        return;
    }
    int32_t w = 56, h = 10;
    lv_opa_t opa = LV_OPA_70;
    switch (e) {
    case FACE_EMOTION_SPEAKING: w = 64; h = 18; opa = LV_OPA_COVER; break;
    case FACE_EMOTION_HAPPY: w = 70; h = 16; break;
    case FACE_EMOTION_SAD:
    case FACE_EMOTION_OFFLINE: w = 48; h = 8; break;
    case FACE_EMOTION_ANNOYED:
    case FACE_EMOTION_BAD_KEY: w = 40; h = 6; break;
    case FACE_EMOTION_SURPRISED: w = 36; h = 36; break;
    case FACE_EMOTION_LISTENING: w = 50; h = 8; break;
    default: break;
    }
    lv_obj_set_size(s_face.mouth, w, h);
    lv_obj_set_style_bg_opa(s_face.mouth, opa, 0);
    lv_obj_set_style_bg_color(s_face.mouth,
                              lv_color_hex((e == FACE_EMOTION_BAD_KEY || e == FACE_EMOTION_OFFLINE)
                                               ? FACE_ERR : FACE_MOUTH),
                              0);
}

static void place_pupils(int32_t dx, int32_t dy)
{
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
    if (s_face.tap_cb) {
        s_face.tap_cb();
    }
    if (face_ui_get_emotion() == FACE_EMOTION_IDLE) {
        ESP_LOGI(TAG, "Tap wake → curious");
        face_ui_set_emotion(FACE_EMOTION_CURIOUS);
    }
}

void face_ui_create(lv_obj_t *parent)
{
    memset(&s_face, 0, sizeof(s_face));
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
    lv_obj_clear_flag(s_face.face, LV_OBJ_FLAG_CLICKABLE);

    s_face.left_eye = make_eye(s_face.face, -70);
    s_face.right_eye = make_eye(s_face.face, 70);
    s_face.left_pupil = make_pupil(s_face.left_eye);
    s_face.right_pupil = make_pupil(s_face.right_eye);
    s_face.left_cheek = make_cheek(s_face.face, -78);
    s_face.right_cheek = make_cheek(s_face.face, 78);

    s_face.mouth = lv_obj_create(s_face.face);
    lv_obj_remove_style_all(s_face.mouth);
    lv_obj_set_size(s_face.mouth, 56, 10);
    lv_obj_set_style_radius(s_face.mouth, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(s_face.mouth, lv_color_hex(FACE_MOUTH), 0);
    lv_obj_set_style_bg_opa(s_face.mouth, LV_OPA_70, 0);
    lv_obj_align(s_face.mouth, LV_ALIGN_CENTER, 0, 78);
    lv_obj_clear_flag(s_face.mouth, LV_OBJ_FLAG_SCROLLABLE);

    s_face.caption = lv_label_create(parent);
    lv_label_set_text(s_face.caption, "");
    lv_label_set_long_mode(s_face.caption, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(s_face.caption, 400);
    lv_obj_set_style_text_color(s_face.caption, lv_color_hex(0x9AA7B5), 0);
    lv_obj_set_style_text_font(s_face.caption, &lv_font_montserrat_18, 0);
    lv_obj_set_style_text_align(s_face.caption, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(s_face.caption, LV_ALIGN_BOTTOM_MID, 0, -170);

    action_panel_create(parent);

    s_face.blink_timer = lv_timer_create(blink_timer_cb, 2400, NULL);
    s_face.speak_timer = lv_timer_create(speak_timer_cb, 120, NULL);
    lv_timer_pause(s_face.speak_timer);
    start_breathe(FACE_EMOTION_IDLE);
    place_pupils(0, 0);

    ESP_LOGI(TAG, "Face UI ready");
}

void face_ui_set_emotion(face_emotion_t emotion)
{
    s_face.emotion = emotion;

    int32_t w, h, dx, dy;
    eye_metrics(emotion, &w, &h, &dx, &dy);
    lv_obj_set_size(s_face.left_eye, w, h);
    lv_obj_set_size(s_face.right_eye, w, h);
    place_pupils(dx, dy);
    start_breathe(emotion);
    apply_mouth(emotion);

    action_panel_show(emotion == FACE_EMOTION_CURIOUS);

    lv_opa_t cheek = LV_OPA_40;
    if (emotion == FACE_EMOTION_HAPPY || emotion == FACE_EMOTION_CURIOUS ||
        emotion == FACE_EMOTION_SPEAKING) {
        cheek = LV_OPA_70;
    }
    lv_obj_set_style_bg_opa(s_face.left_cheek, cheek, 0);
    lv_obj_set_style_bg_opa(s_face.right_cheek, cheek, 0);

    face_ui_set_speaking_pulse(emotion == FACE_EMOTION_SPEAKING);
    ESP_LOGI(TAG, "Emotion → %s", emotion_name(emotion));
}

face_emotion_t face_ui_get_emotion(void)
{
    return s_face.emotion;
}

void face_ui_set_caption(const char *text)
{
    if (!s_face.caption) {
        return;
    }
    lv_label_set_text(s_face.caption, text ? text : "");
}

void face_ui_set_tap_callback(face_ui_tap_cb_t cb)
{
    s_face.tap_cb = cb;
}

void face_ui_set_speaking_pulse(bool on)
{
    if (!s_face.speak_timer) {
        return;
    }
    if (on) {
        lv_timer_resume(s_face.speak_timer);
    } else {
        lv_timer_pause(s_face.speak_timer);
        apply_mouth(s_face.emotion);
    }
}
