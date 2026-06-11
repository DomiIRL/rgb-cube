#pragma once
#include "../Mode.h"
#include <math.h>

class ModeBreath : public Mode {
public:
    explicit ModeBreath(uint32_t stepMs = 20, uint32_t periodMs = 3000, uint32_t hueDriftMs = 20000)
        : _stepMs(stepMs),
          _lastMs(0),
          _phase(0.0f),
          _hue(0),
          _phaseStep(2.0f * (float)M_PI / (float)(periodMs / stepMs)),
          _hueStep((uint16_t)(65536u / (hueDriftMs / stepMs))) {
    }

    void update(Cube& cube, uint32_t ms) override {
        if (ms - _lastMs < _stepMs) {
            return;
        }
        _lastMs = ms;

        uint8_t brightness = (uint8_t)((1.0f + sinf(_phase)) * 0.5f * 255.0f);
        uint32_t color = Cube::colorHSV(_hue, 255, brightness);
        for (int i = 0; i < CUBE_LEDS; i++) {
            cube.setPixel(i, color);
        }
        cube.show();

        _phase += _phaseStep;
        if (_phase >= 2.0f * (float)M_PI) {
            _phase -= 2.0f * (float)M_PI;
        }
        _hue += _hueStep;
    }

private:
    uint32_t _stepMs;
    uint32_t _lastMs;
    float _phase;
    float _phaseStep;
    uint16_t _hue;
    uint16_t _hueStep;
};
