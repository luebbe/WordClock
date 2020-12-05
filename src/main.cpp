#include <FastLED.h>
#include "WordClock.h"

#include "secrets.h"

#define LED_PIN 5 // D1 on D1 Mini

#define COLOR_ORDER GRB
#define CHIPSET WS2812

#define BRIGHTNESS 20
const float UTC_OFFSET = 1;

TimeClient timeClient(UTC_OFFSET);

// Params for width, height and number of extra LEDs are defined in WordClock.h
#define NUM_LEDS (MATRIX_WIDTH * MATRIX_HEIGHT) + MINUTE_LEDS + SECOND_LEDS
CRGB leds_plus_safety_pixel[NUM_LEDS + 1];    // The first pixel in this array is the safety pixel for "out of bounds" results. Never use this array directly!
CRGB *const leds(leds_plus_safety_pixel + 1); // This is the "off-by-one" array that we actually work with and which is passed to FastLED!

WordClock wordClock(leds, &timeClient);

void setup()
{
  Serial.begin(SERIAL_SPEED);

  FastLED.addLeds<CHIPSET, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalSMD5050);

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

  wordClock.loop();
}
