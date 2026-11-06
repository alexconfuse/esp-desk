#pragma once

#include "desk_config.h"
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    WIFI_MGR_MODE_IDLE = 0,
    WIFI_MGR_MODE_SOFTAP,   /**< First-boot / fallback captive portal */
    WIFI_MGR_MODE_STA,      /**< Joined home Wi‑Fi */
} wifi_mgr_mode_t;

/**
 * Bring up netif + Wi‑Fi.
 * - Has SSID → try STA (falls back to SoftAP on failure).
 * - No SSID → SoftAP "DeskBuddy-Setup" for provisioning.
 * Does not block the LVGL face; call after display start.
 */
esp_err_t wifi_mgr_start(const desk_config_t *cfg);

wifi_mgr_mode_t wifi_mgr_get_mode(void);

/** SoftAP IPv4 string (e.g. 192.168.4.1) or empty if not in SoftAP. */
void wifi_mgr_get_ap_ip(char *buf, size_t buflen);

/** STA IPv4 string once connected, else empty. */
void wifi_mgr_get_sta_ip(char *buf, size_t buflen);

bool wifi_mgr_sta_connected(void);

#ifdef __cplusplus
}
#endif
