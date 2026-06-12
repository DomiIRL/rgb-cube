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
- `platformio.ini` — env `esp32dev`, framework `arduino`, dep `Adafruit NeoPixel`.
- `src/main.cpp` — pins (DATA=14, control button=34, BOOT=0), the `modes[]` registry,
  button cycling, and NVS persistence of the current mode.
- `src/Cube.{h,cpp}` — LED buffer + drawing API + automatic power cap.
- `src/Mode.h` — `Mode` base class.
- `src/modes/Mode*.h` — one header-only mode per file.
- `src/Text.{h,cpp}` — scrolling text used for mode labels.

## Cube / Mode API (what a mode needs)
- Geometry: `CUBE_X/Y/Z = 6`, `CUBE_LEDS = 216`. **Y is up** — gravity is −Y, floor `y=0`,
  top `y=5`.
- `class Mode`: override `void update(Cube&, uint32_t ms)`; optional `onEnter/onExit`.
  Gate timing on `ms - _last >= stepMs`, repaint the frame, then call `cube.show()`.
- `Cube`: `setPixel(x,y,z,color)` (serpentine wiring is handled inside — always go through
  this, never index the strip directly), `setPixel(index,color)`, `clear()`, `show()`,
  `setBrightness(v)`. Colours: `Cube::colorRGB(r,g,b)` / `Cube::colorHSV(h,s,v)`, both packed
  as `(r<<16)|(g<<8)|b`.
- **No software power cap** — the cube runs off the ~5 A USB-C PD board, so `Cube::show()` sends
  the frame exactly as drawn (the old `capToBudget`/`estimateCurrentMa` machinery is gone). Just
  repaint normally. The only ceiling is global brightness, set in `main.cpp::setup()` (now 50);
  it bounds the worst case (all-white ≈ 216×60 mA × brightness/255).

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

## Keep this file in sync
A `SessionStart` hook (`.claude/settings.json`) points every session here. Whenever you change
build/flash steps, the layout, the Cube/Mode API, add a mode, or learn a new gotcha,
**mirror that change into this file in the same turn** so it never goes stale.
