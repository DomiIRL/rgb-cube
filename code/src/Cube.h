#pragma once
#include <Adafruit_NeoPixel.h>

constexpr uint8_t  CUBE_X = 6;
constexpr uint8_t  CUBE_Y = 6;  // height (layers) — set to 6 for full cube, 1 for single-layer testing
constexpr uint8_t  CUBE_Z = 6;
constexpr uint16_t CUBE_LEDS = CUBE_X * CUBE_Y * CUBE_Z;

// ── Dead-LED workaround ──────────────────────────────────────────────────────
// The first LED in the chain — logical index 0, the (0,0,0) corner — had its
// DIN pin break. The ESP32 data line is now wired straight to the SECOND LED, so
// the physical chain begins at what the cube logic calls index 1. We shift every
// write down by one: logical index 1 becomes the first pixel clocked out, and
// index 0 is dropped (that corner stays dark). All modes address LEDs through
// Cube::setPixel(), so applying the shift there fixes the whole mapping at once.
// Set back to 0 once the broken LED is repaired or replaced.
constexpr int LED_INDEX_OFFSET = -1;

// ── Power ────────────────────────────────────────────────────────────────────
// The cube now runs off the USB-C PD board (5 V straight to the LED rail, ESP32
// only drives the data line), so the old ESP32-dev-board diode bottleneck is out
// of the LED path and there is no software current cap. Frames are sent as drawn.
// The physical ceiling is the PD board's rating (~5 A); keep global brightness
// (main.cpp::setup) sane so a full-white frame can't exceed it.

class Cube {
public:
    explicit Cube(uint8_t pin);
    void begin();
    void clear();
    void show();
    void setBrightness(uint8_t brightness);
    void setPixel(int x, int y, int z, uint32_t color);
    void setPixel(int index, uint32_t color);
    static uint32_t colorRGB(uint8_t r, uint8_t g, uint8_t b);
    static uint32_t colorRGBW(uint8_t r, uint8_t g, uint8_t b, uint8_t w);
    static uint32_t colorHSV(uint16_t hue, uint8_t sat = 255, uint8_t val = 255);

private:
    Adafruit_NeoPixel _strip;
    static int coordToIndex(int x, int y, int z);
};
