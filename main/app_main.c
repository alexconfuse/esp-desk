/*
 * ESP-Desk — Desk Buddy face bring-up (MVP)
 *
 * Scope: idle / curious LVGL face on ESP-Mosaico 480×480 touch.
 * Tap wake → curious + Talk/Type stubs. No Wi‑Fi keys, LLM, or TTS.
 */

#include "bsp/esp_mosaico.h"
#include "esp_log.h"
#include "face_ui.h"

static const char *TAG = "esp_desk";

void app_main(void)
{
    ESP_LOGI(TAG, "Desk Buddy face MVP — ESP-Mosaico %dx%d",
             BSP_LCD_H_RES, BSP_LCD_V_RES);

    bsp_board_variant_t variant;
    esp_err_t err = bsp_board_variant_get(&variant);
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "Board variant: %s",
                 variant == BSP_BOARD_VARIANT_V1_2 ? "v1.2" : "v1.0");
    } else {
        ESP_LOGW(TAG, "Board variant detect failed (%s) — continuing",
                 esp_err_to_name(err));
    }

    /* Real Mosaico drivers: CO5300 QSPI LCD + CST9217 touch + LVGL adapter. */
    lv_display_t *disp = bsp_display_start();
    if (!disp) {
        ESP_LOGE(TAG, "bsp_display_start failed — check BSP / wiring / IDF target esp32s31");
        return;
    }

    if (!bsp_display_lock(-1)) {
        ESP_LOGE(TAG, "Failed to take LVGL lock");
        return;
    }

    lv_obj_t *scr = lv_screen_active();
    face_ui_create(scr);

    bsp_display_unlock();
    ESP_LOGI(TAG, "UI up — idle face; tap to wake curious (Talk/Type stubs)");
}
