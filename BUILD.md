# BUILD — LED Cube firmware dev guide

Concise build/dev reference so a session can work without re-reading all of `code/src/`.
Hardware, geometry, enclosure and 3D-model specs live in **`CLAUDE.md`** — not repeated here.

## Build & flash
- **Source of truth = the user's VSCode + PlatformIO extension.** Build and flash from there.
- CLI equivalent (only if the shell env is already set up): from `code/` run
  `pio run -e esp32dev` and `pio run -e esp32dev -t upload`.
- **Do NOT touch / "repair" the Python or PlatformIO venv.** If `~/.platformio/penv/bin/pio`
  fails in the agent shell with `ModuleNotFoundError: No module named 'platformio'`, that is a
  shell-environment mismatch, **not** a broken venv — it builds fine in the user's VSCode.
  Never rebuild the venv or switch Python versions; doing so breaks the working setup.
- No-hardware sanity check: compile a mode header with host `g++ -std=gnu++17 -fsyntax-only`
  against tiny stub `Arduino.h` / `Adafruit_NeoPixel.h`. Catches C++ errors — but **not**
  Arduino-macro collisions (see Gotchas), so stub the relevant macros if you rely on this.

## Layout (`code/`)
- `platformio.ini` — env `esp32dev`, framework `arduino`, deps `Adafruit NeoPixel`, `IRremote`.
- `src/main.cpp` — pins (DATA=14, control button=33, BOOT=0, IR receiver=34), the `modes[]`
  registry, BOOT-button + IR-remote control, brightness/speed/pause/blank/auto-cycle state, and
  NVS persistence of the current mode. `handleIrCommand()` maps the full Elegoo NEC remote:
  ▲/▼ next/prev mode, number keys (multi-digit, "1"+"0" = mode 10), `>>|`/`|<<` faster/slower,
  VOL+/- brightness, EQ reset brightness, `>||` pause, Power blank, FUNC/STOP restart, ST/REPT
  auto-cycle (cancelled by any manual mode change). Modes run on a speed-scaled virtual clock
  (`modeClock`), NOT real `millis()` — so speed/pause are global with no per-mode changes. Every
  decoded frame is logged via `printIRResultShort` for remapping. Power the receiver at
  **3.3 V** (see CLAUDE.md). All IR protocol decoders are enabled (no `#define DECODE_NEC`).
- `src/Cube.{h,cpp}` — LED buffer + drawing API. **No software power cap** (removed — the cube
  runs off the USB-C PD board, not the ESP32 diode). Global brightness in `main.cpp`
  (`BRIGHT_MAX` ≈ 80) is the only guard keeping a full-white frame under the ~5 A PD rating.
- `src/Mode.h` — `Mode` base class.
- `src/modes/Mode*.h` — one header-only mode per file.
- `src/Text.{h,cpp}` — scrolling text used for mode labels.

## Cube / Mode API (what a mode needs)
- Geometry: `CUBE_X/Y/Z = 6`, `CUBE_LEDS = 216`. **Y is up** — gravity is −Y, floor `y=0`,
  top `y=5`.
- `class Mode`: override `void update(Cube&, uint32_t ms)`; optional `onEnter/onExit`.
  Gate timing on `ms - _last >= stepMs`, repaint the frame, then call `cube.show()`. **`ms` is a
  speed-scaled virtual clock (`main.cpp::modeClock`), not real `millis()`** — gate only on the
  passed `ms` (never call `millis()` inside a mode) so the remote's speed/pause controls work.
- `Cube`: `setPixel(x,y,z,color)` (serpentine wiring is handled inside — always go through
  this, never index the strip directly), `setPixel(index,color)`, `clear()`, `show()`,
  `setBrightness(v)`. Colours: `Cube::colorRGB(r,g,b)` / `Cube::colorHSV(h,s,v)`, both packed
  as `(r<<16)|(g<<8)|b`.

## Add a new mode
1. Create `src/modes/ModeX.h`: `#pragma once`, `#include "../Mode.h"`, subclass `Mode`.
2. In `main.cpp`: add `#include "modes/ModeX.h"`, a global `ModeX modeX;`, and `&modeX` in
   the `modes[]` array.
3. `INITIAL_MODE` indexes `modes[]`; the active mode is also persisted in NVS across reboots.

## Gotchas
- **Arduino predefines macros that silently clobber bare identifiers** — notably `FALLING`,
  `RISING`, `CHANGE`, `HIGH`, `LOW`, `INPUT`, `OUTPUT`, `min`, `max`, `abs`. Never use these as
  enum constants or variable names. (A bare `FALLING` enumerator once broke the Tetris-mode
  build — fixed by prefixing the phase enum `PH_*`.) A host `g++` check with a bare stub
  `Arduino.h` will NOT catch this; define the colliding macros in the stub to reproduce it.
- **`<IRremote.hpp>` carries its own implementation — include it in exactly ONE translation
  unit** (currently `main.cpp`). Including it from a second `.cpp` causes multiple-definition
  link errors. `#define DECODE_NEC` before the include keeps it lean (NEC-only).

## Keep this file in sync
A `SessionStart` hook (`.claude/settings.json`) points every session here. Whenever you change
build/flash steps, the layout, the Cube/Mode API, add a mode, or learn a new gotcha,
**mirror that change into this file in the same turn** so it never goes stale.
