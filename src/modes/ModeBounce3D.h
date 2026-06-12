#pragma once
#include "../Mode.h"
#include <math.h>

// A glowing ball bouncing around the inside of the cube, reflecting off all six
// walls. The ball's position is tracked in floating point and rendered as a
// soft anti-aliased sphere: every voxel near the centre lights up with a
// brightness that falls off with distance. Because neighbouring voxels share
// the light, the ball appears to glide smoothly between cells instead of
// snapping from one voxel to the next — which is what makes a low-resolution
// 6x6x6 grid read as a real round ball. The ball keeps one colour while it
// travels and jumps to a new hue each time it bounces off a wall.
class ModeBounce3D : public Mode {
public:
    explicit ModeBounce3D(uint32_t stepMs = 33, float radius = 1.5f)
        : _stepMs(stepMs), _lastMs(0), _radius(radius) {
    }

    void onEnter(Cube& cube) override {
        _px = (CUBE_X - 1) * 0.5f;
        _py = (CUBE_Y - 1) * 0.5f;
        _pz = (CUBE_Z - 1) * 0.5f;
        // uneven velocities so the path doesn't fall into a short repeating loop;
        // tuned for ~30 Hz so the ball drifts a fraction of a cell per frame
        _vx = 0.070f;
        _vy = (CUBE_Y > 1) ? 0.045f : 0.0f;
        _vz = 0.090f;
        _hue = 0;
        cube.clear();
        cube.show();
    }

    void update(Cube& cube, uint32_t ms) override {
        if (ms - _lastMs < _stepMs) {
            return;
        }
        _lastMs = ms;

        // step all three axes (each may bounce); change colour only on a bounce.
        // use | not || so every axis is stepped regardless of an earlier bounce.
        bool bounced = step(_px, _vx, CUBE_X)
                     | step(_py, _vy, CUBE_Y)
                     | step(_pz, _vz, CUBE_Z);
        if (bounced) {
            // golden-angle hop so consecutive bounce colours look distinct
            _hue += 40503;
        }
        const uint16_t hue = _hue;

        cube.clear();

        // only touch voxels within the ball's bounding box
        int x0 = clampLo(_px - _radius), x1 = clampHi(_px + _radius, CUBE_X);
        int y0 = clampLo(_py - _radius), y1 = clampHi(_py + _radius, CUBE_Y);
        int z0 = clampLo(_pz - _radius), z1 = clampHi(_pz + _radius, CUBE_Z);

        for (int y = y0; y <= y1; y++) {
            for (int z = z0; z <= z1; z++) {
                for (int x = x0; x <= x1; x++) {
                    float dx = x - _px;
                    float dy = y - _py;
                    float dz = z - _pz;
                    float dist = sqrtf(dx * dx + dy * dy + dz * dz);
                    float b = 1.0f - dist / _radius;   // 1 at centre, 0 at the rim
                    if (b <= 0.0f) {
                        continue;
                    }
                    b = b * b;                         // round the falloff, tighten the core
                    cube.setPixel(x, y, z, Cube::colorHSV(hue, 255, (uint8_t)(b * 255.0f)));
                }
            }
        }
        cube.show();
    }

private:
    // advance a coordinate by its velocity, reflecting off the [0, n-1] walls;
    // returns true if it bounced off a wall this step
    static bool step(float& p, float& v, int n) {
        p += v;
        bool bounced = false;
        if (p < 0.0f)            { p = -p;                       v = -v; bounced = true; }
        if (p > (float)(n - 1))  { p = 2.0f * (n - 1) - p;       v = -v; bounced = true; }
        return bounced;
    }

    static int clampLo(float f) {
        int i = (int)floorf(f);
        return i < 0 ? 0 : i;
    }
    static int clampHi(float f, int n) {
        int i = (int)ceilf(f);
        return i > n - 1 ? n - 1 : i;
    }

    uint32_t _stepMs;
    uint32_t _lastMs;
    float _radius;
    float _px, _py, _pz;
    float _vx, _vy, _vz;
    uint16_t _hue;   // current ball colour; advances on each wall bounce
};
