# Firmware Mode System Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Replace the ad-hoc test sketch with a clean OOP firmware: a `Cube` class for coordinate-based LED control, an abstract `Mode` base, and a `ModeSolid` mode — all wired together in `main.cpp` with a hardcoded mode selector ready for a button.

**Architecture:** `Cube` owns the `Adafruit_NeoPixel` strip and maps (x,y,z) → strip index through a single private method. `Mode` is a pure-virtual base; each mode class implements `update(Cube&, uint32_t ms)`. `main.cpp` holds a pointer array of modes and an index; switching a mode calls `onExit`/`onEnter` and re-points `currentMode`.

**Tech Stack:** ESP32 DevKit V1, PlatformIO + Arduino framework, Adafruit NeoPixel library, NEO_RGB color order, GPIO 4 data pin.

> **Note:** No unit test framework is available for embedded targets. Verification steps use `pio run` (compile check) and flash + serial monitor (behavioral check).

---

## File Map

| Action | Path | Responsibility |
|--------|------|---------------|
| Create | `src/Cube.h` | Constants, Cube class declaration |
| Create | `src/Cube.cpp` | Cube implementation + coordToIndex |
| Create | `src/Mode.h` | Abstract Mode base class |
| Create | `src/modes/ModeSolid.h` | ModeSolid declaration |
| Create | `src/modes/ModeSolid.cpp` | ModeSolid implementation |
| Overwrite | `src/main.cpp` | Entry point, mode array, loop |
| Clear | `src/test.cpp` | Remove old test sketch (prevents duplicate setup/loop) |

---

### Task 1: Cube class

**Files:**
- Create: `src/Cube.h`
- Create: `src/Cube.cpp`

- [ ] **Step 1: Create `src/Cube.h`**

```cpp
#pragma once
#include <Adafruit_NeoPixel.h>

static constexpr uint8_t  CUBE_SIZE = 6;
static constexpr uint16_t CUBE_LEDS = CUBE_SIZE * CUBE_SIZE * CUBE_SIZE;

class Cube {
public:
    explicit Cube(uint8_t pin);
    void begin();
    void clear();
    void show();
    void setBrightness(uint8_t brightness);
    void setPixel(int x, int y, int z, uint8_t r, uint8_t g, uint8_t b);
    void setPixel(int x, int y, int z, uint32_t color);
    static uint32_t colorRGB(uint8_t r, uint8_t g, uint8_t b);
    static uint32_t colorHSV(uint16_t hue, uint8_t sat = 255, uint8_t val = 255);

private:
    Adafruit_NeoPixel _strip;
    static int coordToIndex(int x, int y, int z);
};
```

- [ ] **Step 2: Create `src/Cube.cpp`**

```cpp
#include "Cube.h"

Cube::Cube(uint8_t pin)
    : _strip(CUBE_LEDS, pin, NEO_RGB + NEO_KHZ800) {}

void Cube::begin()                    { _strip.begin(); _strip.show(); }
void Cube::clear()                    { _strip.clear(); }
void Cube::show()                     { _strip.show(); }
void Cube::setBrightness(uint8_t b)   { _strip.setBrightness(b); }

void Cube::setPixel(int x, int y, int z, uint8_t r, uint8_t g, uint8_t b) {
    int idx = coordToIndex(x, y, z);
    if (idx >= 0) _strip.setPixelColor(idx, r, g, b);
}

void Cube::setPixel(int x, int y, int z, uint32_t color) {
    int idx = coordToIndex(x, y, z);
    if (idx >= 0) _strip.setPixelColor(idx, color);
}

uint32_t Cube::colorRGB(uint8_t r, uint8_t g, uint8_t b) {
    return Adafruit_NeoPixel::Color(r, g, b);
}

uint32_t Cube::colorHSV(uint16_t hue, uint8_t sat, uint8_t val) {
    return Adafruit_NeoPixel::ColorHSV(hue, sat, val);
}

// TODO: update this mapping once wiring scheme is decided (snake vs straight)
// Current assumption: layer-major, row-major, left-to-right
// z=0 is bottom layer, x increases left-to-right, y increases front-to-back
int Cube::coordToIndex(int x, int y, int z) {
    if (x < 0 || x >= CUBE_SIZE ||
        y < 0 || y >= CUBE_SIZE ||
        z < 0 || z >= CUBE_SIZE) return -1;
    return z * CUBE_SIZE * CUBE_SIZE + y * CUBE_SIZE + x;
}
```

- [ ] **Step 3: Verify it compiles**

