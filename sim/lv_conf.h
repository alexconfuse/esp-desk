/**
 * LVGL config for the Desk Buddy PC/host simulator (LVGL 9 + SDL2).
 * Intentionally separate from the ESP-IDF / BSP Kconfig path.
 */
#ifndef LV_CONF_H
#define LV_CONF_H

#include <stdint.h>

#define LV_COLOR_DEPTH 32

/* Host builds: never assemble MCU-only SW blend kernels (Helium/Neon). */
#define LV_USE_DRAW_SW_ASM LV_DRAW_SW_ASM_NONE

#define LV_MEM_SIZE (256 * 1024U)

#define LV_USE_STDLIB_MALLOC  LV_STDLIB_CLIB
#define LV_USE_STDLIB_STRING  LV_STDLIB_CLIB
#define LV_USE_STDLIB_SPRINTF LV_STDLIB_CLIB

#define LV_DEF_REFR_PERIOD 15
#define LV_DPI_DEF 130

#define LV_USE_LOG 1
#define LV_LOG_LEVEL LV_LOG_LEVEL_WARN
#define LV_LOG_PRINTF 1

#define LV_USE_ASSERT_NULL 1
#define LV_USE_ASSERT_MALLOC 1

#define LV_FONT_MONTSERRAT_14 1
#define LV_FONT_MONTSERRAT_18 1
#define LV_FONT_MONTSERRAT_24 1
#define LV_FONT_DEFAULT &lv_font_montserrat_18

#define LV_USE_FLEX 1
#define LV_USE_GRID 0

#define LV_USE_TEXTAREA 1
#define LV_USE_KEYBOARD 0
#define LV_USE_LABEL 1
#define LV_USE_BUTTON 1

/* SDL window backend (PC only). */
#define LV_USE_SDL 1
#if LV_USE_SDL
    #define LV_SDL_INCLUDE_PATH <SDL2/SDL.h>
    #define LV_SDL_BUF_COUNT 1
    #define LV_SDL_ACCELERATED 1
    #define LV_SDL_FULLSCREEN 0
    #define LV_SDL_DIRECT_EXIT 1
    #define LV_SDL_MOUSEWHEEL_MODE LV_SDL_MOUSEWHEEL_MODE_ENCODER
#endif

#define LV_USE_PERF_MONITOR 0
#define LV_USE_MEM_MONITOR 0

#endif /* LV_CONF_H */
