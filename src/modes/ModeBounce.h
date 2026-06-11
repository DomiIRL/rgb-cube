#pragma once
#include "../Mode.h"

// One ball per layer: each y-layer holds a single lit voxel that bounces in 2D
// (x/z) off that layer's four walls. Pure deterministic motion, no trail. Each
// layer gets its own hue from its height, so the layers stay distinguishable.
class ModeBounce : public Mode {
public:
    explicit ModeBounce(uint32_t stepMs = 60)
        : _stepMs(stepMs), _lastMs(0) {
    }

    void onEnter(Cube& cube) override {
        for (int y = 0; y < CUBE_Y; y++) {
            Ball& b = _balls[y];
            // stagger start positions and velocities per layer so the balls
            // don't all march in lockstep
            b.px = (CUBE_X - 1) * 0.5f + (y % 3) - 1;
            b.pz = (CUBE_Z - 1) * 0.5f + (y % 2);
            b.vx = 0.13f + 0.015f * y;
            b.vz = 0.17f - 0.012f * y;
            if (y & 1) { b.vx = -b.vx; }   // alternate initial direction
        }
        cube.clear();
        cube.show();
    }

    void update(Cube& cube, uint32_t ms) override {
        if (ms - _lastMs < _stepMs) {
            return;
        }
        _lastMs = ms;

        cube.clear();
        for (int y = 0; y < CUBE_Y; y++) {
            Ball& b = _balls[y];
            step(b.px, b.vx, CUBE_X);
            step(b.pz, b.vz, CUBE_Z);

            int hx = (int)(b.px + 0.5f);
            int hz = (int)(b.pz + 0.5f);
            uint16_t hue = (uint16_t)((float)y / (float)(CUBE_Y > 1 ? CUBE_Y - 1 : 1) * 60000.0f);
            cube.setPixel(hx, y, hz, Cube::colorHSV(hue, 255, 255));
        }
        cube.show();
    }

private:
    struct Ball {
        float px, pz, vx, vz;
    };

    // advance a coordinate by its velocity, reflecting both off the [0, n-1] walls
    static void step(float& p, float& v, int n) {
        p += v;
        if (p < 0.0f)            { p = -p;                       v = -v; }
        if (p > (float)(n - 1))  { p = 2.0f * (n - 1) - p;       v = -v; }
    }

    uint32_t _stepMs;
    uint32_t _lastMs;
    Ball _balls[CUBE_Y];
};
