# esp-desk

Desk Buddy face bring-up for **ESP-Mosaico** (ESP32-S31, 480×480 touch).

MVP scope from the product plan: prove the pet-face loop — **idle** (blink + breathe) and **curious** — with **tap-to-wake**. No Wi‑Fi keys, no LLM, no ElevenLabs / TTS.

## What is real vs stub

| Piece | Status |
| --- | --- |
| Target SoC / board | **Real** — `esp32s31`, ESP-Mosaico v1.0 / v1.2 |
| Display | **Real drivers** via [esp-mosaico-bsp](https://github.com/esp-mosaico/esp-mosaico-bsp) — CO5300 480×480 QSPI |
| Touch | **Real drivers** via the same BSP — CST9217 (CST9220-series) |
| LVGL face (`idle` / `curious`) | **App code** — blink, breathe, pupil glance |
| Tap wake → curious | **Wired** on the active screen |
| Talk / Type buttons | **UI stubs only** — log + status label; no mic, STT, or chat |
| Wi‑Fi / NVS keys / LLM / TTS | **Out of scope** for this scaffold |

There is **no host simulator** in-tree. Building and running requires ESP-IDF + a Mosaico board (or equivalent hardware that the BSP accepts).

## Requirements

- **ESP-IDF 6.2+** with ESP32-S31 support ([get started](https://docs.espressif.com/projects/esp-idf/en/latest/esp32s31/get-started/index.html))
- ESP-Mosaico board (CoreBoard V1.0 or V1.2)
- USB data cable (Type-C)

The BSP dependency is pulled from GitHub (not yet a standalone Component Registry package):

```yaml
# main/idf_component.yml
esp-mosaico-bsp:
  git: https://github.com/esp-mosaico/esp-mosaico-bsp.git
  path: components/esp-mosaico-bsp
  version: 716fc7bca30ed9c61166a0343bf6f5f995b5131f
```

## Build & flash

```bash
# Activate your ESP-IDF environment first, then:
idf.py --preview set-target esp32s31
idf.py build
idf.py -p /dev/ttyACM0 flash monitor
```

Notes from the [ESP-Mosaico user guide](https://docs.espressif.com/projects/esp-dev-kits/en/latest/esp32s31/esp-mosaico/user_guide.html) and BSP:

- First flash may need **manual BOOT** download mode (hold BOOT while resetting / powering).
- After a successful flash, BSP USB CDC console + auto-download are preferred when enabled in the BSP.
- Port name varies (`/dev/ttyACM0`, `/dev/ttyUSB0`, or a `cu.*` device on macOS).
- Use `idf.py -p PORT monitor --no-reset` to attach without rebooting the app.

## UX on device

1. Boot → soft ambient **idle** face (blink + breathe). Not recording.
2. **Tap** the face → **curious** (wider eyes, glance, stronger breathe).
3. **Talk** / **Type** appear as stubs — they only update a status line and log; cloud / audio come later.

## Layout

```text
.
├── CMakeLists.txt
├── sdkconfig.defaults          # esp32s31 + LVGL + PSRAM defaults
├── main/
│   ├── app_main.c              # BSP display start → face UI
│   ├── idf_component.yml       # esp-mosaico-bsp (git)
│   ├── face/                   # idle / curious LVGL face
│   └── ui/                     # Talk / Type stub panel
└── README.md
```

## Next (not in this PR)

Provisioning softAP, OpenAI-compatible STT/LLM/TTS, more emotions, EN/FR — see the Desk Buddy product plan.
