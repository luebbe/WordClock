/*
 * Word clock using the LedMatrix base class.
 * 
 * Version: 1.0
 * Author: Lübbe Onken (http://github.com/luebbe)
 */

#include "WordClock.h"

WordClock::WordClock(CRGB *leds, uint8_t width, uint8_t height, bool serpentineLayout, bool vertical, float utcOffset)
    : LedMatrix(leds, width, height, serpentineLayout, vertical)
{
  _timeClient = new TimeClient(utcOffset);
}

void WordClock::setup()
{
  FastLED.setBrightness(_brightness);
  _timeClient->updateTime();
  clearAll();
}

void WordClock::loop()
{
  if ((millis() - _lastUpdate >= _updateInterval * 1000UL) || (_lastUpdate == 0))
  {
    _timeClient->updateTime();
    updateHoursAndMinutes();
    FastLED.show();
  }
}

void WordClock::updateHoursAndMinutes()
{
  std::array<uint8_t, 10> currentWords;
  int currentHour = _timeClient->getHours().toInt();
  int currentMinute = _timeClient->getMinutes().toInt();

  if (currentHour > 12)
    currentHour = currentHour - 12;

  int j = 0;
  currentWords[j++] = _ES_;
  currentWords[j++] = _IST_;

  if (currentMinute >= 2 && currentMinute < 7)
  {
    currentWords[j++] = _FUENF_;
    currentWords[j++] = _NACH_;
  }
  if (currentMinute >= 7 && currentMinute < 12)
  {
    currentWords[j++] = _ZEHN_;
    currentWords[j++] = _NACH_;
  }
  if (currentMinute >= 12 && currentMinute < 17)
  {
    currentWords[j++] = _VIERTEL_M_;
    currentWords[j++] = _NACH_;
  }
  if (currentMinute >= 17 && currentMinute < 22)
  {
    currentWords[j++] = _ZWANZIG_M_;
    currentWords[j++] = _NACH_;
  }
  if (currentMinute >= 22 && currentMinute < 27)
  {
    currentWords[j++] = _FUENF_;
    currentWords[j++] = _VOR_;
    currentWords[j++] = _HALB_M_;
    currentHour += 1;
  }
  if (currentMinute >= 27 && currentMinute < 32)
  {
    currentWords[j++] = _HALB_M_;
    currentHour += 1;
  }
  if (currentMinute >= 32 && currentMinute < 37)
  {
    currentWords[j++] = _FUENF_;
    currentWords[j++] = _NACH_;
    currentWords[j++] = _HALB_M_;
    currentHour += 1;
  }
  if (currentMinute >= 37 && currentMinute < 42)
  {
    currentWords[j++] = _ZWANZIG_M_;
    currentWords[j++] = _VOR_;
    currentHour += 1;
  }
  if (currentMinute >= 42 && currentMinute < 47)
  {
    currentWords[j++] = _VIERTEL_M_;
    currentWords[j++] = _VOR_;
  }
  if (currentMinute >= 47 && currentMinute < 52)
  {
    currentWords[j++] = _ZEHN_;
    currentWords[j++] = _VOR_;
    currentHour += 1;
  }
  if (currentMinute >= 52 && currentMinute < 57)
  {
    currentWords[j++] = _FUENF_;
    currentWords[j++] = _VOR_;
    currentHour += 1;
  }
  if (currentMinute >= 57 && currentMinute <= 59)
  {
    currentHour += 1;
  }

  switch (currentHour)
  {
  case 1:
    currentWords[j++] = _EINS_;
    break;
  case 2:
    currentWords[j++] = _ZWEI_;
    break;
  case 3:
    currentWords[j++] = _DREI_;
    break;
  case 4:
    currentWords[j++] = _VIER_;
    break;
  case 5:
    currentWords[j++] = _FUENF_;
    break;
  case 6:
    currentWords[j++] = _SECHS_;
    break;
  case 7:
    currentWords[j++] = _SIEBEN_;
    break;
  case 8:
    currentWords[j++] = _ACHT_;
    break;
  case 9:
    currentWords[j++] = _NEUN_;
    break;
  case 10:
    currentWords[j++] = _ZEHN_;
    break;
  case 11:
    currentWords[j++] = _ELF_;
    break;
  case 12:
    currentWords[j++] = _ZWOELF_;
    break;
  default:
    currentWords[j++] = _ZWOELF_;
    break;
  }

  if (_lastWords != currentWords)
  {
    Serial.printf("Sending %d words", j);
    clearAll();
    for (int i = 0; i <= j; i++)
      sendWord(currentWords[i]);
    _lastWords = currentWords;
  }
}

void WordClock::updateSeconds()
{
  int seconds = _timeClient->getSeconds().toInt();
}

int WordClock::randomRGB()
{
  return random(0, 0xFFFFFF);
}

void WordClock::sendWord(uint8_t index)
{
  CRGB actcolor = CRGB(randomRGB());
  for (int j = 0; j < TLEDS[index].len; j++)
  {
    _leds[XY(TLEDS[index].x + j, TLEDS[index].y)] = actcolor;
  }
}
