/*
 * Mood light (single color) using the LedMatrix base class.
 *
 * Version: 1.0
 * Author: Lübbe Onken (http://github.com/luebbe)
 */

#include "MoodLight.h"

MoodLight::MoodLight(const ILedMatrix *ledMatrix, CRGB *_leds, uint16_t count)
    : LedEffect(_leds, count), _ledMatrix(ledMatrix)
{
}

bool MoodLight::paint(bool force)
{
  for (int i = 0; i < _numLeds; i++)
  {
    _leds[i] = CRGB::Black;
  }
  return true;
}