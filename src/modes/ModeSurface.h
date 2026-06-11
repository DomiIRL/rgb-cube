#pragma once
#include "../Mode.h"
#include <math.h>

// A travelling height-field: for every (x,z) column on the floor, two crossing
// sine waves set a surface height in y, and the voxel nearest that height lights
// up with a soft anti-aliased band above and below it — an animated topographic
// surface. Hue follows the surface height, like contour colouring. Needs the
// full CUBE_Y = 6 to read as a surface; collapses gracefully at low height.
class ModeSurface : public Mode {
public:
    explicit ModeSurface(uint32_t stepMs = 30, float speed = 0.12f,
                         float kx = 0.9f, float kz = 1.1f)
        : _stepMs(stepMs), _lastMs(0), _t(0.0f), _speed(speed), _kx(kx), _kz(kz) {
    }

    void update(Cube& cube, uint32_t ms) override {
        if (ms - _lastMs < _stepMs) {
            return;
        }
        _lastMs = ms;

        const float amp = (CUBE_Y - 1) * 0.5f;
        for (int z = 0; z < CUBE_Z; z++) {
            for (int x = 0; x < CUBE_X; x++) {
                float wave = 0.5f * (sinf(x * _kx + _t) + sinf(z * _kz + 1.3f * _t));
                float h = amp * (1.0f + wave);                 // 0 .. CUBE_Y-1
                uint16_t hue = (uint16_t)(h / (CUBE_Y > 1 ? CUBE_Y - 1 : 1) * 50000.0f
                                          + _t * 4000.0f);
                for (int y = 0; y < CUBE_Y; y++) {
                    float b = 1.0f - fabsf(y - h);             // soft band around the surface
                    if (b <= 0.0f) {
                        cube.setPixel(x, y, z, 0);
                        continue;
                    }
                    cube.setPixel(x, y, z, Cube::colorHSV(hue, 255, (uint8_t)(b * b * 255.0f)));
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
    float _kx;
    float _kz;
};
