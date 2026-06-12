#include <Arduino.h>
#include <Preferences.h>
#include "Cube.h"
#include "Text.h"
#include "modes/ModeWaves.h"
#include "modes/ModeRainbow.h"
#include "modes/ModeBreath.h"
#include "modes/ModeRipple.h"
#include "modes/ModeComet.h"
#include "modes/ModeSpiral.h"
#include "modes/ModePlane.h"
#include "modes/ModeSurface.h"
#include "modes/ModeRain.h"
#include "modes/ModeTetris.h"

#define DATA_PIN 14
#define CONTROL_BUTTON_PIN 34
#define BOOT_BUTTON_PIN 0

constexpr int INITIAL_MODE = 0;
constexpr uint32_t DEBOUNCE_MS = 50;

Cube cube(DATA_PIN);

ModeWaves modeWaves;
ModeRainbow modeRainbow;
ModeBreath modeBreath;
ModeRipple modeRipple;
ModeComet modeComet;
ModeSpiral modeSpiral;
ModePlane modePlane;
ModeSurface modeSurface;
ModeRain modeRain;
ModeTetris modeTetris;

Mode* modes[] = { &modeWaves,
                  &modeTetris, &modeRipple, &modeSpiral, &modeComet,
                  &modePlane, &modeSurface,
                  &modeRain, &modeRainbow, &modeBreath };
constexpr int NUM_MODES = sizeof(modes) / sizeof(modes[0]);

Preferences prefs;
TextScroll modeLabel;

int modeIndex = INITIAL_MODE;
Mode* currentMode = modes[INITIAL_MODE];
bool modePending = false;    // true while the mode name is scrolling

struct Button {
    int pin;
    bool lastState;
    bool pending;
    uint32_t pendingMs;
};

Button controlButton = { CONTROL_BUTTON_PIN, HIGH, false, 0 };
Button bootButton = { BOOT_BUTTON_PIN, HIGH, false, 0 };

void setMode(int index) {
    currentMode->onExit(cube);
    modeIndex = index % NUM_MODES;
    currentMode = modes[modeIndex];
    prefs.putInt("mode", modeIndex);
    // show the mode label; the mode itself starts once it finishes.
    // give each mode its own hue, evenly spread around the colour wheel.
    char label[8];
    snprintf(label, sizeof(label), "%d", modeIndex + 1);  // 1-based for display
    uint16_t hue = (uint32_t)modeIndex * 65536UL / NUM_MODES;
    modeLabel.start(label, millis(), Cube::colorHSV(hue));
    modePending = true;
}

bool wasPressed(Button& btn, uint32_t now) {
    bool state = digitalRead(btn.pin);
    if (state != btn.lastState) {
        btn.lastState = state;
        btn.pending = true;
        btn.pendingMs = now;
    }
    if (btn.pending && (now - btn.pendingMs >= DEBOUNCE_MS)) {
        btn.pending = false;
        if (btn.lastState == LOW) {
            return true;
        }
    }
    return false;
}

void setup() {
    Serial.begin(115200);
    Serial.println("LED Cube boot OK");
    pinMode(CONTROL_BUTTON_PIN, INPUT_PULLUP);
    pinMode(BOOT_BUTTON_PIN, INPUT_PULLUP);
    controlButton.lastState = digitalRead(CONTROL_BUTTON_PIN);
    bootButton.lastState = digitalRead(BOOT_BUTTON_PIN);
    cube.begin();
    cube.setBrightness(50);  // 80 =  ~4.1 A worst case (all-white) — safe on the ~5 A PD board
    bool prefsOk = prefs.begin("led-cube", false);
    int savedMode = prefs.getInt("mode", INITIAL_MODE);
    Serial.printf("prefs open: %s, saved mode: %d\n", prefsOk ? "ok" : "FAILED", savedMode);
    setMode(savedMode);
}

void loop() {
    uint32_t now = millis();

    if (wasPressed(bootButton, now)) {
        setMode(modeIndex + 1);
        Serial.printf("mode -> %d\n", modeIndex);
    }

    // while the mode name is scrolling, hold off starting the mode
    if (modePending) {
        modeLabel.update(cube, now);
        if (modeLabel.active()) {
            return;
        }
        currentMode->onEnter(cube);
        modePending = false;
    }

    currentMode->update(cube, now);
}
