#pragma once
#include "Cube.h"

// Scrolls a short string horizontally across the cube's top layer (the y = top
// x-z plane), using a 3x3 font fixed one pixel up from the bottom edge. Drives
// itself frame by frame; report active() to know when the message has fully
// scrolled off.
class TextScroll {
public:
    void start(const char* text, uint32_t now);
    void update(Cube& cube, uint32_t now);
    bool active() const { return _active; }

private:
    static const uint8_t* glyphFor(char c);
    void drawFrame(Cube& cube) const;

    char _text[16] = "";
    int _len = 0;
    int _scrollX = 0;     // x of the leftmost column (viewer space)
    int _pixelWidth = 0;  // total advance width of the text in pixels
    bool _static = false; // true when the text fits and is shown without scrolling
    uint32_t _lastMs = 0;
    uint32_t _endMs = 0;  // when a static (non-scrolling) label finishes
    bool _active = false;
};
