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
    _strip.show();
}

void Cube::setBrightness(uint8_t brightness) {
    _strip.setBrightness(brightness);
}

void Cube::setPixel(int x, int y, int z, uint32_t color) {
    int index = coordToIndex(x, y, z);
    if (index >= 0) {
        index += LED_INDEX_OFFSET;        // shift past the dead head LED
        if (index >= 0) {
            _strip.setPixelColor(index, color);
        }
    }
}

void Cube::setPixel(int index, uint32_t color) {
    if (index >= 0) {
        index += LED_INDEX_OFFSET;        // shift past the dead head LED
        if (index >= 0) {
            _strip.setPixelColor(index, color);
        }
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
