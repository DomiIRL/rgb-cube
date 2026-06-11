#include "Cube.h"

Cube::Cube(uint8_t pin)
    : _strip(CUBE_LEDS, pin, NEO_RGB + NEO_KHZ800) {
}

void Cube::begin() {
    _strip.begin();
    _strip.show();
}

void Cube::clear() {
    _strip.clear();
}

void Cube::show() {
    capToBudget();
    _strip.show();
}

// Sums the raw (already brightness-scaled) buffer bytes and converts to mA via
// the WS2812B model. NEO_RGB → 3 bytes/LED. channelSum/255 is the count of
// fully-lit channels; ×20 mA gives the dimmable draw, plus the fixed idle draw.
uint32_t Cube::estimateCurrentMa() const {
    const uint8_t* buf = _strip.getPixels();
    uint16_t bytes = _strip.numPixels() * 3;
    uint32_t channelSum = 0;
    for (uint16_t i = 0; i < bytes; i++) {
        channelSum += buf[i];
    }
    uint32_t channelMa = channelSum * LED_CHANNEL_FULL_MA / 255;
    uint32_t idleMa = (uint32_t)_strip.numPixels() * LED_IDLE_MA;
    return channelMa + idleMa;
}

// Hard power cap. Called on every show(): if the frame would draw more than
// LED_BUDGET_MA it is scaled down in place to exactly fit. This is safe because
// every Mode fully repaints the buffer each frame from its own state, so the
// scaled-down bytes are discarded before the next frame is drawn. The idle draw
// (~1 mA/LED) cannot be dimmed away, so only the channel portion is scaled.
void Cube::capToBudget() {
    uint8_t* buf = _strip.getPixels();
    uint16_t bytes = _strip.numPixels() * 3;
    uint32_t channelSum = 0;
    for (uint16_t i = 0; i < bytes; i++) {
        channelSum += buf[i];
    }
    uint32_t channelMa = channelSum * LED_CHANNEL_FULL_MA / 255;
    uint32_t idleMa = (uint32_t)_strip.numPixels() * LED_IDLE_MA;
    uint32_t drawMa = idleMa + channelMa;

    if (channelMa == 0 || drawMa <= LED_BUDGET_MA) {
        if (_capActive) {  // dropped back under budget — log the release once
            Serial.printf("[PWR] cap released: %lu mA within %lu mA budget\n",
                          (unsigned long)drawMa, (unsigned long)LED_BUDGET_MA);
            _capActive = false;
        }
        return;  // nothing lit, or already within budget
    }

    uint32_t channelBudget = (LED_BUDGET_MA > idleMa) ? (LED_BUDGET_MA - idleMa) : 0;
    uint32_t scale = channelBudget * 256 / channelMa;  // 0..256 fixed-point
    if (scale > 256) scale = 256;
    for (uint16_t i = 0; i < bytes; i++) {
        buf[i] = (uint16_t)(buf[i] * scale) >> 8;
    }

    // Edge-triggered logging: a guaranteed line the instant the cap engages,
    // then a 1 Hz heartbeat while it stays active (show() runs many times/sec,
    // so an un-throttled log would flood the monitor).
    uint32_t now = millis();
    if (!_capActive) {
        Serial.printf("[PWR] cap ENGAGED: %lu mA -> %lu mA budget (channels %lu%%)\n",
                      (unsigned long)drawMa, (unsigned long)LED_BUDGET_MA,
                      (unsigned long)(scale * 100 / 256));
        _capActive = true;
        _lastCapLogMs = now;
    } else if (now - _lastCapLogMs >= 1000) {
        Serial.printf("[PWR] capping: %lu mA -> %lu mA budget (channels %lu%%)\n",
                      (unsigned long)drawMa, (unsigned long)LED_BUDGET_MA,
                      (unsigned long)(scale * 100 / 256));
        _lastCapLogMs = now;
    }
}

void Cube::setBrightness(uint8_t brightness) {
    _strip.setBrightness(brightness);
}

void Cube::setPixel(int x, int y, int z, uint32_t color) {
    int index = coordToIndex(x, y, z);
    if (index >= 0) {
        _strip.setPixelColor(index, color);
    }
}

void Cube::setPixel(int index, uint32_t color) {
    if (index >= 0) {
        _strip.setPixelColor(index, color);
    }
}

uint32_t Cube::colorRGB(uint8_t r, uint8_t g, uint8_t b) {
    return Adafruit_NeoPixel::Color(r, g, b);
}

uint32_t Cube::colorRGBW(uint8_t r, uint8_t g, uint8_t b, uint8_t w) {
    return Adafruit_NeoPixel::Color(r, g, b, w);
}

uint32_t Cube::colorHSV(uint16_t hue, uint8_t sat, uint8_t val) {
    return Adafruit_NeoPixel::ColorHSV(hue, sat, val);
}

// Serpentine (boustrophedon) wiring. The WS2812B chain snakes continuously:
//  - within a layer, x reverses every row (row 0 runs +x, row 1 runs -x, ...);
//  - between layers, the z-direction reverses, so a layer ends at the same x/z
//    corner where the next layer begins and the data wire runs straight up.
int Cube::coordToIndex(int x, int y, int z) {
    bool outOfBounds = x < 0 || x >= CUBE_X
                    || y < 0 || y >= CUBE_Y
                    || z < 0 || z >= CUBE_Z;
    if (outOfBounds) {
        return -1;
    }
    // even layers count z upward, odd layers count z downward (layer mirror)
    int row = (y % 2 == 0) ? z : (CUBE_Z - 1 - z);
    // even rows run x upward, odd rows run x downward (snake)
    int col = (row % 2 == 0) ? x : (CUBE_X - 1 - x);
    return y * CUBE_X * CUBE_Z + row * CUBE_X + col;
}
