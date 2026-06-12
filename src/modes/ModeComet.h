#pragma once
#include "../Mode.h"
#include <Arduino.h>
#include <math.h>
#include <string.h>

// A glowing comet that flies through the cube under real bounce physics: its
// position and velocity are tracked in floating point and it reflects off all
// six walls exactly (angle in = angle out) at a constant speed — no gravity, no
// drag, no energy loss, just an ideal elastic ball ricocheting around the box.
//
// The head is drawn with a small anti-aliased splat (a separable tent kernel):
// the voxels around the float position light up so that their luminance-weighted
// centroid equals the true position, which is what makes the sub-cell location
// read on the coarse 6x6x6 grid and lets the head glide smoothly between cells
// instead of snapping from one to the next. That same footprint is blended into
// a fading trail buffer, so the comet leaves a tail behind it. The head holds
// one colour while it flies and jumps to a new hue only when it collides with a
// wall, so each straight run between bounces is a solid colour with the colour
// breaks falling exactly at the bounce points.
//
// This single mode replaces the two it was merged from: the old ModeComet had a
// trail but re-rolled a random direction on each bounce (unphysical) and only a
// crisp one-voxel head, and ModeBounce3D had the exact reflection physics and
// the soft sphere but no trail. The trail buffer is logical and painted back
// through cube.setPixel(x,y,z,...) so it follows the serpentine wiring.
class ModeComet : public Mode {
public:
    explicit ModeComet(uint32_t stepMs = 33, uint8_t fade = 6, float radius = 1.3f,
                       float gamma = 2.2f)
        : _stepMs(stepMs), _lastMs(0), _fade(fade), _radius(radius), _gamma(gamma) {
    }

    void onEnter(Cube& cube) override {
        memset(_val, 0, sizeof(_val));
        memset(_hue, 0, sizeof(_hue));
        // start near the centre with uneven velocities so the elastic path does
        // not collapse into a short repeating loop. Constant speed: these are
        // never scaled, only sign-flipped on a bounce.
        _px = (CUBE_X - 1) * 0.5f;
        _py = (CUBE_Y - 1) * 0.5f;
        _pz = (CUBE_Z - 1) * 0.5f;
        _vx = 0.070f;
        _vy = (CUBE_Y > 1) ? 0.045f : 0.0f;   // hold still in y when running a single layer
        _vz = 0.090f;
        _hueBase = 0;
        cube.clear();
        cube.show();
    }

    void update(Cube& cube, uint32_t ms) override {
        if (ms - _lastMs < _stepMs) {
            return;
        }
        _lastMs = ms;

        // fade the whole trail one notch
        for (int i = 0; i < CUBE_LEDS; i++) {
            _val[i] = (_val[i] > _fade) ? _val[i] - _fade : 0;
        }

        // advance the head and reflect off the six walls at constant speed.
        // use | (not ||) so every axis is stepped even if an earlier one bounced.
        bool bounced = reflect(_px, _vx, CUBE_X)
                     | reflect(_py, _vy, CUBE_Y)
                     | reflect(_pz, _vz, CUBE_Z);
        // change colour only on a wall collision; a golden-angle hop keeps
        // consecutive bounce colours distinct. Between bounces the hue is held,
        // so the head and the trail it lays down stay a single solid colour.
        if (bounced) {
            _hueBase += 40503;
        }

        // stamp the head with a SEPARABLE TENT kernel: per-axis linear weight
        // (1 at the centre, falling to 0 at +/- radius), the three multiplied
        // together. Two properties of this kernel are what kill the "teleporting"
        // look on a 6-wide grid: the luminance-weighted centroid of the lit
        // voxels equals the true float position exactly (so the head never sticks
        // to the nearest cell and then snaps to the next), and the total light
        // stays ~constant as it crosses a cell boundary (so it doesn't dim and
        // flicker mid-step). The old radial *squared* falloff broke both. max-
        // blend keeps the brighter of the fresh head and the fading trail, and a
        // voxel the head freshly lights takes the current hue, so the tail keeps
        // the colour the head wore when it passed.
        int x0 = clampLo(_px - _radius), x1 = clampHi(_px + _radius, CUBE_X);
        int y0 = clampLo(_py - _radius), y1 = clampHi(_py + _radius, CUBE_Y);
        int z0 = clampLo(_pz - _radius), z1 = clampHi(_pz + _radius, CUBE_Z);
        for (int y = y0; y <= y1; y++) {
            float wy = 1.0f - fabsf(y - _py) / _radius;
            if (wy <= 0.0f) continue;
            for (int z = z0; z <= z1; z++) {
                float wz = 1.0f - fabsf(z - _pz) / _radius;
                if (wz <= 0.0f) continue;
                for (int x = x0; x <= x1; x++) {
                    float wx = 1.0f - fabsf(x - _px) / _radius;
                    if (wx <= 0.0f) continue;
                    // gamma-encode the linear tent weight so that *perceived*
                    // brightness ramps linearly as the head nears a voxel instead
                    // of snapping on. The eye's response to LED PWM is roughly
                    // value^(1/gamma), so pre-raising the weight to _gamma makes
                    // perceived brightness ~proportional to the tent weight.
                    uint8_t bv = (uint8_t)(powf(wx * wy * wz, _gamma) * 255.0f);
                    int i = bufIndex(x, y, z);
                    if (bv > _val[i]) {
                        _val[i] = bv;
                        _hue[i] = _hueBase;
                    }
                }
            }
        }

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

    // advance one axis by its (constant-magnitude) velocity, reflecting off the
    // [0, n-1] walls. Returns true if it bounced off a wall this step. A
    // single-cell axis (n <= 1) never moves and never bounces.
    static bool reflect(float& p, float& v, int n) {
        if (n <= 1) { v = 0.0f; return false; }
        p += v;
        bool bounced = false;
        if (p < 0.0f)           { p = -p;                  v = -v; bounced = true; }
        if (p > (float)(n - 1)) { p = 2.0f * (n - 1) - p;  v = -v; bounced = true; }
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
    uint8_t _fade;
    float _radius;
    float _gamma;    // perceptual gamma for the head's brightness falloff
    uint8_t _val[CUBE_LEDS];
    uint16_t _hue[CUBE_LEDS];
    float _px, _py, _pz;
    float _vx, _vy, _vz;
    uint16_t _hueBase;
};
