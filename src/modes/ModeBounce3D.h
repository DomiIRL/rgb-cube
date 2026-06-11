#pragma once
#include "../Mode.h"

// A single voxel flying through the cube in a straight line, reflecting off all
// six walls like a ball bouncing in a box. Pure deterministic x/y/z motion: one
// lit pixel, no trail. Hue is driven by height, so the ball's layer can be read
// off its colour.
class ModeBounce3D : public Mode {
public:
    explicit ModeBounce3D(uint32_t stepMs = 60)
        : _stepMs(stepMs), _lastMs(0) {
    }

    void onEnter(Cube& cube) override {
        _px = (CUBE_X - 1) * 0.5f;
        _py = (CUBE_Y - 1) * 0.5f;
        _pz = (CUBE_Z - 1) * 0.5f;
        // uneven velocities so the path doesn't fall into a short repeating loop;
        // kept small so the ball holds each cell for several frames (slow glide)
        _vx = 0.13f;
        _vy = (CUBE_Y > 1) ? 0.08f : 0.0f;
        _vz = 0.17f;
        cube.clear();
        cube.show();
    }

    void update(Cube& cube, uint32_t ms) override {
        if (ms - _lastMs < _stepMs) {
            return;
        }
        _lastMs = ms;

        step(_px, _vx, CUBE_X);
        step(_py, _vy, CUBE_Y);
        step(_pz, _vz, CUBE_Z);

        int hx = (int)(_px + 0.5f);
        int hy = (int)(_py + 0.5f);
        int hz = (int)(_pz + 0.5f);
        uint16_t hue = (uint16_t)((float)hy / (float)(CUBE_Y > 1 ? CUBE_Y - 1 : 1) * 60000.0f);

        cube.clear();
        cube.setPixel(hx, hy, hz, Cube::colorHSV(hue, 255, 255));
        cube.show();
    }

private:
    // advance a coordinate by its velocity, reflecting both off the [0, n-1] walls
    static void step(float& p, float& v, int n) {
        p += v;
        if (p < 0.0f)            { p = -p;                       v = -v; }
        if (p > (float)(n - 1))  { p = 2.0f * (n - 1) - p;       v = -v; }
    }

    uint32_t _stepMs;
    uint32_t _lastMs;
    float _px, _py, _pz;
    float _vx, _vy, _vz;
};
