#include <Arduino.h>
#include <Adafruit_NeoPixel.h>

#define DATA_PIN 14
#define NUM_LEDS 4

Adafruit_NeoPixel strip(NUM_LEDS, DATA_PIN, NEO_RGB + NEO_KHZ800);

static int counter = 0;

void setup() {
    Serial.begin(115200);

    strip.begin();
    strip.setBrightness(50);
    strip.show();
}

void loop() {
    strip.setPixelColor(0, strip.Color(255, 0, 0));
    strip.setPixelColor(1, strip.Color(0, 0, 0));
    strip.setPixelColor(2, strip.Color(0, 255, 0));
    strip.setPixelColor(3, strip.Color(0, 0, 255));
    strip.show();
    delay(100);
    strip.setPixelColor(0, strip.Color(0, 0, 0));
    strip.setPixelColor(1, strip.Color(255, 0, 0));
    strip.setPixelColor(2, strip.Color(0, 0, 0));
    strip.setPixelColor(3, strip.Color(0, 0, 0));
    strip.show();
    delay(100);
}
