#include <FastLED.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <NTPClient.h>
#include <TimeZone.h>
#include <TimeLib.h>
#include <time.h>

#include "WordClock.h"
#include "secrets.h"

#define LED_PIN 5 // D1 on D1 Mini

#define COLOR_ORDER GRB
#define CHIPSET WS2812

#define BRIGHTNESS 20

const char *TC_SERVER = "europe.pool.ntp.org";

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, TC_SERVER);

// For starters use hardwired Central European Time (Berlin, Paris, ...)
TimeChangeRule CEST = {"CEST", Last, Sun, Mar, 2, 120}; // Central European Summer Time
TimeChangeRule CET = {"CET ", Last, Sun, Oct, 3, 60};   // Central European Standard Time
Timezone Europe(CEST, CET);

// Params for width, height and number of extra LEDs are defined in WordClock.h
#define NUM_LEDS (MATRIX_WIDTH * MATRIX_HEIGHT) + MINUTE_LEDS + SECOND_LEDS
CRGB leds_plus_safety_pixel[NUM_LEDS + 1];    // The first pixel in this array is the safety pixel for "out of bounds" results. Never use this array directly!
CRGB *const leds(leds_plus_safety_pixel + 1); // This is the "off-by-one" array that we actually work with and which is passed to FastLED!

bool onGetTime(int &hours, int &minutes, int &seconds);
WordClock wordClock(leds, onGetTime);

bool onGetTime(int &hours, int &minutes, int &seconds)
{
  if (timeClient.update())
  {
    time_t localTime = Europe.toLocal(timeClient.getEpochTime());

    hours = ((localTime % 86400L) / 3600) % 24;
    minutes = (localTime % 3600) / 60;
    seconds = localTime % 60;

    return true;
  }
  else
  {
    return false;
  }
}

void setup()
{
  Serial.begin(SERIAL_SPEED);

  FastLED.addLeds<CHIPSET, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);

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

  // initialize NTP Client after WiFi is connected
  timeClient.begin();
}

void loop()
{
  wordClock.loop();
}
