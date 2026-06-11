#pragma once
#include <Adafruit_NeoPixel.h>

constexpr uint8_t  CUBE_X = 6;
constexpr uint8_t  CUBE_Y = 3;  // height (layers) — set to 6 for full cube, 1 for single-layer testing
constexpr uint8_t  CUBE_Z = 6;
constexpr uint16_t CUBE_LEDS = CUBE_X * CUBE_Y * CUBE_Z;

// ── Power budget — limited by the ESP32 dev board, not the LEDs ──────────────
//
// When the cube is powered THROUGH the ESP32 dev board (5 V arrives on the USB
// connector or the VIN/5V pin and the LEDs tap that same rail), the bottleneck
// is neither the LEDs (~13 A theoretical) nor the 3V3 regulator — it is the
// little Schottky diode the board puts in the USB-5V → board-5V path for
// reverse-polarity/back-feed protection. On the DOIT ESP32 DevKit V1 that is an
// SS14: I_F(AV) = 1.0 A continuous. Sustained overload cooks it, and it tends
// to fail SHORTED, which hides the fault. So the diode is the hard ceiling.
//
// We derate it for thermal/continuous margin and reserve current for the ESP32
// core + AMS1117 LDO that share the same rail:
//     usable rail = 1000 mA × 70 %        = 700 mA
//     LED budget  = 700 mA − 200 mA (ESP) = 500 mA
//
// Intentionally conservative. When the final cube runs off the USB-C PD board
// (5 V straight to the LED rail, ESP32 only drives the data line), the diode is
// out of the LED path — raise LED_BUDGET_MA to the PD board's rating (~5 A).
constexpr uint16_t ESP32_DIODE_RATED_MA  = 1000;  // SS14 I_F(AV), DevKit V1
constexpr uint8_t  ESP32_DIODE_DERATE    = 70;    // % continuous/thermal margin
constexpr uint16_t ESP32_CORE_RESERVE_MA = 200;   // ESP32 + LDO share the rail
constexpr uint16_t LED_BUDGET_MA =
    (uint32_t)ESP32_DIODE_RATED_MA * ESP32_DIODE_DERATE / 100 - ESP32_CORE_RESERVE_MA;

// WS2812B per-LED current model @ 5 V:
//   each colour channel at full ≈ 20 mA  → full white ≈ 60 mA
//   controller idle draw ≈ 1 mA  (always present once powered, cannot be dimmed)
constexpr uint8_t LED_CHANNEL_FULL_MA = 20;
constexpr uint8_t LED_IDLE_MA         = 1;

class Cube {
public:
    explicit Cube(uint8_t pin);
    void begin();
    void clear();
    void show();
    void setBrightness(uint8_t brightness);
    // Estimated draw of the current frame at the current brightness, in mA.
    uint32_t estimateCurrentMa() const;
    void setPixel(int x, int y, int z, uint32_t color);
    void setPixel(int index, uint32_t color);
    static uint32_t colorRGB(uint8_t r, uint8_t g, uint8_t b);
    static uint32_t colorRGBW(uint8_t r, uint8_t g, uint8_t b, uint8_t w);
    static uint32_t colorHSV(uint16_t hue, uint8_t sat = 255, uint8_t val = 255);

private:
    Adafruit_NeoPixel _strip;
    uint32_t _lastCapLogMs = 0;  // throttles the "power-capped" Serial notice
    // Scales the frame down in place so its draw fits LED_BUDGET_MA.
    void capToBudget();
    static int coordToIndex(int x, int y, int z);
};
