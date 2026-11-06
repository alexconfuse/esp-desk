#include "sim_settings_ui.h"

#include <stdio.h>

static lv_obj_t *s_modal;
static lv_obj_t *s_info;
static lv_obj_t *s_lang_lab;
static lv_obj_t *s_tts_lab;
static lv_obj_t *s_live_lab;
static sim_config_t *s_cfg;

static void refresh_labels(void)
{
    if (!s_cfg) {
        return;
    }
    if (s_lang_lab) {
        char buf[64];
        snprintf(buf, sizeof(buf), "Language: %s", sim_lang_name(s_cfg->lang));
        lv_label_set_text(s_lang_lab, buf);
    }
    if (s_tts_lab) {
        char buf[80];
        snprintf(buf, sizeof(buf), "TTS: %s (stub)", sim_tts_name(s_cfg->tts_provider));
        lv_label_set_text(s_tts_lab, buf);
    }
    if (s_live_lab) {
        char buf[160];
        snprintf(buf, sizeof(buf), "Live API: %s%s",
                 s_cfg->prefer_live ? "ON" : "OFF (mock)",
                 sim_config_has_openai_key(s_cfg) ? " · key set" : " · no OPENAI_API_KEY");
        lv_label_set_text(s_live_lab, buf);
    }
    if (s_info) {
        lv_label_set_text(s_info,
                          "Mock by default. Optional live chat:\n"
                          "OPENAI_API_KEY + DESK_BUDDY_SIM_LIVE=1\n"
                          "ElevenLabs key is stored/shown only — TTS stays stub.");
    }
}

static void on_close(lv_event_t *e)
{
    (void)e;
    sim_settings_ui_hide();
}

static void on_lang(lv_event_t *e)
{
    (void)e;
    if (!s_cfg) {
        return;
    }
    s_cfg->lang = (sim_lang_t)(((int)s_cfg->lang + 1) % 3);
    refresh_labels();
}

static void on_tts(lv_event_t *e)
{
    (void)e;
    if (!s_cfg) {
        return;
    }
    s_cfg->tts_provider = (s_cfg->tts_provider == SIM_TTS_OPENAI)
                              ? SIM_TTS_ELEVENLABS
                              : SIM_TTS_OPENAI;
    refresh_labels();
}

static void on_live(lv_event_t *e)
{
    (void)e;
    if (!s_cfg) {
        return;
    }
    s_cfg->prefer_live = !s_cfg->prefer_live;
    sim_config_reload_env(s_cfg);
    refresh_labels();
}

static void on_reload(lv_event_t *e)
{
    (void)e;
    if (!s_cfg) {
        return;
    }
    sim_config_reload_env(s_cfg);
    refresh_labels();
}

static lv_obj_t *make_btn(lv_obj_t *parent, const char *title, lv_event_cb_t cb)
{
    lv_obj_t *btn = lv_button_create(parent);
    lv_obj_set_size(btn, 180, 40);
    lv_obj_set_style_radius(btn, 10, 0);
    lv_obj_set_style_bg_color(btn, lv_color_hex(0x2A3340), 0);
    lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, 0);
    lv_obj_add_event_cb(btn, cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *lab = lv_label_create(btn);
    lv_label_set_text(lab, title);
    lv_obj_set_style_text_color(lab, lv_color_hex(0xF2F5F8), 0);
    lv_obj_center(lab);
    return btn;
}

static void on_gear(lv_event_t *e)
{
    (void)e;
    sim_settings_ui_show();
}

void sim_settings_ui_create_gear(lv_obj_t *parent)
{
    lv_obj_t *gear = lv_button_create(parent);
    lv_obj_set_size(gear, 48, 40);
    lv_obj_align(gear, LV_ALIGN_TOP_RIGHT, -12, 12);
    lv_obj_set_style_radius(gear, 10, 0);
    lv_obj_set_style_bg_color(gear, lv_color_hex(0x2A3340), 0);
    lv_obj_set_style_bg_opa(gear, LV_OPA_80, 0);
    lv_obj_add_event_cb(gear, on_gear, LV_EVENT_CLICKED, NULL);
    lv_obj_t *lab = lv_label_create(gear);
    lv_label_set_text(lab, "Set");
    lv_obj_set_style_text_color(lab, lv_color_hex(0xF2F5F8), 0);
    lv_obj_center(lab);
}

void sim_settings_ui_create(lv_obj_t *parent, sim_config_t *cfg)
{
    s_cfg = cfg;

    s_modal = lv_obj_create(parent);
    lv_obj_remove_style_all(s_modal);
    lv_obj_set_size(s_modal, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_color(s_modal, lv_color_hex(0x000000), 0);
    lv_obj_set_style_bg_opa(s_modal, LV_OPA_70, 0);
    lv_obj_clear_flag(s_modal, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *card = lv_obj_create(s_modal);
    lv_obj_set_size(card, 430, 360);
    lv_obj_center(card);
    lv_obj_set_style_radius(card, 16, 0);
    lv_obj_set_style_bg_color(card, lv_color_hex(0x1A222C), 0);
    lv_obj_set_style_border_width(card, 0, 0);
    lv_obj_set_style_pad_all(card, 16, 0);
    lv_obj_set_flex_flow(card, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(card, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_row(card, 10, 0);

    lv_obj_t *title = lv_label_create(card);
    lv_label_set_text(title, "Host settings");
    lv_obj_set_style_text_color(title, lv_color_hex(0xF2F5F8), 0);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_18, 0);

    s_lang_lab = lv_label_create(card);
    lv_obj_set_style_text_color(s_lang_lab, lv_color_hex(0x9AA7B5), 0);
    make_btn(card, "Cycle language", on_lang);

    s_tts_lab = lv_label_create(card);
    lv_obj_set_style_text_color(s_tts_lab, lv_color_hex(0x9AA7B5), 0);
    make_btn(card, "Toggle TTS provider", on_tts);

    s_live_lab = lv_label_create(card);
    lv_obj_set_style_text_color(s_live_lab, lv_color_hex(0x9AA7B5), 0);
    make_btn(card, "Toggle live API", on_live);
    make_btn(card, "Reload env keys", on_reload);

    s_info = lv_label_create(card);
    lv_obj_set_width(s_info, 390);
    lv_label_set_long_mode(s_info, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_text_color(s_info, lv_color_hex(0x7A8794), 0);

    make_btn(card, "Close", on_close);

    refresh_labels();
    sim_settings_ui_hide();
}

void sim_settings_ui_show(void)
{
    if (!s_modal) {
        return;
    }
    sim_config_reload_env(s_cfg);
    refresh_labels();
    lv_obj_remove_flag(s_modal, LV_OBJ_FLAG_HIDDEN);
}

void sim_settings_ui_hide(void)
{
    if (s_modal) {
        lv_obj_add_flag(s_modal, LV_OBJ_FLAG_HIDDEN);
    }
}
