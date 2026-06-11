#pragma once
#include "../Mode.h"
#include <math.h>

// Concentric colour rings expanding outward from the centre of the layer,
// like ripples from a drop of water. Reads as 2D circles on a 6x6 grid.
class ModeRipple : public Mode {
public:
    explicit ModeRipple(uint32_t stepMs = 30, float speed = 0.10f, float wavelength = 2.4f)
        : _stepMs(stepMs), _lastMs(0), _t(0.0f), _speed(speed), _wavelength(wavelength) {
    }

    void update(Cube& cube, uint32_t ms) override {
        if (ms - _lastMs < _stepMs) {
            return;
        }
        _lastMs = ms;

        const float cx = (CUBE_X - 1) * 0.5f;
        const float cz = (CUBE_Z - 1) * 0.5f;
        for (int y = 0; y < CUBE_Y; y++) {
            for (int z = 0; z < CUBE_Z; z++) {
                for (int x = 0; x < CUBE_X; x++) {
                    float dx = x - cx;
                    float dz = z - cz;
                    float dist = sqrtf(dx * dx + dz * dz);
                    float phase = dist / _wavelength - _t;
                    float ring = sinf(phase * 2.0f * (float)M_PI) * 0.5f + 0.5f;
                    uint16_t hue = (uint16_t)(dist * 7000.0f + _t * 14000.0f);
                    uint8_t val = (uint8_t)(ring * ring * 255.0f);
                    cube.setPixel(x, y, z, Cube::colorHSV(hue, 255, val));
                }
            }
        }
        cube.show();

        _t += _speed;
    }

private:
    uint32_t _stepMs;
    uint32_t _lastMs;
    float _t;
    float _speed;
    float _wavelength;
};
