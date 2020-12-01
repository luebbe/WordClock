/*
 * Word clock using the LedMatrix base class.
 * 
 * Version: 1.0
 * Author: Lübbe Onken (http://github.com/luebbe)
 */

#include "WordClock.h"

WordClock::WordClock(CRGB *leds, TimeClient *timeClient)
    : LedMatrix(leds, MATRIX_WIDTH, MATRIX_HEIGHT),
      _timeClient(timeClient),
      _updateInterval(1),
      _lastUpdate(0),
      _useThreeQuarters(false)
{
  _lastWords.clear();

  //##DEBUG REMOVE
  _minuteLEDs = (_matrixLEDs + 7); // last four LEDs in bottom row for debugging
  // _minuteLEDs = (_matrixLEDs + _width * _height);  // Pointer to the start of the buffer for the minute LEDs
  _secondLEDs = (_minuteLEDs + MINUTE_LEDS); // Pointer to the start of the buffer for the second LEDs
}

void WordClock::setup()
{
  LedMatrix::setup();

  //##DEBUG REMOVE
  Serial.printf("%10d %10d %10d\r\n", int(_matrixLEDs), int(_minuteLEDs), int(_secondLEDs));

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

void WordClock::adjustTime(int &hour, int &minute, int &second)
{
  if (hour > 12)
    hour = hour - 12;

// When there are no minute LEDs, advance the clock by 2.5 minutes.
// so that the current time is "centered" around the displayed time.
// e.g. 16:57:30..17:02:29 are shown as "five o'clock"
#ifndef HAS_MINUTES
  int minuteOffset = (_timeClient->getSeconds().toInt() < 30 ? 2 : 3);
  // Check whether the offset pushes us into the next hour
  if (minute + minuteOffset >= 60)
  {
    hour++;
  }
  minute = (minute + minuteOffset) % 60;
#endif
}

void WordClock::createWords(TWORDBUF &currentWords, int &currentHour, int &currentMinute)
{
  // The order of the word indexes in the array is not the order of the words on the display.
  currentWords.clear();
  currentWords.push_back(_O_ES_);
  currentWords.push_back(_O_IST_);

  switch (currentMinute)
  {
  case 0 ... 4:
    // This places "o'clock" before the hour in the output array, but it's only visible in the debug output.
    currentWords.push_back(_O_UHR_);
    break;
  case 5 ... 9:
    currentWords.push_back(_M_FUENF_);
    currentWords.push_back(_O_NACH_);
    break;
  case 10 ... 14:
    currentWords.push_back(_M_ZEHN_);
    currentWords.push_back(_O_NACH_);
    break;
  case 15 ... 19:
    if (_useThreeQuarters)
    {
      // Use "quarter x+1" for hh:15
      currentWords.push_back(_M_VIERTEL_);
    }
    else
    {
      // Use "quarter past x" for hh:15
      currentWords.push_back(_M_VIERTEL_);
      currentWords.push_back(_O_NACH_);
    }
    break;
  case 20 ... 24:
    currentWords.push_back(_M_ZWANZIG_);
    currentWords.push_back(_O_NACH_);
    break;
  case 25 ... 29:
    currentWords.push_back(_M_FUENF_);
    currentWords.push_back(_O_VOR_);
    currentWords.push_back(_M_HALB_);
    currentHour += 1;
    break;
  case 30 ... 34:
    currentWords.push_back(_M_HALB_);
    currentHour += 1;
    break;
  case 35 ... 39:
    currentWords.push_back(_M_FUENF_);
    currentWords.push_back(_O_NACH_);
    currentWords.push_back(_M_HALB_);
    currentHour += 1;
    break;
  case 40 ... 44:
    currentWords.push_back(_M_ZWANZIG_);
    currentWords.push_back(_O_VOR_);
    currentHour += 1;
    break;
  case 45 ... 49:
    if (_useThreeQuarters)
    {
      // Use "three quarters x+1" for hh:45
      currentWords.push_back(_M_DREIVIERTEL_);
    }
    else
    {
      // Use "quarter to x+1" for hh:45
      currentWords.push_back(_M_VIERTEL_);
      currentWords.push_back(_O_VOR_);
    }
    currentHour += 1;
    break;
  case 50 ... 54:
    currentWords.push_back(_M_ZEHN_);
    currentWords.push_back(_O_VOR_);
    currentHour += 1;
    break;
  case 55 ... 59:
    currentWords.push_back(_M_FUENF_);
    currentWords.push_back(_O_VOR_);
    currentHour += 1;
    break;
  }

  switch (currentHour)
  {
  case 1:
    // "Es ist ein Uhr", aber "es ist fünf nach eins".
    if ((currentMinute >= 0 && currentMinute < 2) || (currentMinute >= 57 && currentMinute <= 59))
    {
      currentWords.push_back(_H_EIN_);
    }
    else
    {
      currentWords.push_back(_H_EINS_);
    }
    break;
  case 2:
    currentWords.push_back(_H_ZWEI_);
    break;
  case 3:
    currentWords.push_back(_H_DREI_);
    break;
  case 4:
    currentWords.push_back(_H_VIER_);
    break;
  case 5:
    currentWords.push_back(_H_FUENF_);
    break;
  case 6:
    currentWords.push_back(_H_SECHS_);
    break;
  case 7:
    currentWords.push_back(_H_SIEBEN_);
    break;
  case 8:
    currentWords.push_back(_H_ACHT_);
    break;
  case 9:
    currentWords.push_back(_H_NEUN_);
    break;
  case 10:
    currentWords.push_back(_H_ZEHN_);
    break;
  case 11:
    currentWords.push_back(_H_ELF_);
    break;
  case 12:
    currentWords.push_back(_H_ZWOELF_);
    break;
  default:
    currentWords.push_back(_H_ZWOELF_);
    break;
  }
}

void WordClock::updateHoursAndMinutes()
{
  int currentHour = _timeClient->getHours().toInt();
  int currentMinute = _timeClient->getMinutes().toInt();
  int currentSecond = _timeClient->getSeconds().toInt();
  TWORDBUF currentWords;

  // Adjust the time for special cases
  adjustTime(currentHour, currentMinute, currentSecond);
  createWords(currentWords, currentHour, currentMinute);

  // Update the LED matrix if the values have changed
  if (_lastWords != currentWords)
  {
#ifdef DEBUG
    Serial.printf("%s Sending %d words:", _timeClient->getFormattedTime().c_str(), currentWords.size());
#endif
    clearAll();
    for (size_t i = 0; i < currentWords.size(); i++)
    {
      sendWord(currentWords[i]);
    }
    _lastWords = currentWords;
#ifdef DEBUG
    Serial.printf("\r\n");
#endif
  }

// Always update the minute LEDs if available
#ifdef HAS_MINUTES
  int8_t minutePart = (currentMinute % 5) - 1;
  static CRGB minuteColor = CRGB(0xFF);
  if (minutePart == -1)
  {
    // Erase buffer in minute 0 of 5 and pick a color for all minute LEDs
    memset(_minuteLEDs, 0, sizeof(struct CRGB) * MINUTE_LEDS);
    minuteColor = randomRGB();
  }
  else
  {
    // Set the color for all minutes up to the current minute
    for (int i = 0; i <= minutePart; i++)
    {
      _minuteLEDs[i] = minuteColor;
    }
  }
#endif
}

void WordClock::updateSeconds()
{
  int seconds = _timeClient->getSeconds().toInt();
}

CRGB WordClock::randomRGB()
{
  // If we're unlucky, we'll get an almost invisible colour
  return CRGB(random(1, 0xFFFFFF));
}

void WordClock::sendWord(uint8_t index)
{
#ifdef DEBUG
  char buffer[30];
  strcpy_P(buffer, (char *)pgm_read_dword(&(DEBUGTWORDS[index])));
  Serial.printf(" %d=%s", index, buffer);
#endif

  CRGB actcolor = randomRGB();
  for (int j = 0; j < TLEDS[index].len; j++)
  {
    _matrixLEDs[XY(TLEDS[index].x + j, TLEDS[index].y)] = actcolor;
  }
}
