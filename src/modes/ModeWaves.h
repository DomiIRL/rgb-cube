#pragma once
#include "../Mode.h"
#include <math.h>

// A single thin sheet of rainbow light sweeping through the cube along an axis
// that slowly tumbles in 3D. The brightness profile is a sharp sin^4 band, so
// only a thin slab lights at any moment (the rest stays dark) — that keeps it
// crisp, high-contrast AND within the cube's tight power budget, instead of a
// washed-out full-volume glow. Hue runs along the sweep axis so the sheet itself
// is a moving gradient of colour.
class ModeWaves : public Mode {
public:
    explicit ModeWaves(uint32_t stepMs = 28) : _stepMs(stepMs), _lastMs(0) {}

    void update(Cube& cube, uint32_t ms) override {
        if (ms - _lastMs < _stepMs) return;
        _lastMs = ms;
        const float t = ms * 0.001f;

        // sweep direction: azimuth turns, elevation nods → the sheet tumbles
        float az = t * 0.22f;
        float el = 0.55f * sinf(t * 0.11f);
        float dx = cosf(el) * cosf(az);
        float dy = sinf(el);
        float dz = cosf(el) * sinf(az);
        uint16_t baseHue = (uint16_t)(t * 3000.0f);
        const float c = (CUBE_X - 1) * 0.5f;

        cube.clear();
        for (int z = 0; z < CUBE_Z; z++)
        for (int y = 0; y < CUBE_Y; y++)
        for (int x = 0; x < CUBE_X; x++) {
            float s = (x - c) * dx + (y - c) * dy + (z - c) * dz;   // pos along axis
            float b = sinf(s * 0.7f - t * 1.7f);
            if (b < 0.0f) continue;
            b = b * b; b = b * b;                                   // sin^4 → thin band
            if (b < 0.02f) continue;
            uint16_t hue = (uint16_t)(baseHue + (int)(s * 6500.0f));
            cube.setPixel(x, y, z, Cube::colorHSV(hue, 255, (uint8_t)(b * 255.0f)));
        }
        cube.show();
    }

private:
    uint32_t _stepMs;
    uint32_t _lastMs;
};
