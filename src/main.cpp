#include <FastLED.h>
// #include "RainbowAnimation.h"
#include "WordClock.h"
#include "secrets.h"

#define LED_PIN 5 // D1 on D1 Mini

#define COLOR_ORDER GRB
#define CHIPSET WS2812

#define BRIGHTNESS 5
const float UTC_OFFSET = 1;

// Params for width, height and number of extra LEDs are defined in WordClock.h

#define NUM_LEDS (MATRIX_WIDTH * MATRIX_HEIGHT) + MINUTE_LEDS + SECOND_LEDS
CRGB leds_plus_safety_pixel[NUM_LEDS + 1];                // The first pixel in this array is the safety pixel for "out of bounds" results. Never use this array directly!
CRGB *const leds(leds_plus_safety_pixel + 1);             // This is the "off-by-one" array that we actually work with and which is passed to FastLED!

// RainbowAnimation rainbowAnimation(leds, MATRIX_WIDTH, MATRIX_HEIGHT);
WordClock wordClock(leds, UTC_OFFSET);

void setup()
{
  Serial.begin(SERIAL_SPEED);

  // Connect to WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");

  FastLED.addLeds<CHIPSET, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalSMD5050);
  FastLED.setBrightness(BRIGHTNESS);
  // rainbowAnimation.setBrightness(BRIGHTNESS);

  wordClock.setBrightness(BRIGHTNESS);
  wordClock.setup();
}

void loop()
{
  // rainbowAnimation.loop();
  wordClock.loop();
}
