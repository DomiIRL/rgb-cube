#include <Arduino.h>
#include <Preferences.h>
// Leave ALL protocol decoders enabled so any remote shows up in the serial log.
// Once the remote's protocol is confirmed, narrow with e.g. `#define DECODE_NEC` to save flash.
#include <IRremote.hpp>
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
#define CONTROL_BUTTON_PIN 33
#define BOOT_BUTTON_PIN 0
#define IR_RECEIVE_PIN 34

constexpr int INITIAL_MODE = 0;
constexpr uint32_t DEBOUNCE_MS = 50;

// Elegoo IR remote — NEC command bytes (watch the serial log to confirm for your unit)
constexpr uint8_t IR_POWER     = 0x45;  // power      -> blank / unblank the cube
constexpr uint8_t IR_VOL_UP    = 0x46;  // VOL+       -> brighter
constexpr uint8_t IR_FUNC_STOP = 0x47;  // FUNC/STOP  -> restart current mode
constexpr uint8_t IR_PREV      = 0x44;  // |<<        -> previous mode
constexpr uint8_t IR_PLAY      = 0x40;  // >||        -> pause / resume animation
constexpr uint8_t IR_NEXT      = 0x43;  // >>|        -> next mode
constexpr uint8_t IR_DOWN      = 0x07;  // arrow down -> previous mode
constexpr uint8_t IR_VOL_DOWN  = 0x15;  // VOL-       -> dimmer
constexpr uint8_t IR_UP        = 0x09;  // arrow up   -> next mode
constexpr uint8_t IR_EQ        = 0x19;  // EQ         -> factory reset (all settings to defaults)
constexpr uint8_t IR_ST_REPT   = 0x0D;  // ST/REPT    -> toggle auto-cycle
constexpr uint8_t IR_0         = 0x16;  // number keys -> jump straight to a mode
constexpr uint8_t IR_1         = 0x0C;
constexpr uint8_t IR_2         = 0x18;
constexpr uint8_t IR_3         = 0x5E;
constexpr uint8_t IR_4         = 0x08;
constexpr uint8_t IR_5         = 0x1C;
constexpr uint8_t IR_6         = 0x5A;
constexpr uint8_t IR_7         = 0x42;
constexpr uint8_t IR_8         = 0x52;
constexpr uint8_t IR_9         = 0x4A;

// Brightness limits (NeoPixel scale). Keep max sane: ~80 ≈ 4.1 A full-white on the ~5 A PD board.
constexpr uint8_t BRIGHT_MIN     = 5;
constexpr uint8_t BRIGHT_MAX     = 80;
constexpr uint8_t BRIGHT_STEP    = 10;
constexpr uint8_t BRIGHT_DEFAULT = 40;

constexpr uint32_t AUTO_CYCLE_MS = 15000;  // auto-advance interval when ST/REPT is on
constexpr uint32_t MULTI_DIGIT_MS = 1000;  // window to combine number keys into one mode number

constexpr float SPEED_MIN    = 0.25f;      // |<< / >>| scale the current mode's animation speed
constexpr float SPEED_MAX    = 4.0f;
constexpr float SPEED_FACTOR = 1.5f;       // multiply / divide per press

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

uint8_t brightness = BRIGHT_DEFAULT;
bool displayOn = true;       // false = blanked via the Power button
bool paused = false;         // true = animation frozen via Play/Pause
bool autoCycle = false;      // true = auto-advance through the modes
uint32_t lastCycleMs = 0;    // last time the auto-cycle (or a mode change) fired

int typedNumber = 0;         // number-key entry so far (1-based mode #, 0 = none pending)
uint32_t lastDigitMs = 0;    // last number-key press, for the multi-digit window

float speed = 1.0f;          // animation speed multiplier (|<< slower, >>| faster)
uint32_t modeClock = 0;      // speed-scaled virtual time fed to the current mode's update()
uint32_t lastRealMs = 0;     // real millis() at the previous loop, for the clock delta
float clockFrac = 0.0f;      // sub-ms remainder carried between frames (keeps slow speeds smooth)

struct Button {
    int pin;
    bool lastState;
    bool pending;
    uint32_t pendingMs;
};

Button controlButton = { CONTROL_BUTTON_PIN, HIGH, false, 0 };
Button bootButton = { BOOT_BUTTON_PIN, HIGH, false, 0 };

// Persist every user setting to NVS (setMode also calls this on manual mode changes).
void saveSettings() {
    prefs.putInt("mode", modeIndex);
    prefs.putUChar("bright", brightness);
    prefs.putFloat("speed", speed);
    prefs.putBool("paused", paused);
    prefs.putBool("display", displayOn);
    prefs.putBool("cycle", autoCycle);
}

