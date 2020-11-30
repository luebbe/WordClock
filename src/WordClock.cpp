/*
 * Word clock using the LedMatrix base class.
 * 
 * Version: 1.0
 * Author: Lübbe Onken (http://github.com/luebbe)
 */

#include "WordClock.h"

WordClock::WordClock(CRGB *leds, float utcOffset)
    : LedMatrix(leds, MATRIX_WIDTH, MATRIX_HEIGHT), _updateInterval(1), _lastUpdate(0)
{
  _timeClient = new TimeClient(utcOffset);
  _lastWords.fill(0);

  //##DEBUG REMOVE
  _minuteLEDs = (_matrixLEDs + 7); // last four LEDs in bottom row for debugging
  // _minuteLEDs = (_matrixLEDs + _width * _height);  // Pointer to the start of the buffer for the minute LEDs
  _secondLEDs = (_minuteLEDs + MINUTE_LEDS); // Pointer to the start of the buffer for the second LEDs
}

void WordClock::setup()
{
  LedMatrix::setup();

  //##DEBUG REMOVE
  Serial.printf("%10d %10d %10d\r\n", _matrixLEDs, _minuteLEDs, _secondLEDs);

  _timeClient->updateTime();
}

void WordClock::loop()
{
  LedMatrix::loop();

  if ((millis() - _lastUpdate >= _updateInterval * 1000UL) || (_lastUpdate == 0))
  {
    _timeClient->updateTime();
    updateHoursAndMinutes();
    updateSeconds();
    FastLED.show();
  }
}

void WordClock::updateHoursAndMinutes()
{
  std::array<uint8_t, cMaxWords> currentWords;
  int currentHour = _timeClient->getHours().toInt();
  int currentMinute = _timeClient->getMinutes().toInt();
  int currentSecond = _timeClient->getSeconds().toInt();

  if (currentHour > 12)
    currentHour = currentHour - 12;

  // if there are no minute LEDs, advance the clock by three minutes (2.5 minutes would be correct)
  // so that the medium value "around" the displayed time matches better. 
  // e.g. 16:57..17:02 are shown as "five o'clock"
#ifndef HAS_MINUTES
  currentMinute = (currentMinute + 3) % 60;
#endif

  int j = 0;
  currentWords.fill(0);

  currentWords[j++] = _O_ES_;
  currentWords[j++] = _O_IST_;

  switch (currentMinute)
  {
  case 0 ... 4:
    currentWords[j++] = _O_UHR_;
    break;
  case 5 ... 9:
    currentWords[j++] = _M_FUENF_;
    currentWords[j++] = _O_NACH_;
    break;
  case 10 ... 14:
    currentWords[j++] = _M_ZEHN_;
    currentWords[j++] = _O_NACH_;
    break;
  case 15 ... 19:
#ifdef NORTHERN_GERMAN
    currentWords[j++] = _M_VIERTEL_;
    currentWords[j++] = _O_NACH_;
#else
    currentWords[j++] = _M_VIERTEL_;
#endif
    break;
  case 20 ... 24:
    currentWords[j++] = _M_ZWANZIG_;
    currentWords[j++] = _O_NACH_;
    break;
  case 25 ... 29:
    currentWords[j++] = _M_FUENF_;
    currentWords[j++] = _O_VOR_;
    currentWords[j++] = _M_HALB_;
    currentHour += 1;
    break;
  case 30 ... 34:
    currentWords[j++] = _M_HALB_;
    currentHour += 1;
    break;
  case 35 ... 39:
    currentWords[j++] = _M_FUENF_;
    currentWords[j++] = _O_NACH_;
    currentWords[j++] = _M_HALB_;
    currentHour += 1;
    break;
  case 40 ... 44:
    currentWords[j++] = _M_ZWANZIG_;
    currentWords[j++] = _O_VOR_;
    currentHour += 1;
    break;
  case 45 ... 49:
#ifdef NORTHERN_GERMAN
    currentWords[j++] = _M_VIERTEL_;
    currentWords[j++] = _O_VOR_;
#else
    currentWords[j++] = _M_DREIVIERTEL_;
#endif
    currentHour += 1;
    break;
  case 50 ... 54:
    currentWords[j++] = _M_ZEHN_;
    currentWords[j++] = _O_VOR_;
    currentHour += 1;
    break;
  case 55 ... 59:
    currentWords[j++] = _M_FUENF_;
    currentWords[j++] = _O_VOR_;
    currentHour += 1;
    break;
  }

  switch (currentHour)
  {
  case 1:
    // "Es ist ein Uhr", aber "es ist fünf nach eins".
    if ((currentMinute >= 0 && currentMinute < 2) || (currentMinute >= 57 && currentMinute <= 59))
    {
      currentWords[j++] = _H_EIN_;
    }
    else
    {
      currentWords[j++] = _H_EINS_;
    }
    break;
  case 2:
    currentWords[j++] = _H_ZWEI_;
    break;
  case 3:
    currentWords[j++] = _H_DREI_;
    break;
  case 4:
    currentWords[j++] = _H_VIER_;
    break;
  case 5:
    currentWords[j++] = _H_FUENF_;
    break;
  case 6:
    currentWords[j++] = _H_SECHS_;
    break;
  case 7:
    currentWords[j++] = _H_SIEBEN_;
    break;
  case 8:
    currentWords[j++] = _H_ACHT_;
    break;
  case 9:
    currentWords[j++] = _H_NEUN_;
    break;
  case 10:
    currentWords[j++] = _H_ZEHN_;
    break;
  case 11:
    currentWords[j++] = _H_ELF_;
    break;
  case 12:
    currentWords[j++] = _H_ZWOELF_;
    break;
  default:
    currentWords[j++] = _H_ZWOELF_;
    break;
  }

  if (_lastWords != currentWords)
  {
    Serial.printf("%s Sending %d words:", _timeClient->getFormattedTime().c_str(), j);
    clearAll();
    for (int i = 0; i < j; i++)
    {
      Serial.printf(" %d", currentWords[i]);
      sendWord(currentWords[i]);
    }
    Serial.printf("\r\n");
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
    _matrixLEDs[XY(TLEDS[index].x + j, TLEDS[index].y)] = actcolor;
  }
}
