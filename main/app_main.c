/*
 * ESP-Desk — Desk Buddy
 *
 * Face + SoftAP/settings + OpenAI-compatible Talk/Type cloud round-trip.
 * ElevenLabs remains a settings stub; MVP TTS = OpenAI-compatible PCM.
 */

#include "bsp/esp_mosaico.h"
#include "chat_session.h"
#include "desk_config.h"
#include "esp_log.h"
#include "face_ui.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "settings_http.h"
#include "wifi_mgr.h"

static const char *TAG = "esp_desk";

static void net_task(void *arg)
{
    desk_config_t *cfg = (desk_config_t *)arg;

    esp_err_t err = wifi_mgr_start(cfg);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "wifi_mgr_start: %s", esp_err_to_name(err));
        vTaskDelete(NULL);
        return;
    }

    err = settings_http_start(cfg);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "settings_http_start: %s", esp_err_to_name(err));
    }

    if (wifi_mgr_sta_connected()) {
        settings_mdns_start();
        char ip[16];
        wifi_mgr_get_sta_ip(ip, sizeof(ip));
        ESP_LOGI(TAG, "Settings/chat: http://desk-buddy.local/ or http://%s/", ip);
    } else {
        char ip[16];
        wifi_mgr_get_ap_ip(ip, sizeof(ip));
        ESP_LOGI(TAG, "Provisioning: SoftAP DeskBuddy-Setup → http://%s/", ip);
    }

    vTaskDelete(NULL);
}

void app_main(void)
{
    ESP_LOGI(TAG, "Desk Buddy — face + Wi‑Fi + cloud chat %dx%d",
             BSP_LCD_H_RES, BSP_LCD_V_RES);

    ESP_ERROR_CHECK(desk_config_nvs_init());

    static desk_config_t cfg;
    ESP_ERROR_CHECK(desk_config_load(&cfg));

    bsp_board_variant_t variant;
    esp_err_t err = bsp_board_variant_get(&variant);
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "Board variant: %s",
                 variant == BSP_BOARD_VARIANT_V1_2 ? "v1.2" : "v1.0");
    } else {
        ESP_LOGW(TAG, "Board variant detect failed (%s) — continuing",
                 esp_err_to_name(err));
    }

    lv_display_t *disp = bsp_display_start();
    if (!disp) {
        ESP_LOGE(TAG, "bsp_display_start failed — check BSP / IDF target esp32s31");
        return;
    }

    if (!bsp_display_lock(-1)) {
        ESP_LOGE(TAG, "Failed to take LVGL lock");
        return;
    }

    lv_obj_t *scr = lv_screen_active();
    face_ui_create(scr);

    bsp_display_unlock();
    ESP_LOGI(TAG, "UI up — tap → Talk/Type; listen cancel on second tap");

    if (chat_session_start(&cfg) != ESP_OK) {
        ESP_LOGE(TAG, "chat_session_start failed");
    }

    BaseType_t ok = xTaskCreate(net_task, "desk_net", 8192, &cfg, 5, NULL);
    if (ok != pdPASS) {
        ESP_LOGE(TAG, "Failed to start desk_net task");
    }
}
