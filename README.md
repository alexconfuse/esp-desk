# esp-desk

Desk Buddy face bring-up for **ESP-Mosaico** (ESP32-S31, 480×480 touch), plus a **PC/host simulator** that exercises the full idle → tap → Talk/Type → think → speak loop without hardware.

## What is real vs stub (firmware)

| Piece | Status |
| --- | --- |
| Target SoC / board | **Real** — `esp32s31`, ESP-Mosaico v1.0 / v1.2 |
| Display / touch | **Real drivers** via [esp-mosaico-bsp](https://github.com/esp-mosaico/esp-mosaico-bsp) |
| LVGL face emotions | **App** — idle, curious, listening, thinking, speaking, moods, offline, bad_key |
| Talk / Type on device | **Handlers pluggable** — stubs until cloud session is wired |
| Wi‑Fi / NVS / SoftAP / cloud | See product plan / later PRs |
| PC/host full simulator | **Supported** — see below |

## PC / host simulator (full loop)

Separate CMake project under `sim/` that reuses `main/face/` + `main/ui/` against **LVGL 9 + SDL2**. Talk/Type drive a host pipeline: **listening → thinking → speaking + emotion → idle**. Mock STT/LLM/TTS by default; optional live OpenAI-compatible chat if keys are set.

### Build & run

```bash
# Debian/Ubuntu once: sudo apt install cmake build-essential pkg-config libsdl2-dev
# optional live API: sudo apt install libcurl4-openssl-dev
cd sim
CC=gcc CXX=g++ cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
./build/desk_buddy_sim
```

**Demo:** tap face → **Talk** (mock listen) or **Type** (text modal) → watch emotions + caption → idle. **Set** toggles Auto/EN/FR, TTS provider stub, live API. Headless: `DESK_BUDDY_SIM_DEMO=1 xvfb-run -a ./build/desk_buddy_sim`.

Optional live: `OPENAI_API_KEY=… DESK_BUDDY_SIM_LIVE=1 ./build/desk_buddy_sim` (Type/LLM only; STT/TTS still mocked/stubbed).

Details + mocked-vs-real table: [`sim/README.md`](sim/README.md).

## Requirements (firmware)

- **ESP-IDF 6.2+** with ESP32-S31 support
- ESP-Mosaico board + USB data cable

```yaml
# main/idf_component.yml
esp-mosaico-bsp:
  git: https://github.com/esp-mosaico/esp-mosaico-bsp.git
  path: components/esp-mosaico-bsp
  version: 716fc7bca30ed9c61166a0343bf6f5f995b5131f
```

## Build & flash

```bash
idf.py --preview set-target esp32s31
idf.py build
idf.py -p /dev/ttyACM0 flash monitor
```

## Layout

```text
.
├── CMakeLists.txt              # ESP-IDF firmware (sim does not touch)
├── main/face/                  # Shared face emotions (used by sim)
├── main/ui/                    # Talk/Type panel + callbacks (used by sim)
└── sim/                        # Host full simulator (LVGL + SDL2)
    ├── chat/                   # Mock/live LLM session
    ├── ui/                     # Type modal + settings
    └── host/                   # esp_log / esp_random stubs
```
