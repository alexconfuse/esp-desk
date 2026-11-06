#pragma once

#include "desk_config.h"
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Start local settings / captive HTTP server on port 80.
 * Serves the same form in SoftAP (first boot) and STA (key rotate) modes.
 * Saving posts to NVS then reboots — no cloud calls.
 */
esp_err_t settings_http_start(const desk_config_t *initial);

/** Optional mDNS hostname "desk-buddy" → http://desk-buddy.local/ (STA). */
esp_err_t settings_mdns_start(void);

#ifdef __cplusplus
}
#endif