void applyBrightness(int value) {
    if (value < BRIGHT_MIN) value = BRIGHT_MIN;
    if (value > BRIGHT_MAX) value = BRIGHT_MAX;
    brightness = (uint8_t)value;
    cube.setBrightness(brightness);
    cube.show();   // re-scale the current frame so the change shows immediately
    saveSettings();
    Serial.printf("brightness -> %d\n", brightness);
}

void changeSpeed(float factor) {
    speed *= factor;
    if (speed < SPEED_MIN) speed = SPEED_MIN;
    if (speed > SPEED_MAX) speed = SPEED_MAX;
    saveSettings();
    Serial.printf("speed -> %.2fx\n", speed);
}

void setMode(int index, bool manual = true) {
    currentMode->onExit(cube);
    modeIndex = ((index % NUM_MODES) + NUM_MODES) % NUM_MODES;  // wrap; safe for negatives
    currentMode = modes[modeIndex];
    displayOn = true;          // any mode change wakes the display...
    paused = false;            // ...and resumes animation
    if (manual) autoCycle = false;  // a manual mode change stops auto-cycling
    lastCycleMs = millis();    // restart the auto-cycle clock
    typedNumber = 0;           // end any multi-digit entry (enterDigit re-sets it afterwards)
    if (manual) saveSettings();                   // persist mode + the reset flags
    else        prefs.putInt("mode", modeIndex);  // auto-cycle: only the mode changed
    // show the mode label; the mode itself starts once it finishes.
    // give each mode its own hue, evenly spread around the colour wheel.
    char label[8];
    snprintf(label, sizeof(label), "%d", modeIndex + 1);  // 1-based for display
    uint16_t hue = (uint32_t)modeIndex * 65536UL / NUM_MODES;
    modeLabel.start(label, millis(), Cube::colorHSV(hue));
    modePending = true;
}

// A number key was pressed. Combine digits typed within MULTI_DIGIT_MS into one mode number
// (so "1" then "0" -> mode 10). If the combined number has no matching mode, fall back to the
// single digit just pressed (so "4" then "2" -> mode 2, since there is no mode 42).
void enterDigit(int digit, uint32_t now) {
    int candidate = digit;
    if (typedNumber > 0 && now - lastDigitMs <= MULTI_DIGIT_MS) {
        int combined = typedNumber * 10 + digit;
        if (combined >= 1 && combined <= NUM_MODES) {
            candidate = combined;     // extend the current number
        }                             // else: no such mode -> keep just this digit
    }
    lastDigitMs = now;
    if (candidate >= 1 && candidate <= NUM_MODES) {
        setMode(candidate - 1);       // typed number is 1-based; mode index is 0-based
        typedNumber = candidate;      // setMode cleared it; restore so the next digit can extend
    } else {
        typedNumber = 0;              // a lone "0" has no mode — ignore and reset entry
    }
}

// EQ button: wipe stored settings and return everything to defaults.
void factoryReset() {
    Serial.println("FACTORY RESET -> defaults");
    prefs.clear();                 // erase the saved namespace
    brightness = BRIGHT_DEFAULT;
    speed      = 1.0f;
    cube.setBrightness(brightness);
    setMode(INITIAL_MODE);         // resets the flags and re-persists all defaults
}

