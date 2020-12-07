/*
 * Word clock using the LedMatrix base class.
 * 
 * Version: 1.0
 * Author: Lübbe Onken (http://github.com/luebbe)
 */

#include "WordClock.h"

WordClock::WordClock(CRGB *leds, TGetTimeFunction onGetTime)
    : LedMatrix(leds, MATRIX_WIDTH, MATRIX_HEIGHT),
      _onGetTime(onGetTime),
      _minuteColor(CRGB(0xFF00FF)), // Initial color for the minute LEDs
      _secondColor(CRGB(0x00FFFF)), // Initial color for the second LEDs
      _useThreeQuarters(false),
      _lastUpdate(0)
{
  _lastWords.clear();
  _minuteLEDs = (_matrixLEDs + MATRIX_WIDTH * MATRIX_HEIGHT); // Pointer to the start of the buffer for the minute LEDs
  _secondLEDs = (_minuteLEDs + MINUTE_LEDS);                  // Pointer to the start of the buffer for the second LEDs
}

void WordClock::setup()
{
  LedMatrix::setup();

  memset8(_minuteLEDs, 0, sizeof(struct CRGB) * MINUTE_LEDS);
  memset8(_secondLEDs, 0, sizeof(struct CRGB) * SECOND_LEDS);
}

void WordClock::loop()
{
  LedMatrix::loop();

  ulong now = millis();
  if ((now - _lastUpdate >= 1000UL) || (_lastUpdate == 0))
  {
    _lastUpdate = now;

    int hour;
    int minute;
    int second;

    if (_onGetTime(hour, minute, second))
    {
      updateHoursAndMinutes(hour, minute, second);
      updateSeconds(second);
      FastLED.show();
    }
  }
}

void WordClock::adjustTime(int &hours, int &minutes, int &seconds)
{
  if (hours > 12)
    hours = hours - 12;

// When there are no minute LEDs, advance the clock by 2.5 minutes.
// so that the current time is "centered" around the displayed time.
// e.g. 16:57:30..17:02:29 are shown as "five o'clock"
#ifndef HAS_MINUTES
  int minuteOffset = (seconds < 30 ? 2 : 3);
  // Check whether the offset pushes us into the next hour
  if (minutes + minuteOffset >= 60)
  {
    hours++;
  }
  minutes = (minutes + minuteOffset) % 60;
#endif
}

void WordClock::createWords(TWORDBUF &currentWords, int &hour, int &minute)
{
  // The order of the word indexes in the array is not the order of the words on the display.
  currentWords.clear();
  currentWords.push_back(_O_ES_);
  currentWords.push_back(_O_IST_);

  switch (minute)
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
      // Use "quarter hh+1" for hh:15
      currentWords.push_back(_M_VIERTEL_);
    }
    else
    {
      // Use "quarter past hh" for hh:15
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
    hour += 1;
    break;
  case 30 ... 34:
    currentWords.push_back(_M_HALB_);
    hour += 1;
    break;
  case 35 ... 39:
    currentWords.push_back(_M_FUENF_);
    currentWords.push_back(_O_NACH_);
    currentWords.push_back(_M_HALB_);
    hour += 1;
    break;
  case 40 ... 44:
    currentWords.push_back(_M_ZWANZIG_);
    currentWords.push_back(_O_VOR_);
    hour += 1;
    break;
  case 45 ... 49:
    if (_useThreeQuarters)
    {
      // Use "three quarters hh+1" for hh:45
      currentWords.push_back(_M_DREIVIERTEL_);
    }
    else
    {
      // Use "quarter to hh+1" for hh:45
      currentWords.push_back(_M_VIERTEL_);
      currentWords.push_back(_O_VOR_);
    }
    hour += 1;
    break;
  case 50 ... 54:
    currentWords.push_back(_M_ZEHN_);
    currentWords.push_back(_O_VOR_);
    hour += 1;
    break;
  case 55 ... 59:
    currentWords.push_back(_M_FUENF_);
    currentWords.push_back(_O_VOR_);
    hour += 1;
    break;
  }

  switch (hour)
  {
  case 1:
    // "Es ist ein Uhr", aber "es ist fünf nach eins".
    if ((minute >= 0 && minute < 2) || (minute >= 57 && minute <= 59))
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

void WordClock::updateHoursAndMinutes(int &hours, int &minutes, int &seconds)
{
  TWORDBUF currentWords;

  // Adjust the time for special cases
  adjustTime(hours, minutes, seconds);
  createWords(currentWords, hours, minutes);

  // Update the LED matrix if the values have changed
  if (_lastWords != currentWords)
  {
#ifdef DEBUG
    Serial.printf("Sending %d words:", currentWords.size());
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
  int minuteIndex = (minutes % 5) - 1;
  if (minuteIndex == -1)
  {
    // Erase buffer in minute 0 of 5 and pick a color for all minute LEDs
    memset8(_minuteLEDs, 0, sizeof(struct CRGB) * MINUTE_LEDS);
    _minuteColor = randomRGB();
  }
  else
  {
    // Set the color for all minutes up to the current minute
    for (int i = 0; i <= minuteIndex; i++)
    {
      _minuteLEDs[i] = _minuteColor;
    }
  }
#endif
}

void WordClock::updateSeconds(int &seconds)
{
#ifdef HAS_SECONDS
  int secondIndex = (seconds * SECOND_LEDS / 60);

  // Erase buffer in second 0 and pick a new color for all LEDs for the next minute
  if (secondIndex == 0)
  {
    memset8(_secondLEDs, 0, sizeof(struct CRGB) * SECOND_LEDS);
    _secondColor = randomRGB();
  }

  // Now add the offset for the "real" LED number zero
  secondIndex = (secondIndex + SECOND_OFFSET) % SECOND_LEDS;

  // This fills the second LEDs with the color
  _secondLEDs[secondIndex] = _secondColor;
#endif
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
