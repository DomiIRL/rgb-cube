#pragma once
#include "../Mode.h"
#include <math.h>

// A rotating arm that corkscrews up the cube: each layer's arm is twisted a
// little further around than the one below it, so the bright edge traces a
// helix/barber-pole through the volume while the whole thing spins over time.
// Brightest at the arm, fading round behind it; hue cycles with the sweep and
// drifts up the height.
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
        // one full turn of the arm spread across the height of the cube
        const float twist = (CUBE_Y > 1) ? twoPi / (float)CUBE_Y : 0.0f;

        for (int y = 0; y < CUBE_Y; y++) {
            const float armAngle = _angle + y * twist;   // this layer's arm direction
            for (int z = 0; z < CUBE_Z; z++) {
                for (int x = 0; x < CUBE_X; x++) {
                    float dx = x - cx;
                    float dz = z - cz;
                    float a = atan2f(dz, dx);          // -pi..pi
                    float behind = armAngle - a;       // how far behind this layer's arm
                    while (behind < 0.0f)    behind += twoPi;
                    while (behind >= twoPi)  behind -= twoPi;
                    float b = 1.0f - behind / twoPi;   // 1 at arm, fades round
                    b = b * b * b;
                    uint16_t hue = (uint16_t)(a / twoPi * 65535.0f
                                              + _angle * 10000.0f
                                              + y * 6000.0f);
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
