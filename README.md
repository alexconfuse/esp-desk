# esp-desk

Desk Buddy for **ESP-Mosaico** (ESP32-S31, 480×480 touch): animated face + local Wi‑Fi / API-key settings.

No LLM, STT, or TTS cloud calls yet — keys are stored for the next phase.

## What is real vs stub

| Piece | Status |
| --- | --- |
| Target SoC / board | **Real** — `esp32s31`, ESP-Mosaico v1.0 / v1.2 |
| Display / touch | **Real drivers** via [esp-mosaico-bsp](https://github.com/esp-mosaico/esp-mosaico-bsp) (CO5300 + CST9217) |
| LVGL face (`idle` / `curious`) | **App** — blink, breathe; tap wake |
| Talk / Type buttons | **UI stubs** — no mic / chat |
| SoftAP + captive portal | **Real** — first-boot / STA-fail fallback (`DeskBuddy-Setup`) |
| NVS Wi‑Fi + API keys | **Real** — namespace `desk` |
| Settings HTTP page | **Real** — SoftAP IP or `http://desk-buddy.local/` / STA IP |
| TTS provider picker + ElevenLabs key | **Stored stub** — field saved; TTS unused |
| LLM / STT / TTS HTTPS | **Not wired** |

## Requirements

- **ESP-IDF 6.2+** with ESP32-S31 ([get started](https://docs.espressif.com/projects/esp-idf/en/latest/esp32s31/get-started/index.html))
- ESP-Mosaico board + USB data cable

## Build & flash

```bash
idf.py --preview set-target esp32s31
idf.py build
idf.py -p /dev/ttyACM0 flash monitor
```

First flash may need manual **BOOT** download mode. See the [ESP-Mosaico user guide](https://docs.espressif.com/projects/esp-dev-kits/en/latest/esp32s31/esp-mosaico/user_guide.html).

## Provisioning & settings

1. **No Wi‑Fi in NVS** (or STA join fails) → SoftAP **`DeskBuddy-Setup`** (open).
2. Phone joins AP; captive DNS steers probes to the portal. Open **`http://192.168.4.1/`** if needed.
3. Form fields:
   - Wi‑Fi SSID + password
   - OpenAI-compatible **base URL** + **API key**
   - **TTS provider** stub: OpenAI-compatible vs ElevenLabs
   - **ElevenLabs API key** (stored even when unused)
4. **Save & reboot** → device joins STA.
5. On the LAN: **`http://desk-buddy.local/`** (mDNS) or the device IP — same page to rotate keys. Blank password/key fields mean “keep existing”.

Face idle / curious / tap keeps running while networking starts on a background task.

## Layout

```text
main/
  app_main.c           # NVS → face UI → net task
  face/                # idle / curious LVGL
  ui/                  # Talk / Type stubs
  config/              # desk_config NVS
  net/                 # wifi_mgr, captive_dns, settings_http
```

## Next

Cloud STT / LLM / TTS round-trip using stored keys; more face emotions — see the Desk Buddy product plan.
