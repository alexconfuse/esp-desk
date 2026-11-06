/**
 * Host stub for ESP-IDF logging — used only by the PC simulator.
 * Firmware continues to use the real esp_log from ESP-IDF.
 */
#pragma once

#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

#define ESP_LOGI(tag, fmt, ...) \
    do { printf("[I][%s] " fmt "\n", (tag), ##__VA_ARGS__); } while (0)

#define ESP_LOGW(tag, fmt, ...) \
    do { printf("[W][%s] " fmt "\n", (tag), ##__VA_ARGS__); } while (0)

#define ESP_LOGE(tag, fmt, ...) \
    do { fprintf(stderr, "[E][%s] " fmt "\n", (tag), ##__VA_ARGS__); } while (0)

#define ESP_LOGD(tag, fmt, ...) \
    do { printf("[D][%s] " fmt "\n", (tag), ##__VA_ARGS__); } while (0)

#ifdef __cplusplus
}
#endif
