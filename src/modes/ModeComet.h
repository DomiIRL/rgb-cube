#pragma once
#include "../Mode.h"
#include <Arduino.h>
#include <string.h>

// A bright comet head that ricochets through the whole cube in 3D, leaving a
// fading rainbow trail behind it in the volume. It steps one cell at a time and
// bounces off all six walls; on each bounce it re-rolls a random direction —
// each axis independently picks -1, 0 or +1 (the axis that hit keeps heading
// inward) — so it never settles into a fixed repeating path and wanders the
// whole volume: sometimes straight, sometimes along a face, sometimes diagonal.
// (ModeBounce3D is the trail-less slow glide; this one streaks and bounces
// faster.) The per-voxel trail is stored in a logical buffer and painted back
// through cube.setPixel(x,y,z,...) so it follows the serpentine wiring.
class ModeComet : public Mode {
public:
    explicit ModeComet(uint32_t stepMs = 70, uint8_t fade = 40)
        : _stepMs(stepMs), _lastMs(0), _fade(fade) {
    }

    void onEnter(Cube& cube) override {
        memset(_val, 0, sizeof(_val));
        memset(_hue, 0, sizeof(_hue));
        _x = random(0, CUBE_X);
        _y = random(0, CUBE_Y);
        _z = random(0, CUBE_Z);
        _dx = randSign();
        _dy = (CUBE_Y > 1) ? randSign() : 0;   // hold still in y when running a single layer
        _dz = randSign();
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

        // advance the head and bounce off the walls. On ANY bounce, re-roll a
        // fresh direction for every axis (each independently steps -1, 0 or +1).
        // Re-rolling all axes — not just the ones that missed a wall — is what
        // breaks the corner-to-corner trap: at a corner all three axes bounce,
        // and the old code would just flip to the opposite diagonal forever. An
        // axis that hit a wall may only rest or head back inward (never straight
        // back into the wall), and a guard keeps at least one axis moving.
        int hx = bounce(_x, _dx, CUBE_X);
        int hy = bounce(_y, _dy, CUBE_Y);
        int hz = bounce(_z, _dz, CUBE_Z);
        if (hx || hy || hz) {
            _dx = chooseDir(hx, CUBE_X);
            _dy = chooseDir(hy, CUBE_Y);
            _dz = chooseDir(hz, CUBE_Z);
            if (_dx == 0 && _dy == 0 && _dz == 0) {   // never freeze in place
                if      (hx) _dx = hx;
                else if (hz) _dz = hz;
                else if (hy) _dy = hy;
            }
        }

        _hueBase += 1500;

        // light the single head voxel at full brightness
        int head = bufIndex(_x, _y, _z);
        _val[head] = 255;
        _hue[head] = _hueBase;

        // repaint the volume from the trail buffer (3-arg setPixel = serpentine-correct)
        for (int y = 0; y < CUBE_Y; y++) {
            for (int z = 0; z < CUBE_Z; z++) {
                for (int x = 0; x < CUBE_X; x++) {
                    int i = bufIndex(x, y, z);
                    cube.setPixel(x, y, z, Cube::colorHSV(_hue[i], 255, _val[i]));
                }
            }
        }
        cube.show();
    }

private:
    // plain logical index into the trail buffer (not the wiring order)
    static int bufIndex(int x, int y, int z) {
        return (y * CUBE_Z + z) * CUBE_X + x;
    }

    // step one axis, reflecting off the [0, n-1] walls. Returns 0 if no wall was
    // hit, +1 if it hit the low wall (inward is now +1), -1 if it hit the high
    // wall (inward is now -1). A single-cell axis (n <= 1) never moves.
    static int bounce(int& p, int& d, int n) {
        if (n <= 1) { d = 0; return 0; }
        p += d;
        if (p < 0)   { p = 1;     d = 1;  return +1; }
        if (p >= n)  { p = n - 2; d = -1; return -1; }
        return 0;
    }

    // pick a new step for one axis after a bounce. A free axis (hit == 0) may go
    // -1, 0 or +1; an axis that just hit a wall may only rest (0) or head back
    // inward, never straight back into the wall it just left.
    static int chooseDir(int hit, int n) {
        if (n <= 1)  return 0;
        if (hit > 0) return random(0, 2) ? 1 : 0;    // hit low wall: +1 (inward) or rest
        if (hit < 0) return random(0, 2) ? -1 : 0;   // hit high wall: -1 (inward) or rest
        return random(0, 3) - 1;                      // free axis: -1, 0, or +1
    }

    // random unit step, -1 or +1
    static int randSign() {
        return random(0, 2) ? 1 : -1;
    }

    uint32_t _stepMs;
    uint32_t _lastMs;
    uint8_t _fade;
    uint8_t _val[CUBE_LEDS];
    uint16_t _hue[CUBE_LEDS];
    int _x, _y, _z, _dx, _dy, _dz;
    uint16_t _hueBase;
};
