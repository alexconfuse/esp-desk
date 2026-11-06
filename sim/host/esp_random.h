/**
 * Host stub for ESP-IDF RNG — used only by the PC simulator blink cadence.
 */
#pragma once

#include <stdint.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

static inline uint32_t esp_random(void)
{
    return (uint32_t)rand();
}

#ifdef __cplusplus
}
#endif
