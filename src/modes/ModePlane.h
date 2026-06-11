#pragma once
#include "../Mode.h"
#include <math.h>

// A flat plane sweeping through the cube. The plane is defined by a unit normal;
// every LED whose projection onto that normal is close to the sweep offset lights
// up, with a soft falloff across the plane's thickness. The normal cycles through
// the three axes and a body diagonal, so each pass cleanly isolates one
// coordinate before the diagonal plane-wave. Hue encodes how far the plane has
// swept.
class ModePlane : public Mode {
public:
    explicit ModePlane(uint32_t stepMs = 30, float speed = 0.06f, float thickness = 0.9f)
        : _stepMs(stepMs), _lastMs(0), _speed(speed), _thickness(thickness),
          _normalIdx(0), _d(0.0f) {
    }

    void onEnter(Cube& cube) override {
        _normalIdx = 0;
        _d = 0.0f;
        cube.clear();
        cube.show();
    }

    void update(Cube& cube, uint32_t ms) override {
        if (ms - _lastMs < _stepMs) {
            return;
        }
        _lastMs = ms;

        const float nx = NORMALS[_normalIdx][0];
        const float ny = NORMALS[_normalIdx][1];
        const float nz = NORMALS[_normalIdx][2];
        // all normals have non-negative components, so projections run [0, dmax]
        const float dmax = nx * (CUBE_X - 1) + ny * (CUBE_Y - 1) + nz * (CUBE_Z - 1);

        for (int y = 0; y < CUBE_Y; y++) {
            for (int z = 0; z < CUBE_Z; z++) {
                for (int x = 0; x < CUBE_X; x++) {
                    float proj = nx * x + ny * y + nz * z;
                    float dist = fabsf(proj - _d);
                    float b = 1.0f - dist / _thickness;      // 1 on the plane, 0 at its edge
                    if (b <= 0.0f) {
                        cube.setPixel(x, y, z, 0);
                        continue;
                    }
                    b = b * b;
                    uint16_t hue = (uint16_t)(_d / dmax * 50000.0f + _normalIdx * 12000.0f);
                    cube.setPixel(x, y, z, Cube::colorHSV(hue, 255, (uint8_t)(b * 255.0f)));
                }
            }
        }
        cube.show();

        // sweep forward; once the plane has fully left the cube, advance to the
        // next normal and start again from just before the cube
        _d += _speed;
        if (_d > dmax + _thickness) {
            _normalIdx = (_normalIdx + 1) % NUM_NORMALS;
            _d = -_thickness;
        }
    }

private:
    static constexpr int NUM_NORMALS = 4;
    // +x, +y, +z, and the unit body diagonal (1,1,1)/sqrt(3)
    static constexpr float NORMALS[NUM_NORMALS][3] = {
        { 1.0f, 0.0f, 0.0f },
        { 0.0f, 1.0f, 0.0f },
        { 0.0f, 0.0f, 1.0f },
        { 0.57735f, 0.57735f, 0.57735f },
    };

    uint32_t _stepMs;
    uint32_t _lastMs;
    float _speed;
    float _thickness;
    int _normalIdx;
    float _d;
};

constexpr float ModePlane::NORMALS[ModePlane::NUM_NORMALS][3];
