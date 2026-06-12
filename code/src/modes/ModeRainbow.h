#pragma once
#include "../Mode.h"

class ModeRainbow : public Mode {
public:
    explicit ModeRainbow(uint32_t stepMs = 20) : _stepMs(stepMs), _lastMs(0), _offset(0) {
    }

    void update(Cube& cube, uint32_t ms) override {
        if (ms - _lastMs < _stepMs) {
            return;
        }
        _lastMs = ms;
        _offset += 256;

        int maxSpread = (CUBE_X - 1) + (CUBE_Y - 1) + (CUBE_Z - 1);
        if (maxSpread < 1) maxSpread = 1;
        for (int y = 0; y < CUBE_Y; y++) {
            for (int z = 0; z < CUBE_Z; z++) {
                for (int x = 0; x < CUBE_X; x++) {
                    uint16_t hue = _offset + (uint16_t)((x + y + z) * 65536 / maxSpread);
                    cube.setPixel(x, y, z, Cube::colorHSV(hue));
                }
            }
        }
        cube.show();
    }

private:
    uint32_t _stepMs;
    uint32_t _lastMs;
    uint16_t _offset;
};
