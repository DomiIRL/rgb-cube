# Firmware Mode System — Design Spec
_2026-06-08_

## Goal

Provide a clean OOP firmware architecture for the 6×6×6 LED cube that:
- Addresses LEDs by (x, y, z) coordinates instead of raw index
- Supports multiple display modes, each encapsulated in its own class
- Allows mode switching via a button (hardcoded index for now, ISR later)

---

## Architecture

### Cube

Owns the `Adafruit_NeoPixel` strip. All LED writes go through it.

**Public API:**
- `begin()` — initialise strip
- `clear()` — zero all pixels
- `show()` — flush to hardware
- `setBrightness(uint8_t)`
- `setPixel(int x, int y, int z, uint8_t r, uint8_t g, uint8_t b)`
- `setPixel(int x, int y, int z, uint32_t color)`
- `static uint32_t colorRGB(uint8_t r, uint8_t g, uint8_t b)`
- `static uint32_t colorHSV(uint16_t hue, uint8_t sat, uint8_t val)`

**coordToIndex** is a private static method — single place to update when wiring scheme is finalised.  
Current mapping: `index = z * 36 + y * 6 + x` (layer-major, row-major, left-to-right).  
Out-of-bounds coordinates are silently ignored (return -1).

Constants `CUBE_SIZE = 6` and `CUBE_LEDS = 216` are defined in `Cube.h`.

---

### Mode (abstract base)

```cpp
class Mode {
public:
    virtual ~Mode() = default;
    virtual void update(Cube& cube, uint32_t ms) = 0;
    virtual void onEnter(Cube& cube) { cube.clear(); }
    virtual void onExit(Cube& cube) {}
};
```

`update()` is called every loop iteration. `ms` is `millis()` — modes use it for non-blocking animation timing. No `delay()` in any mode.

---

### ModeSolid

Sets all LEDs to a single color. Color is passed as a constructor argument.

```cpp
ModeSolid(uint8_t r, uint8_t g, uint8_t b);
```

`update()` only redraws on first call after `onEnter` to avoid redundant strip writes.

---

### main.cpp

```cpp
#define CURRENT_MODE 0   // change to switch mode; button will replace this

Mode* modes[] = { &modeSolid /*, &modeRainbow, ... */ };
constexpr int NUM_MODES = sizeof(modes) / sizeof(modes[0]);
int modeIndex = CURRENT_MODE;
```

Button ISR (future): increment `modeIndex % NUM_MODES`, call `modes[modeIndex-1]->onExit()`, call `modes[modeIndex]->onEnter()`.

---

## File Layout

```
src/
  main.cpp
  Cube.h
  Cube.cpp
  Mode.h
  modes/
    ModeSolid.h
    ModeSolid.cpp
```

Existing `test.cpp` cleared. `log.cpp` left empty.

---

## Constraints

- NEO_RGB color order (confirmed from hardware testing)
- Data pin: GPIO 4
- `coordToIndex` must be updated once wiring scheme is decided (CLAUDE.md: snake vs straight)
- Testing with 2 physical LEDs — code uses full 216-LED buffer; only first 2 respond
