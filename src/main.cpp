#include <FastLED.h>
// #include "RainbowAnimation.h"
#include "WordClock.h"

#include "secrets.h"

#define LED_PIN 5 // D1 on D1 Mini

#define COLOR_ORDER GRB
#define CHIPSET WS2812

#define BRIGHTNESS 5
const float UTC_OFFSET = 1;

TimeClient timeClient(UTC_OFFSET);

// Params for width, height and number of extra LEDs are defined in WordClock.h

#define NUM_LEDS (MATRIX_WIDTH * MATRIX_HEIGHT) + MINUTE_LEDS + SECOND_LEDS
CRGB leds_plus_safety_pixel[NUM_LEDS + 1];    // The first pixel in this array is the safety pixel for "out of bounds" results. Never use this array directly!
CRGB *const leds(leds_plus_safety_pixel + 1); // This is the "off-by-one" array that we actually work with and which is passed to FastLED!

// LedMatrix matrix(leds, MATRIX_WIDTH, MATRIX_HEIGHT);
// RainbowAnimation rainbowAnimation(leds, MATRIX_WIDTH, MATRIX_HEIGHT);
WordClock wordClock(leds, &timeClient);

// unsigned long _updateInterval = 100;
// unsigned long _lastUpdate = 0;

void setup()
{
  Serial.begin(SERIAL_SPEED);

  FastLED.addLeds<CHIPSET, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalSMD5050);

  // rainbowAnimation.setBrightness(BRIGHTNESS);
  // rainbowAnimation.setup();

  wordClock.setBrightness(BRIGHTNESS);
  wordClock.setup();

  // Connect to WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    if (leds[0] != CRGB(0))
      leds[0] = 0;
    else
      leds[0] = CRGB(0xFFFFFFF);
    FastLED.show();
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  leds[0] = 0;
  FastLED.show();

  timeClient.updateTime();
  Serial.println("Time updated");
}

void loop()
{
  if (timeClient.getHours() == 0)
  {
    timeClient.updateTime();
    Serial.println("Time updated");
  }

  // rainbowAnimation.loop();
  wordClock.loop();

  // uint8_t secondHand = (millis() / 1000) % 60;
  // if ((millis() - _lastUpdate >= _updateInterval) || (_lastUpdate == 0))
  // {
  //   switch (secondHand)
  //   {
  //   case 0 ... 14:
  //     matrix.scrollUp();
  //     break;
  //   case 15 ... 29:
  //     matrix.scrollDown();
  //     break;
  //   case 30 ... 44:
  //     matrix.scrollLeft();
  //     break;
  //   case 45 ... 59:
  //     matrix.scrollRight();
  //     break;
  //   }

  //   FastLED.show();
  //   _lastUpdate = millis();
  // }
}
