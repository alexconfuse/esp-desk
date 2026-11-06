# esp-desk

Desk Buddy for **ESP-Mosaico** (ESP32-S31, 480×480): pet face + Wi‑Fi settings + OpenAI-compatible **Talk/Type** cloud round-trip.

## What is real vs stub

| Piece | Status |
| --- | --- |
| Display / touch | **Real** — `esp-mosaico-bsp` (CO5300 + CST9217) |
| Face emotions | **App** — idle, curious, listening, thinking, speaking, happy/sad/…, offline, bad_key |
| SoftAP + NVS settings | **Real** — Wi‑Fi + API keys |
| Mic / speaker | **Real BSP audio** (ES8311) when codec init succeeds |
| STT / LLM / TTS | **Real HTTPS** via OpenAI-compatible base URL + key from NVS |
| EN / FR | **Prompt + Whisper auto** — reply in user language |
| ElevenLabs | **Settings stub only** — key stored; MVP TTS is OpenAI-compatible `tts-1` PCM |
| Wake word | **Not present** — tap to wake only |

## Build & flash

```bash
idf.py --preview set-target esp32s31
idf.py build
idf.py -p /dev/ttyACM0 flash monitor
```

Needs **ESP-IDF 6.2+** with ESP32-S31. First flash may need manual BOOT mode.

## UX

1. **Idle** face (blink/breathe). Tap → **curious** + Talk / Type.
2. **Talk** → listening (~4.5s, tap again to cancel) → STT → thinking → LLM JSON emotion+text → speaking (TTS) → settle idle.
3. **Type** on-device → short EN/FR stub line through the same pipeline; or open **`http://desk-buddy.local/chat`** to type freely.
4. Offline / 401 → **offline** / **bad_key** faces.

## Settings

- SoftAP `DeskBuddy-Setup` when unprovisioned
- After STA: `http://desk-buddy.local/` (keys) and `/chat` (type path)

## Layout

```text
main/
  face/     LVGL emotions
  ui/       Talk / Type
  config/   NVS
  net/      SoftAP, captive DNS, settings + /chat
  audio/    mic record / spk play
  cloud/    OpenAI-compatible STT, chat, TTS + session
```
