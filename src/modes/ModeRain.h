#pragma once
#include "../Mode.h"
#include <Arduino.h>
#include <string.h>

// Falling rain. Drops appear at random (x,z) columns on the top layer and fall
// straight down the Y axis one cell per step, each leaving a short fading trail
// behind it (the whole volume dims a little every frame, so a bright head turns
// into a streak). New drops keep spawning to maintain a light shower. Colour is
// water-blue: deep blue trails brightening to a cyan-tipped blue at the drop
// head (no white). The trail buffer is painted back through the 3-arg setPixel
// so it follows the serpentine wiring.
class ModeRain : public Mode {
public:
    explicit ModeRain(uint32_t stepMs = 90, uint8_t fade = 55, uint8_t spawnChance = 60)
        : _stepMs(stepMs), _lastMs(0), _fade(fade), _spawnChance(spawnChance) {
    }

    void onEnter(Cube& cube) override {
        memset(_val, 0, sizeof(_val));
        for (int i = 0; i < MAX_DROPS; i++) {
            _drops[i].active = false;
        }
        cube.clear();
        cube.show();
    }

    void update(Cube& cube, uint32_t ms) override {
        if (ms - _lastMs < _stepMs) {
            return;
        }
        _lastMs = ms;

        // fade the whole volume so heads leave fading trails
        for (int i = 0; i < CUBE_LEDS; i++) {
            _val[i] = (_val[i] > _fade) ? _val[i] - _fade : 0;
        }

        // advance existing drops, lighting their head cell on the way down
        for (int d = 0; d < MAX_DROPS; d++) {
            if (!_drops[d].active) {
                continue;
            }
            _val[bufIndex(_drops[d].x, _drops[d].y, _drops[d].z)] = 255;
            if (--_drops[d].y < 0) {
                _drops[d].active = false;   // hit the floor
            }
        }

        // maybe spawn a new drop at the top in a free column
        if ((uint8_t)random(0, 100) < _spawnChance) {
            spawnDrop();
        }

        // render: blue-water ramp — bright heads are near-white, trails go deep blue
        for (int y = 0; y < CUBE_Y; y++) {
            for (int z = 0; z < CUBE_Z; z++) {
                for (int x = 0; x < CUBE_X; x++) {
                    uint8_t v = _val[bufIndex(x, y, z)];
                    cube.setPixel(x, y, z, waterColor(v));
                }
            }
        }
        cube.show();
    }

private:
    static constexpr int MAX_DROPS = 14;

    struct Drop {
        int x, z, y;
        bool active;
    };

    // plain logical index into the trail buffer (not the wiring order)
    static int bufIndex(int x, int y, int z) {
        return (y * CUBE_Z + z) * CUBE_X + x;
    }

    // map a brightness to a water colour: deep blue in the trails, brightening
    // to a cyan-tipped blue at the drop head. Red stays 0 so it never washes out
    // to white — keeps a cool, moody rain look.
    static uint32_t waterColor(uint8_t v) {
        if (v == 0) {
            return 0;
        }
        uint8_t blue  = v;                               // blue tracks brightness
        // only the brightest part of the head picks up a little green -> cyan tint
        uint8_t green = (v > 120) ? (uint8_t)((v - 120) * 150 / 135) : 0;
        return Cube::colorRGB(0, green, blue);
    }

    void spawnDrop() {
        for (int d = 0; d < MAX_DROPS; d++) {
            if (_drops[d].active) {
                continue;
            }
            _drops[d].x = random(0, CUBE_X);
            _drops[d].z = random(0, CUBE_Z);
            _drops[d].y = CUBE_Y - 1;
            _drops[d].active = true;
            return;
        }
    }

    uint32_t _stepMs;
    uint32_t _lastMs;
    uint8_t _fade;
    uint8_t _spawnChance;   // 0..100 chance per step to spawn a drop
    uint8_t _val[CUBE_LEDS];
    Drop _drops[MAX_DROPS];
};
