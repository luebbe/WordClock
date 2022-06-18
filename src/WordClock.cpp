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
  _lastWords.clear();
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
  if ((now - _lastUpdate >= UPDATE_MS) || (_lastUpdate == 0))
  {
    _lastUpdate = now;

    if (force)
    {
      _lastWords.clear();
    }

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

      updateHours(hours, minutes);
      updateMinutes(minutes);
      updateSeconds(seconds);
      return true;
    }
  }
  return false;
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

void WordClock::updateHours(int &hours, int &minutes)
{
  TWORDBUF currentWords;

  createWords(currentWords, hours, minutes);

  // Update the LED matrix if the values have changed
  if (_lastWords != currentWords)
  {
    DEBUG_PRINTF("%02d:%02d sending %d words:", hours, minutes, currentWords.size());

    memset8(_leds, 0, sizeof(struct CRGB) * _ledMatrix->getCount());
    for (size_t i = 0; i < currentWords.size(); i++)
    {
      sendWord(currentWords[i]);
    }
    _lastWords = currentWords;

    DEBUG_PRINTF("\r\n");
  }
}

void WordClock::updateMinutes(int &minutes)
{
// Always update the minute LEDs if available
#ifdef HAS_MINUTES
  int minuteIndex = (minutes % 5) - 1;
  if (minuteIndex == -1)
  {
    // Erase buffer in minute 0 of 5 and pick a color for all minute LEDs
    memset8(_minuteLEDs, 0, sizeof(struct CRGB) * MINUTE_LEDS);
    _minuteColor = getRandomColor();
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
  static bool getColor = true;
  int secondIndex = (seconds * SECOND_LEDS / 60);

  // Erase buffer in second 0 and pick a new color for all LEDs for the next minute
  if (secondIndex == 0)
  {
    if (getColor)
    {
      memset8(_secondLEDs, 0, sizeof(struct CRGB) * SECOND_LEDS);
      _secondColor = getRandomColor();
      getColor = false; // get a new color only once during the first second
    }
  }
  else
  {
    getColor = true;
  }

  // Now add the offset for the "real" LED number zero
  secondIndex = (secondIndex + SECOND_OFFSET) % SECOND_LEDS;

  // This fills the second LEDs with the color
  _secondLEDs[secondIndex] = _secondColor;
#endif
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