```bash
cd ~/projects/led-cube/code && pio run
```

Expected: `[SUCCESS]` — the Cube files compile but nothing calls them yet (no linker errors since main still exists from test.cpp).

---

### Task 2: Mode base class

**Files:**
- Create: `src/Mode.h`

- [ ] **Step 1: Create `src/Mode.h`**

```cpp
#pragma once
#include "Cube.h"

class Mode {
public:
    virtual ~Mode() = default;
    virtual void update(Cube& cube, uint32_t ms) = 0;
    virtual void onEnter(Cube& cube) { cube.clear(); cube.show(); }
    virtual void onExit(Cube& cube)  {}
};
```

- [ ] **Step 2: Verify it compiles**

```bash
pio run
```

Expected: `[SUCCESS]`

---

### Task 3: ModeSolid

**Files:**
- Create: `src/modes/ModeSolid.h`
- Create: `src/modes/ModeSolid.cpp`

- [ ] **Step 1: Create `src/modes/ModeSolid.h`**

```cpp
#pragma once
#include "../Mode.h"

class ModeSolid : public Mode {
public:
    ModeSolid(uint8_t r, uint8_t g, uint8_t b);
    void update(Cube& cube, uint32_t ms) override;
    void onEnter(Cube& cube) override;

private:
    uint8_t _r, _g, _b;
    bool _drawn;
};
```

- [ ] **Step 2: Create `src/modes/ModeSolid.cpp`**

```cpp
#include "ModeSolid.h"

ModeSolid::ModeSolid(uint8_t r, uint8_t g, uint8_t b)
    : _r(r), _g(g), _b(b), _drawn(false) {}

void ModeSolid::onEnter(Cube& cube) {
    _drawn = false;
    cube.clear();
    cube.show();
}

void ModeSolid::update(Cube& cube, uint32_t ms) {
    if (_drawn) return;
    for (int z = 0; z < CUBE_SIZE; z++)
        for (int y = 0; y < CUBE_SIZE; y++)
            for (int x = 0; x < CUBE_SIZE; x++)
                cube.setPixel(x, y, z, _r, _g, _b);
    cube.show();
    _drawn = true;
}
```

- [ ] **Step 3: Verify it compiles**

```bash
pio run
```

Expected: `[SUCCESS]`

---

### Task 4: Wire up main.cpp

**Files:**
- Overwrite: `src/main.cpp` (create if absent)
- Clear: `src/test.cpp` (must be empty to avoid duplicate `setup`/`loop` linker error)

- [ ] **Step 1: Clear `src/test.cpp`**

Replace the entire content of `src/test.cpp` with an empty file (zero bytes).

- [ ] **Step 2: Write `src/main.cpp`**

```cpp
#include <Arduino.h>
#include "Cube.h"
#include "modes/ModeSolid.h"

#define DATA_PIN     4
#define CURRENT_MODE 0   // 0 = solid red; button will replace this

Cube cube(DATA_PIN);

ModeSolid modeSolid(255, 0, 0);   // red

Mode* modes[] = { &modeSolid };
constexpr int NUM_MODES = sizeof(modes) / sizeof(modes[0]);

int       modeIndex   = CURRENT_MODE;
Mode*     currentMode = modes[CURRENT_MODE];

void setMode(int index) {
    currentMode->onExit(cube);
    modeIndex   = index % NUM_MODES;
    currentMode = modes[modeIndex];
    currentMode->onEnter(cube);
}

void setup() {
    Serial.begin(115200);
    Serial.println("LED Cube boot OK");
    cube.begin();
    cube.setBrightness(50);
    currentMode->onEnter(cube);
}

void loop() {
    currentMode->update(cube, millis());
}
```

- [ ] **Step 3: Build**

```bash
pio run
```

Expected: `[SUCCESS]` with RAM ~6%, Flash ~21%.

- [ ] **Step 4: Flash and verify**

```bash
pio run --target upload && pio device monitor
```

Expected serial output:
```
LED Cube boot OK
```
Expected hardware: LED(s) solid red, not blinking.

---

## Adding more modes later

To add a new mode (e.g. `ModeRainbow`):

1. Create `src/modes/ModeRainbow.h` and `src/modes/ModeRainbow.cpp` following the same pattern as `ModeSolid`.
2. In `main.cpp`: `#include "modes/ModeRainbow.h"`, instantiate `ModeRainbow modeRainbow;`, add `&modeRainbow` to the `modes[]` array.
3. To switch via button: call `setMode((modeIndex + 1) % NUM_MODES)` in the button ISR.