// Map one decoded remote command to an action. Repeat frames are filtered upstream.
void handleIrCommand(uint16_t cmd, uint32_t now) {
    switch (cmd) {
        // mode navigation (arrows; also the BOOT button and number keys)
        case IR_UP:   setMode(modeIndex + 1); break;
        case IR_DOWN: setMode(modeIndex - 1); break;

        // animation speed of the current mode (skip buttons)
        case IR_NEXT: changeSpeed(SPEED_FACTOR);        break;  // >>|  faster
        case IR_PREV: changeSpeed(1.0f / SPEED_FACTOR); break;  // |<<  slower

        // number keys -> select a mode by number (multi-digit; "1"+"0" = mode 10, see enterDigit)
        case IR_1: enterDigit(1, now); break;
        case IR_2: enterDigit(2, now); break;
        case IR_3: enterDigit(3, now); break;
        case IR_4: enterDigit(4, now); break;
        case IR_5: enterDigit(5, now); break;
        case IR_6: enterDigit(6, now); break;
        case IR_7: enterDigit(7, now); break;
        case IR_8: enterDigit(8, now); break;
        case IR_9: enterDigit(9, now); break;
        case IR_0: enterDigit(0, now); break;

        // brightness
        case IR_VOL_UP:   applyBrightness(brightness + BRIGHT_STEP); break;
        case IR_VOL_DOWN: applyBrightness(brightness - BRIGHT_STEP); break;
        case IR_EQ:       factoryReset(); break;   // wipe all settings -> defaults

        // power / playback
        case IR_POWER:
            displayOn = !displayOn;
            if (!displayOn) { cube.clear(); cube.show(); }
            saveSettings();
            Serial.printf("display %s\n", displayOn ? "ON" : "OFF");
            break;
        case IR_PLAY:
            paused = !paused;
            saveSettings();
            Serial.println(paused ? "paused" : "resumed");
            break;
        case IR_FUNC_STOP:
            setMode(modeIndex);   // restart the current mode (replays its label)
            break;
        case IR_ST_REPT:
            autoCycle = !autoCycle;
            lastCycleMs = now;
            saveSettings();
            Serial.printf("auto-cycle %s\n", autoCycle ? "ON" : "OFF");
            break;

        default:
            Serial.printf("unmapped IR cmd 0x%02X\n", cmd);
            break;
    }
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
    IrReceiver.begin(IR_RECEIVE_PIN);  // no on-board LED feedback
    Serial.printf("IR receiver started on GPIO%d, active protocols: ", IR_RECEIVE_PIN);
    IrReceiver.printActiveIRProtocols(&Serial);
    Serial.println();
    cube.begin();

    // restore all saved settings (defaults match the global initialisers)
    bool prefsOk  = prefs.begin("led-cube", false);
    int savedMode = prefs.getInt("mode", INITIAL_MODE);
    brightness    = prefs.getUChar("bright", BRIGHT_DEFAULT);
    speed         = prefs.getFloat("speed", 1.0f);
    bool sPaused  = prefs.getBool("paused", false);
    bool sDisplay = prefs.getBool("display", true);
    bool sCycle   = prefs.getBool("cycle", false);
    if (brightness < BRIGHT_MIN) brightness = BRIGHT_MIN;
    if (brightness > BRIGHT_MAX) brightness = BRIGHT_MAX;
    if (speed < SPEED_MIN) speed = SPEED_MIN;
    if (speed > SPEED_MAX) speed = SPEED_MAX;
    cube.setBrightness(brightness);
    Serial.printf("prefs %s | mode %d bright %d speed %.2f paused %d display %d cycle %d\n",
                  prefsOk ? "ok" : "FAILED", savedMode, brightness, speed, sPaused, sDisplay, sCycle);

    setMode(savedMode, false);   // restore mode without resetting/over-persisting the flags
    displayOn = sDisplay;        // ...then re-apply the saved flags that setMode forced
    paused    = sPaused;
    autoCycle = sCycle;
    saveSettings();              // persist the fully-restored state
    lastRealMs = millis();
}

void loop() {
    uint32_t now = millis();
    uint32_t delta = now - lastRealMs;   // real time elapsed since the previous loop
    lastRealMs = now;

    if (wasPressed(bootButton, now)) {
        setMode(modeIndex + 1);
        Serial.printf("mode -> %d\n", modeIndex);
    }

    if (IrReceiver.decode()) {
        // DEBUG: dump every frame — protocol, address, command, raw data, repeat flag.
        // Read the Command=0x.. value for your button and set the matching IR_* constant.
        IrReceiver.printIRResultShort(&Serial);

        IRData& ir = IrReceiver.decodedIRData;
        // ignore the auto-repeat frames sent while a button is held down
        if (!(ir.flags & IRDATA_FLAGS_IS_REPEAT)) {
            handleIrCommand(ir.command, now);
        }
        IrReceiver.resume();  // ready for the next frame
    }

    // auto-advance through the modes when ST/REPT is on
    if (autoCycle && displayOn && !paused && !modePending && now - lastCycleMs >= AUTO_CYCLE_MS) {
        setMode(modeIndex + 1, false);   // false = automatic, so it doesn't cancel auto-cycle
    }

    // blanked via Power — keep the cube dark, skip all rendering
    if (!displayOn) {
        return;
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

    // frozen via Play/Pause — leave the last frame on the cube
    if (paused) {
        return;
    }

    // advance the speed-scaled clock now that we know a frame will render, so pausing,
    // blanking, or a label scroll never makes the animation jump when it resumes.
    float advance = (float)delta * speed + clockFrac;
    uint32_t whole = (uint32_t)advance;
    clockFrac = advance - (float)whole;
    modeClock += whole;
    currentMode->update(cube, modeClock);
}
