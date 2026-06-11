#pragma once
#include "../Mode.h"
#include <Arduino.h>
#include <string.h>

// A bright comet head that bounces around the layer, leaving a fading rainbow
// trail behind it. Moves diagonally and reflects off the edges.
class ModeComet : public Mode {
public:
    explicit ModeComet(uint32_t stepMs = 70, uint8_t fade = 40)
        : _stepMs(stepMs), _lastMs(0), _fade(fade) {
    }

    void onEnter(Cube& cube) override {
        memset(_val, 0, sizeof(_val));
        memset(_hue, 0, sizeof(_hue));
        _x = random(0, CUBE_X);
        _z = random(0, CUBE_Z);
        _dx = 1;
        _dz = 1;
        _hueBase = 0;
        cube.clear();
        cube.show();
    }

    void update(Cube& cube, uint32_t ms) override {
        if (ms - _lastMs < _stepMs) {
            return;
        }
        _lastMs = ms;

        // fade the whole trail
        for (int i = 0; i < CUBE_LEDS; i++) {
            _val[i] = (_val[i] > _fade) ? _val[i] - _fade : 0;
        }

        // advance the head and bounce off the edges
        _x += _dx;
        if (_x < 0)        { _x = 1;           _dx = 1;  }
        if (_x >= CUBE_X)  { _x = CUBE_X - 2;  _dx = -1; }
        _z += _dz;
        if (_z < 0)        { _z = 1;           _dz = 1;  }
        if (_z >= CUBE_Z)  { _z = CUBE_Z - 2;  _dz = -1; }

        _hueBase += 1500;

        // light the head at full brightness on every layer
        for (int y = 0; y < CUBE_Y; y++) {
            int idx = index(_x, y, _z);
            _val[idx] = 255;
            _hue[idx] = _hueBase;
        }

        for (int i = 0; i < CUBE_LEDS; i++) {
            cube.setPixel(i, Cube::colorHSV(_hue[i], 255, _val[i]));
        }
        cube.show();
    }

private:
    // matches Cube::coordToIndex
    static int index(int x, int y, int z) {
        return y * CUBE_X * CUBE_Z + z * CUBE_X + x;
    }

    uint32_t _stepMs;
    uint32_t _lastMs;
    uint8_t _fade;
    uint8_t _val[CUBE_LEDS];
    uint16_t _hue[CUBE_LEDS];
    int _x, _z, _dx, _dz;
    uint16_t _hueBase;
};
