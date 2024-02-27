/*
 * Word clock using the LedMatrix base class.
 *
 * Version: 1.0
 * Author: Lübbe Onken (http://github.com/luebbe)
 */

#include "WordClock.h"

WordClock::WordClock(const ILedMatrix *ledMatrix, CRGB *leds, uint16_t count, TGetTimeFunction onGetTime)
    : LedEffect(leds, count),
      _ledMatrix(ledMatrix),
      _onGetTime(onGetTime),
      _minuteColor(CRGB(0xFF00FF)), // Initial color for the minute LEDs
      _secondColor(CRGB(0x00FFFF)), // Initial color for the second LEDs
      _useThreeQuarters(false),
      _lastUpdate(0)
{
  _currentWords.clear();
  _minuteLEDs = (_leds + _ledMatrix->getCount()); // Pointer to the start of the buffer for the minute LEDs
  _secondLEDs = (_minuteLEDs + MINUTE_LEDS);      // Pointer to the start of the buffer for the second LEDs
}

void WordClock::init()
{
  memset8(_minuteLEDs, 0, sizeof(struct CRGB) * MINUTE_LEDS);
  memset8(_secondLEDs, 0, sizeof(struct CRGB) * SECOND_LEDS);
}

bool WordClock::paint(bool force)
{
  uint64_t now = millis();
  if (force || (now - _lastUpdate >= UPDATE_MS) || (_lastUpdate == 0))
  {
    _lastUpdate = now;

    int hours;
    int minutes;
    int seconds;

    if (_onGetTime(hours, minutes, seconds))
    {

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

      if (minutes == 0)
      {
        createRandomPalette();
      }

      updateHours(hours, minutes, force);
      updateMinutes(minutes, force);
      updateSeconds(seconds);
      return true;
    }
  }
  return false;
}

void WordClock::createWords(TWORDBUF &newWords, int &hour, int &minute)
{
  // The order of the word indexes in the array is not the order of the words on the display.
  newWords.clear();
  newWords.push_back(_O_ES_);
  newWords.push_back(_O_IST_);

  switch (minute)
  {
  case 0 ... 4:
    // This places "o'clock" before the hour in the output array, but it's only visible in the debug output.
    newWords.push_back(_O_UHR_);
    break;
  case 5 ... 9:
    newWords.push_back(_M_FUENF_);
    newWords.push_back(_O_NACH_);
    break;
  case 10 ... 14:
    newWords.push_back(_M_ZEHN_);
    newWords.push_back(_O_NACH_);
    break;
  case 15 ... 19:
    if (_useThreeQuarters)
    {
      // Use "quarter hh+1" for hh:15
      newWords.push_back(_M_VIERTEL_);
      hour += 1;
    }
    else
    {
      // Use "quarter past hh" for hh:15
      newWords.push_back(_M_VIERTEL_);
      newWords.push_back(_O_NACH_);
    }
    break;
  case 20 ... 24:
    newWords.push_back(_M_ZWANZIG_);
    newWords.push_back(_O_NACH_);
    break;
  case 25 ... 29:
    newWords.push_back(_M_FUENF_);
    newWords.push_back(_O_VOR_);
    newWords.push_back(_M_HALB_);
    hour += 1;
    break;
  case 30 ... 34:
    newWords.push_back(_M_HALB_);
    hour += 1;
    break;
  case 35 ... 39:
    newWords.push_back(_M_FUENF_);
    newWords.push_back(_O_NACH_);
    newWords.push_back(_M_HALB_);
    hour += 1;
    break;
  case 40 ... 44:
    newWords.push_back(_M_ZWANZIG_);
    newWords.push_back(_O_VOR_);
    hour += 1;
    break;
  case 45 ... 49:
    if (_useThreeQuarters)
    {
      // Use "three quarters hh+1" for hh:45
      newWords.push_back(_M_DREIVIERTEL_);
    }
    else
    {
      // Use "quarter to hh+1" for hh:45
      newWords.push_back(_M_VIERTEL_);
      newWords.push_back(_O_VOR_);
    }
    hour += 1;
    break;
  case 50 ... 54:
    newWords.push_back(_M_ZEHN_);
    newWords.push_back(_O_VOR_);
    hour += 1;
    break;
  case 55 ... 59:
    newWords.push_back(_M_FUENF_);
    newWords.push_back(_O_VOR_);
    hour += 1;
    break;
  }

  // Limit to 12H display
  if (hour > 12)
    hour = hour - 12;

  switch (hour)
  {
  case 1:
    // Subtle difference in German:
    // "Es ist ein Uhr", aber "es ist fünf nach eins".
    if (minute >= 0 && minute <= 4)
    {
      newWords.push_back(_H_EIN_);
    }
    else
    {
      newWords.push_back(_H_EINS_);
    }
    break;
  case 2:
    newWords.push_back(_H_ZWEI_);
    break;
  case 3:
    newWords.push_back(_H_DREI_);
    break;
  case 4:
    newWords.push_back(_H_VIER_);
    break;
  case 5:
    newWords.push_back(_H_FUENF_);
    break;
  case 6:
    newWords.push_back(_H_SECHS_);
    break;
  case 7:
    newWords.push_back(_H_SIEBEN_);
    break;
  case 8:
    newWords.push_back(_H_ACHT_);
    break;
  case 9:
    newWords.push_back(_H_NEUN_);
    break;
  case 10:
    newWords.push_back(_H_ZEHN_);
    break;
  case 11:
    newWords.push_back(_H_ELF_);
    break;
  case 12:
    newWords.push_back(_H_ZWOELF_);
    break;
  default:
    newWords.push_back(_H_ZWOELF_);
    break;
  }
}

