#include <Arduino.h>
#include <Adafruit_NeoPixel.h>

#define DATA_PIN 14
#define NUM_LEDS (6*6*6) - 1

Adafruit_NeoPixel strip(NUM_LEDS, DATA_PIN, NEO_RGB + NEO_KHZ800);

static int counter = 0;

void setup() {
    Serial.begin(115200);

    strip.begin();
    strip.setBrightness(1);
    strip.show();
}

void loop() {
    for (int i = 0; i < NUM_LEDS; i++) {
        strip.setPixelColor(i, strip.Color(255, 255, 255));
    }
    strip.show();
    delay(100);
    for (int i = 0; i < NUM_LEDS; i++) {
        strip.setPixelColor(i, strip.Color(0, 0, 0));
    }
    strip.show();
    delay(100);
}
