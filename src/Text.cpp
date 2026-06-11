#include "Text.h"
#include <string.h>

namespace {

constexpr int CHAR_W = 3;       // glyph width in pixels
constexpr int CHAR_H = 5;       // glyph height in pixels
constexpr int ADVANCE = 4;      // CHAR_W + 1 column gap between glyphs
constexpr int Z_BASE = 1;       // bottom row sits one pixel up from the z=0 edge
constexpr uint32_t STEP_MS = 110;    // ms per one-column scroll step
constexpr uint32_t STATIC_MS = 900;  // how long a non-scrolling label is held
constexpr uint8_t VAL = 200;         // brightness

// Compact 3x5 font, rows top->bottom, 3 bits per row (bit2 = leftmost column).
// Digits use a clean seven-segment style. Order: 0-9, A-Z, space at index 36.
const uint8_t FONT[37][CHAR_H] = {
    {0b111,0b101,0b101,0b101,0b111}, // 0
    {0b110,0b010,0b010,0b010,0b111}, // 1
    {0b111,0b001,0b111,0b100,0b111}, // 2
    {0b111,0b001,0b111,0b001,0b111}, // 3
    {0b101,0b101,0b111,0b001,0b001}, // 4
    {0b111,0b100,0b111,0b001,0b111}, // 5
    {0b111,0b100,0b111,0b101,0b111}, // 6
    {0b111,0b001,0b001,0b001,0b001}, // 7
    {0b111,0b101,0b111,0b101,0b111}, // 8
    {0b111,0b101,0b111,0b001,0b111}, // 9
    {0b111,0b101,0b111,0b101,0b101}, // A
    {0b110,0b101,0b110,0b101,0b110}, // B
    {0b111,0b100,0b100,0b100,0b111}, // C
    {0b110,0b101,0b101,0b101,0b110}, // D
    {0b111,0b100,0b111,0b100,0b111}, // E
    {0b111,0b100,0b111,0b100,0b100}, // F
    {0b111,0b100,0b101,0b101,0b111}, // G
    {0b101,0b101,0b111,0b101,0b101}, // H
    {0b111,0b010,0b010,0b010,0b111}, // I
    {0b001,0b001,0b001,0b101,0b111}, // J
    {0b101,0b101,0b110,0b101,0b101}, // K
    {0b100,0b100,0b100,0b100,0b111}, // L
    {0b101,0b111,0b111,0b101,0b101}, // M
    {0b101,0b111,0b101,0b101,0b101}, // N
    {0b111,0b101,0b101,0b101,0b111}, // O
    {0b111,0b101,0b111,0b100,0b100}, // P
    {0b111,0b101,0b101,0b111,0b001}, // Q
    {0b110,0b101,0b110,0b101,0b101}, // R
    {0b111,0b100,0b111,0b001,0b111}, // S
    {0b111,0b010,0b010,0b010,0b010}, // T
    {0b101,0b101,0b101,0b101,0b111}, // U
    {0b101,0b101,0b101,0b101,0b010}, // V
    {0b101,0b101,0b111,0b111,0b101}, // W
    {0b101,0b101,0b010,0b101,0b101}, // X
    {0b101,0b101,0b010,0b010,0b010}, // Y
    {0b111,0b001,0b010,0b100,0b111}, // Z
    {0b000,0b000,0b000,0b000,0b000}, // space
};

int glyphIndex(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'A' && c <= 'Z') return 10 + (c - 'A');
    if (c >= 'a' && c <= 'z') return 10 + (c - 'a');
    return 36; // space / unknown
}

}  // namespace

const uint8_t* TextScroll::glyphFor(char c) {
    return FONT[glyphIndex(c)];
}

void TextScroll::drawFrame(Cube& cube) const {
    const int y = CUBE_Y - 1;  // top layer
    const uint32_t color = Cube::colorRGB(VAL, VAL, VAL);
    cube.clear();
    for (int i = 0; i < _len; i++) {
        const uint8_t* g = glyphFor(_text[i]);
        int charX = _scrollX + i * ADVANCE;       // viewer x of this glyph's left col
        for (int col = 0; col < CHAR_W; col++) {
            int vx = charX + col;                  // viewer x of this column
            if (vx < 0 || vx >= CUBE_X) {
                continue;
            }
            int x = (CUBE_X - 1) - vx;              // single global flip corrects the mirror
            for (int r = 0; r < CHAR_H; r++) {
                if (g[r] & (1 << (CHAR_W - 1 - col))) {
                    cube.setPixel(x, y, Z_BASE + (CHAR_H - 1 - r), color);
                }
            }
        }
    }
    cube.show();
}

void TextScroll::start(const char* text, uint32_t now) {
    strncpy(_text, text, sizeof(_text) - 1);
    _text[sizeof(_text) - 1] = '\0';
    _len = (int)strlen(_text);
    _pixelWidth = _len * ADVANCE;
    // ink width = glyphs plus one-pixel gaps between them
    int inkWidth = _len > 0 ? _len * CHAR_W + (_len - 1) : 0;
    _active = (_len > 0);
    _lastMs = now - STEP_MS;  // draw the first frame immediately

    if (inkWidth <= CUBE_X) {
        // fits on the layer: show it centred, no scrolling
        _static = true;
        _scrollX = (CUBE_X - inkWidth) / 2;
        _endMs = now + STATIC_MS;
    } else {
        _static = false;
        _scrollX = CUBE_X;       // start just off the right edge
    }
}

void TextScroll::update(Cube& cube, uint32_t now) {
    if (!_active || now - _lastMs < STEP_MS) {
        return;
    }
    _lastMs = now;

    drawFrame(cube);

    if (_static) {
        if (now >= _endMs) {
            _active = false;  // held long enough; the mode takes over next
        }
    } else {
        _scrollX--;
        if (_scrollX <= -_pixelWidth) {
            _active = false;  // fully scrolled off; the mode takes over next
        }
    }
}
