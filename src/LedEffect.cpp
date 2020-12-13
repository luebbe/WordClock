/*
 * base class to display animations on a LED strip
 * 
 * Author: Lübbe Onken (http://github.com/luebbe)
 */

#include "LedEffect.h"

LedEffect::LedEffect(CRGB * leds, uint16_t count)
    : _leds(leds), _numLeds(count)
{
}

LedEffect::~LedEffect()
{
}

void LedEffect::init()
{
  // zero out the led data managed by this effect
  memset8((void *)_leds, 0, sizeof(struct CRGB) * _numLeds);
}

CRGB LedEffect::getRandomColor() const
{
  return CHSV(random(0, 255), 255, 255);
}
