#pragma once

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Tiny UDP DNS on port 53 that answers every A-query with @p redirect_ip
 * ( SoftAP captive-portal hijack). Call only while SoftAP is up.
 */
esp_err_t captive_dns_start(const char *redirect_ip);

void captive_dns_stop(void);

#ifdef __cplusplus
}
#endif