void WordClock::updateHours(int &hours, int &minutes, bool force)
{
  TWORDBUF newWords;

  if (force)
    _currentWords.clear();

  createWords(newWords, hours, minutes);

  // Update the LED matrix if the values have changed
  if (_currentWords != newWords)
  {
    DEBUG_PRINTF("%02d:%02d sending %d words:", hours, minutes, newWords.size());

    _currentWords = newWords;
    sendWords();

    DEBUG_PRINTF("\r\n");
  }
}

void WordClock::updateMinutes(int &minutes, bool force)
{
// Always update the minute LEDs if available
#ifdef HAS_MINUTES
  int minuteIndex = (minutes % 5) - 1;
  if (force || (minuteIndex == -1))
  {
    // Erase buffer in minute 0 of 5 and pick a color for all minute LEDs
    memset8(_minuteLEDs, 0, sizeof(struct CRGB) * MINUTE_LEDS);
    _minuteColor = getRandomColor();
  }

  // Set the color for all minutes up to the current minute
  for (int i = 0; i <= minuteIndex; i++)
  {
    _minuteLEDs[i] = _minuteColor;
  }
#endif
}

void WordClock::updateSeconds(int &seconds)
{
#ifdef HAS_SECONDS
  static bool resetBuffer = true;
  int secondIndex = (seconds * SECOND_LEDS / 60);

  // Erase buffer in second 0 and pick a new color for all LEDs for the next minute
  if ((secondIndex == 0) && resetBuffer)
  {
    // Clear all second LEDs
    memset8(_secondLEDs, 0, sizeof(struct CRGB) * SECOND_LEDS);
    resetBuffer = false;
    // _secondColor = getRandomColor();  // get a new color only once during the first second
  }
  else
  {
    resetBuffer = true;
  }

  // Fill all LEDs starting from zero. secondIndex is the last LED to be lit.
  for (int i = 0; i <= secondIndex; i++)
  {
    _secondColor = getColorFromPalette(255 * i / 60);

    // Now add the offset for the "real" LED number zero
    int ledIndex = (i + SECOND_OFFSET) % SECOND_LEDS;

    // This fills the second LEDs with the color
    _secondLEDs[ledIndex] = _secondColor;
  }
#endif
}

void WordClock::sendWords()
{
  memset8(_leds, 0, sizeof(struct CRGB) * _ledMatrix->getCount());
  for (size_t i = 0; i < _currentWords.size(); i++)
  {
    sendWord(_currentWords[i]);
  }
}

void WordClock::sendWord(uint8_t index)
{
#ifdef DEBUG
  char buffer[30];
  strcpy_P(buffer, (char *)pgm_read_dword(&(DEBUGTWORDS[index])));
  DEBUG_PRINTF(" %d=%s", index, buffer);
#endif

  CRGB actcolor = getRandomColor();
  for (int j = 0; j < TLEDS[index].len; j++)
  {
    _leds[_ledMatrix->toStrip(TLEDS[index].x + j, TLEDS[index].y)] = actcolor;
  }
}
