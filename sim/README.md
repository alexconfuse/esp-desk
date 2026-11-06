# Desk Buddy PC/host face simulator

LVGL 9 + SDL2 host app that **reuses** `main/face/` and `main/ui/` from the firmware.
No ESP-IDF, no Mosaico board, no SoftAP / mic / cloud.

## Dependencies

| Platform | Packages |
| --- | --- |
| Linux (Debian/Ubuntu) | `cmake`, `build-essential` (gcc/g++), `pkg-config`, `libsdl2-dev` |
| macOS | `brew install cmake sdl2 pkg-config` |
| Windows | MSYS2 MinGW: `mingw-w64-x86_64-cmake`, `mingw-w64-x86_64-gcc`, `mingw-w64-x86_64-SDL2`, `mingw-w64-x86_64-pkgconf` — or vcpkg `sdl2` |

Prefer **gcc/g++** (`CC=gcc CXX=g++`) if your default `cc` is Clang without a working C++ stdlib for LVGL’s CMake.

First configure downloads **LVGL v9.2.2** via CMake `FetchContent` (needs network once).

## Build & run

```bash
cd sim
CC=gcc CXX=g++ cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
./build/desk_buddy_sim
```

Headless / CI smoke (Linux):

```bash
cd sim
CC=gcc CXX=g++ cmake -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build -j
xvfb-run -a timeout 3 ./build/desk_buddy_sim || true
```

### Controls

| Input | Effect |
| --- | --- |
| Click the face | `idle` → `curious` (Talk / Type stubs appear) |
| Click **Talk** / **Type** | Status stub only (same as firmware MVP) |
| **Esc** | Back to `idle` (**sim-only** — firmware has no Esc) |
| Close window | Quit |

## Simulated vs board-only

| Piece | Simulator | ESP-Mosaico firmware |
| --- | --- | --- |
| Idle blink + breathe | Yes (same `face_ui.c`) | Yes |
| Tap → curious | Yes (mouse click) | Yes (CST9217 touch) |
| Talk / Type stubs | Yes (same `action_panel.c`) | Yes |
| Esc → idle | Yes | No |
| Display / touch drivers | SDL window | CO5300 + CST9217 via BSP |
| Mic / I2S / speaker | **No** | Board hardware |
| SoftAP / captive / NVS keys | **No** | Firmware (when present) |
| Real STT / LLM / TTS | **No** | Cloud path (when present) |
| `idf.py` build | Untouched — separate CMake tree | Root project |

Host stubs live under `sim/host/` (`esp_log.h`, `esp_random.h`) and are **not** linked into firmware.
