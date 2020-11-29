#include <FastLED.h>
#include "RainbowAnimation.h"

#define LED_PIN 5 // D1 on D1 Mini

#define COLOR_ORDER GRB
#define CHIPSET WS2812

#define BRIGHTNESS 5

// Params for width and height
const uint8_t matrixWidth = 11;
const uint8_t matrixHeight = 10;

#define NUM_LEDS (matrixWidth * matrixHeight) // + minutePixels + secondPixels
CRGB leds_plus_safety_pixel[NUM_LEDS + 1];      // The first pixel in this array is the safety pixel for "out of bounds" results. Never use this array directly!
CRGB *const leds(leds_plus_safety_pixel + 1);   // This is the "off-by-one" array that we actually work with and which is passed to FastLED!

RainbowAnimation rainbowAnimation(leds, matrixWidth, matrixHeight);

void setup()
{
  FastLED.addLeds<CHIPSET, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalSMD5050);
  FastLED.setBrightness(BRIGHTNESS);
  rainbowAnimation.setBrightness(BRIGHTNESS);
}

void loop()
{
  rainbowAnimation.loop();
}
