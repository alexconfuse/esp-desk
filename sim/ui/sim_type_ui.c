#include "sim_type_ui.h"

#include <stdio.h>
#include <string.h>

static lv_obj_t *s_modal;
static lv_obj_t *s_ta;
static sim_type_submit_cb_t s_submit;

static void close_modal(void)
{
    if (s_modal) {
        lv_obj_add_flag(s_modal, LV_OBJ_FLAG_HIDDEN);
    }
}

static void on_cancel(lv_event_t *e)
{
    (void)e;
    close_modal();
}

static void do_submit(void)
{
    if (!s_ta || !s_submit) {
        return;
    }
    const char *txt = lv_textarea_get_text(s_ta);
    if (!txt || !txt[0]) {
        return;
    }
    char copy[280];
    snprintf(copy, sizeof(copy), "%s", txt);
    close_modal();
    lv_textarea_set_text(s_ta, "");
    s_submit(copy);
}

static void on_send(lv_event_t *e)
{
    (void)e;
    do_submit();
}

static void on_ta_ready(lv_event_t *e)
{
    (void)e;
    do_submit();
}

static lv_obj_t *make_btn(lv_obj_t *parent, const char *title, lv_event_cb_t cb, uint32_t bg)
{
    lv_obj_t *btn = lv_button_create(parent);
    lv_obj_set_size(btn, 120, 44);
    lv_obj_set_style_radius(btn, 12, 0);
    lv_obj_set_style_bg_color(btn, lv_color_hex(bg), 0);
    lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, 0);
    lv_obj_add_event_cb(btn, cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *lab = lv_label_create(btn);
    lv_label_set_text(lab, title);
    lv_obj_set_style_text_color(lab, lv_color_hex(0xF2F5F8), 0);
    lv_obj_center(lab);
    return btn;
}

void sim_type_ui_create(lv_obj_t *parent, sim_type_submit_cb_t on_submit)
{
    s_submit = on_submit;

    s_modal = lv_obj_create(parent);
    lv_obj_remove_style_all(s_modal);
    lv_obj_set_size(s_modal, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_color(s_modal, lv_color_hex(0x000000), 0);
    lv_obj_set_style_bg_opa(s_modal, LV_OPA_70, 0);
    lv_obj_add_flag(s_modal, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_clear_flag(s_modal, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *card = lv_obj_create(s_modal);
    lv_obj_set_size(card, 420, 220);
    lv_obj_center(card);
    lv_obj_set_style_radius(card, 16, 0);
    lv_obj_set_style_bg_color(card, lv_color_hex(0x1A222C), 0);
    lv_obj_set_style_border_width(card, 0, 0);
    lv_obj_set_style_pad_all(card, 16, 0);
    lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *title = lv_label_create(card);
    lv_label_set_text(title, "Type to Desk Buddy");
    lv_obj_set_style_text_color(title, lv_color_hex(0xF2F5F8), 0);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_18, 0);
    lv_obj_align(title, LV_ALIGN_TOP_LEFT, 0, 0);

    s_ta = lv_textarea_create(card);
    lv_obj_set_size(s_ta, 380, 90);
    lv_obj_align(s_ta, LV_ALIGN_TOP_MID, 0, 36);
    lv_textarea_set_placeholder_text(s_ta, "Hello! / Bonjour!");
    lv_textarea_set_one_line(s_ta, true);
    lv_textarea_set_max_length(s_ta, 240);
    lv_obj_add_event_cb(s_ta, on_ta_ready, LV_EVENT_READY, NULL);

    lv_obj_t *row = lv_obj_create(card);
    lv_obj_remove_style_all(row);
    lv_obj_set_size(row, 380, 50);
    lv_obj_align(row, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row, LV_FLEX_ALIGN_END, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(row, 12, 0);

    make_btn(row, "Cancel", on_cancel, 0x3A4450);
    make_btn(row, "Send", on_send, 0x3D6B8A);

    sim_type_ui_hide();
}

void sim_type_ui_show(void)
{
    if (!s_modal) {
        return;
    }
    lv_obj_remove_flag(s_modal, LV_OBJ_FLAG_HIDDEN);
    if (s_ta) {
        lv_obj_add_state(s_ta, LV_STATE_FOCUSED);
        lv_textarea_set_text(s_ta, "");
    }
}

void sim_type_ui_hide(void)
{
    close_modal();
}
