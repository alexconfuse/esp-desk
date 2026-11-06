#include "action_panel.h"

#include "chat_session.h"
#include "esp_err.h"
#include "esp_log.h"

static const char *TAG = "action_panel";

static lv_obj_t *s_panel;
static lv_obj_t *s_status;

static void on_talk(lv_event_t *e)
{
    (void)e;
    ESP_LOGI(TAG, "Talk");
    esp_err_t err = chat_session_request_talk();
    if (err != ESP_OK) {
        if (s_status) {
            lv_label_set_text(s_status, "Busy — wait for this turn");
        }
    } else if (s_status) {
        lv_label_set_text(s_status, "Listening… tap face to cancel");
    }
}

static void on_type(lv_event_t *e)
{
    (void)e;
    /* On-device stub: short greeting through the same LLM→TTS pipeline.
     * Prefer typing at http://desk-buddy.local/chat when on LAN. */
    ESP_LOGI(TAG, "Type stub → cloud chat");
    if (s_status) {
        lv_label_set_text(s_status, "Typing stub… or use /chat on LAN");
    }
    esp_err_t err = chat_session_request_text("Hello! Comment ça va?");
    if (err != ESP_OK && s_status) {
        lv_label_set_text(s_status, "Busy — wait for this turn");
    }
}

static lv_obj_t *make_btn(lv_obj_t *parent, const char *title, lv_event_cb_t cb)
{
    lv_obj_t *btn = lv_button_create(parent);
    lv_obj_set_size(btn, 160, 56);
    lv_obj_set_style_radius(btn, 16, 0);
    lv_obj_set_style_bg_color(btn, lv_color_hex(0x2A3340), 0);
    lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(btn, 0, 0);
    lv_obj_add_event_cb(btn, cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *label = lv_label_create(btn);
    lv_label_set_text(label, title);
    lv_obj_set_style_text_color(label, lv_color_hex(0xF2F5F8), 0);
    lv_obj_set_style_text_font(label, &lv_font_montserrat_24, 0);
    lv_obj_center(label);
    return btn;
}

void action_panel_create(lv_obj_t *parent)
{
    s_panel = lv_obj_create(parent);
    lv_obj_remove_style_all(s_panel);
    lv_obj_set_size(s_panel, LV_PCT(100), 140);
    lv_obj_align(s_panel, LV_ALIGN_BOTTOM_MID, 0, -24);
    lv_obj_set_style_bg_opa(s_panel, LV_OPA_TRANSP, 0);
    lv_obj_set_flex_flow(s_panel, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(s_panel, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_row(s_panel, 12, 0);

    lv_obj_t *row = lv_obj_create(s_panel);
    lv_obj_remove_style_all(row);
    lv_obj_set_size(row, LV_PCT(100), 60);
    lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, 0);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(row, 20, 0);

    make_btn(row, "Talk", on_talk);
    make_btn(row, "Type", on_type);

    s_status = lv_label_create(s_panel);
    lv_label_set_text(s_status, "Tap face woke me — pick Talk or Type");
    lv_obj_set_style_text_color(s_status, lv_color_hex(0x9AA7B5), 0);
    lv_obj_set_style_text_font(s_status, &lv_font_montserrat_18, 0);

    action_panel_show(false);
}

void action_panel_show(bool visible)
{
    if (!s_panel) {
        return;
    }
    if (visible) {
        lv_obj_remove_flag(s_panel, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_flag(s_panel, LV_OBJ_FLAG_HIDDEN);
        if (s_status) {
            lv_label_set_text(s_status, "Tap face woke me — pick Talk or Type");
        }
    }
}

void action_panel_set_status(const char *text)
{
    if (s_status) {
        lv_label_set_text(s_status, text ? text : "");
    }
}
