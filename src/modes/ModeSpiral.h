#pragma once
#include "../Mode.h"
#include <math.h>

// A rotating radar arm that sweeps around the centre of the layer, brightest at
// the arm and fading behind it, with the hue cycling as it turns.
class ModeSpiral : public Mode {
public:
    explicit ModeSpiral(uint32_t stepMs = 30, float speed = 0.12f)
        : _stepMs(stepMs), _lastMs(0), _angle(0.0f), _speed(speed) {
    }

    void update(Cube& cube, uint32_t ms) override {
        if (ms - _lastMs < _stepMs) {
            return;
        }
        _lastMs = ms;

        const float cx = (CUBE_X - 1) * 0.5f;
        const float cz = (CUBE_Z - 1) * 0.5f;
        const float twoPi = 2.0f * (float)M_PI;
        for (int y = 0; y < CUBE_Y; y++) {
            for (int z = 0; z < CUBE_Z; z++) {
                for (int x = 0; x < CUBE_X; x++) {
                    float dx = x - cx;
                    float dz = z - cz;
                    float a = atan2f(dz, dx);          // -pi..pi
                    float behind = _angle - a;         // how far behind the arm
                    while (behind < 0.0f)    behind += twoPi;
                    while (behind >= twoPi)  behind -= twoPi;
                    float b = 1.0f - behind / twoPi;   // 1 at arm, fades round
                    b = b * b * b;
                    uint16_t hue = (uint16_t)(a / twoPi * 65535.0f + _angle * 10000.0f);
                    cube.setPixel(x, y, z, Cube::colorHSV(hue, 255, (uint8_t)(b * 255.0f)));
                }
            }
        }
        cube.show();

        _angle += _speed;
        if (_angle >= 2.0f * (float)M_PI) {
            _angle -= 2.0f * (float)M_PI;
        }
    }

private:
    uint32_t _stepMs;
    uint32_t _lastMs;
    float _angle;
    float _speed;
};
