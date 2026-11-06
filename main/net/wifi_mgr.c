#include "wifi_mgr.h"

#include <stdio.h>
#include <string.h>

#include "captive_dns.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"

static const char *TAG = "wifi_mgr";

#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1
#define STA_MAX_RETRY      8

#define SOFTAP_SSID        "DeskBuddy-Setup"
#define SOFTAP_PASS        ""          /* open AP for first-boot phones */
#define SOFTAP_CHANNEL     1
#define SOFTAP_MAX_CONN    4

static EventGroupHandle_t s_wifi_eg;
static wifi_mgr_mode_t s_mode = WIFI_MGR_MODE_IDLE;
static esp_netif_t *s_netif_ap;
static esp_netif_t *s_netif_sta;
static int s_retry;
static bool s_sta_up;
static char s_ap_ip[16];
static char s_sta_ip[16];

static void fill_ip(esp_netif_t *netif, char *buf, size_t buflen)
{
    buf[0] = '\0';
    if (!netif) {
        return;
    }
    esp_netif_ip_info_t info;
    if (esp_netif_get_ip_info(netif, &info) == ESP_OK) {
        snprintf(buf, buflen, IPSTR, IP2STR(&info.ip));
    }
}

static void start_softap(void)
{
    wifi_config_t ap = {0};
    strncpy((char *)ap.ap.ssid, SOFTAP_SSID, sizeof(ap.ap.ssid));
    ap.ap.ssid_len = strlen(SOFTAP_SSID);
    ap.ap.channel = SOFTAP_CHANNEL;
    ap.ap.max_connection = SOFTAP_MAX_CONN;
    ap.ap.authmode = WIFI_AUTH_OPEN;

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &ap));
    ESP_ERROR_CHECK(esp_wifi_start());

    fill_ip(s_netif_ap, s_ap_ip, sizeof(s_ap_ip));
    if (s_ap_ip[0] == '\0') {
        /* Default SoftAP IP before DHCP clients attach */
        strncpy(s_ap_ip, "192.168.4.1", sizeof(s_ap_ip) - 1);
    }
    s_mode = WIFI_MGR_MODE_SOFTAP;
    ESP_LOGI(TAG, "SoftAP '%s' up — captive portal at http://%s/", SOFTAP_SSID, s_ap_ip);
    captive_dns_start(s_ap_ip);
}

static void try_sta(const desk_config_t *cfg)
{
    wifi_config_t sta = {0};
    strncpy((char *)sta.sta.ssid, cfg->wifi_ssid, sizeof(sta.sta.ssid));
    strncpy((char *)sta.sta.password, cfg->wifi_pass, sizeof(sta.sta.password));
    sta.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &sta));
    ESP_ERROR_CHECK(esp_wifi_start());
    ESP_ERROR_CHECK(esp_wifi_connect());
    ESP_LOGI(TAG, "STA connecting to '%s'…", cfg->wifi_ssid);

    EventBits_t bits = xEventGroupWaitBits(s_wifi_eg,
                                           WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
                                           pdFALSE, pdFALSE,
                                           pdMS_TO_TICKS(25000));
    if (bits & WIFI_CONNECTED_BIT) {
        s_mode = WIFI_MGR_MODE_STA;
        s_sta_up = true;
        fill_ip(s_netif_sta, s_sta_ip, sizeof(s_sta_ip));
        ESP_LOGI(TAG, "STA connected, IP %s", s_sta_ip);
        return;
    }

    ESP_LOGW(TAG, "STA failed — falling back to SoftAP provisioning");
    ESP_ERROR_CHECK(esp_wifi_stop());
    start_softap();
}

static void on_wifi_event(void *arg, esp_event_base_t base, int32_t id, void *data)
{
    (void)arg;
    (void)base;
    (void)data;
    if (id == WIFI_EVENT_STA_START) {
        /* connect issued by try_sta */
    } else if (id == WIFI_EVENT_STA_DISCONNECTED) {
        s_sta_up = false;
        s_sta_ip[0] = '\0';
        if (s_retry < STA_MAX_RETRY && s_mode != WIFI_MGR_MODE_SOFTAP) {
            s_retry++;
            esp_wifi_connect();
            ESP_LOGI(TAG, "STA retry %d/%d", s_retry, STA_MAX_RETRY);
        } else if (s_mode != WIFI_MGR_MODE_SOFTAP) {
            xEventGroupSetBits(s_wifi_eg, WIFI_FAIL_BIT);
        }
    } else if (id == WIFI_EVENT_AP_START) {
        fill_ip(s_netif_ap, s_ap_ip, sizeof(s_ap_ip));
    }
}

static void on_ip_event(void *arg, esp_event_base_t base, int32_t id, void *data)
{
    (void)arg;
    (void)base;
    if (id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)data;
        snprintf(s_sta_ip, sizeof(s_sta_ip), IPSTR, IP2STR(&event->ip_info.ip));
        s_retry = 0;
        s_sta_up = true;
        xEventGroupSetBits(s_wifi_eg, WIFI_CONNECTED_BIT);
    }
}

esp_err_t wifi_mgr_start(const desk_config_t *cfg)
{
    if (!cfg) {
        return ESP_ERR_INVALID_ARG;
    }

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    s_netif_ap = esp_netif_create_default_wifi_ap();
    s_netif_sta = esp_netif_create_default_wifi_sta();

    wifi_init_config_t wcfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&wcfg));

    s_wifi_eg = xEventGroupCreate();
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &on_wifi_event, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &on_ip_event, NULL));

    if (desk_config_has_wifi(cfg)) {
        try_sta(cfg);
    } else {
        ESP_LOGI(TAG, "No Wi‑Fi creds in NVS — SoftAP provisioning");
        start_softap();
    }
    return ESP_OK;
}

wifi_mgr_mode_t wifi_mgr_get_mode(void)
{
    return s_mode;
}

void wifi_mgr_get_ap_ip(char *buf, size_t buflen)
{
    if (!buf || buflen == 0) {
        return;
    }
    strncpy(buf, s_ap_ip, buflen - 1);
    buf[buflen - 1] = '\0';
}

void wifi_mgr_get_sta_ip(char *buf, size_t buflen)
{
    if (!buf || buflen == 0) {
        return;
    }
    strncpy(buf, s_sta_ip, buflen - 1);
    buf[buflen - 1] = '\0';
}

bool wifi_mgr_sta_connected(void)
{
    return s_sta_up;
}
