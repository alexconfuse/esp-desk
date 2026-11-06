# Desk Buddy PC/host full simulator

LVGL 9 + SDL2 host app that **reuses** `main/face/` and `main/ui/` and fills Talk/Type with a host chat pipeline.

**Default:** mock STT / LLM / TTS (no API keys).  
**Optional:** live OpenAI-compatible chat when `OPENAI_API_KEY` + `DESK_BUDDY_SIM_LIVE=1` (needs libcurl at build time).

## Dependencies

| Platform | Packages |
| --- | --- |
| Linux (Debian/Ubuntu) | `cmake`, `build-essential` (gcc/g++), `pkg-config`, `libsdl2-dev`, optional `libcurl4-openssl-dev` |
| macOS | `brew install cmake sdl2 pkg-config curl` |
| Windows | MSYS2 MinGW SDL2 (+ optional curl) or vcpkg |

Prefer **gcc/g++** (`CC=gcc CXX=g++`). First configure downloads **LVGL v9.2.2** via CMake `FetchContent`.

## Build & run (full loop)

```bash
cd sim
CC=gcc CXX=g++ cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
./build/desk_buddy_sim
```

### Demo the loop

1. **Click** the face → `curious` + Talk / Type.
2. **Talk** → `listening` (mock STT ~1.8s; tap face to cancel) → `thinking` → `speaking` + caption → settle `idle`.
3. **Type** → text modal → Send → same think/speak path (no listen).
4. **Set** (top-right) → cycle **language** Auto/EN/FR, **TTS** OpenAI vs ElevenLabs stub, toggle **live API**.
5. **Esc** → idle when not mid-turn (cancels listen if listening).

Headless smoke:

```bash
cd sim
CC=gcc CXX=g++ cmake -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build -j
DESK_BUDDY_SIM_DEMO=1 xvfb-run -a ./build/desk_buddy_sim
```

### Optional live chat

```bash
export OPENAI_API_KEY=sk-...
# optional: export OPENAI_BASE_URL=https://api.openai.com/v1
export DESK_BUDDY_SIM_LIVE=1
./build/desk_buddy_sim
```

Or toggle **Live API** in Set after exporting the key. Talk STT stays mocked (no mic). TTS is always a stub (face mouth pulse + caption; no audio decode).

Env helpers: `DESK_BUDDY_SIM_LANG=auto|en|fr`, `ELEVENLABS_API_KEY` (shown in settings only).

## Simulated vs board / real cloud

| Piece | PC simulator | Board firmware |
| --- | --- | --- |
| Idle / curious / listen / think / speak / mood faces | Yes (shared `face_ui.c`) | Same sources |
| Talk / Type UX | Wired to host session | SoftAP `/chat` + cloud session (when present) |
| STT | **Mock phrase** (no mic) | Device mic + cloud STT |
| LLM | **Mock** (keyword + EN/FR) or optional live chat | Cloud chat |
| TTS | **Stub** (animation + log; OpenAI/ElevenLabs picker only) | Cloud TTS + speaker |
| SoftAP / NVS / Wi‑Fi | **No** | Firmware |
| `idf.py` | Untouched — separate CMake under `sim/` | Root project |

Host-only code: `sim/chat/`, `sim/ui/`, `sim/host/`.
