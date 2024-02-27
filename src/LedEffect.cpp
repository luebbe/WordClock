/*
 * base class to display animations on a LED strip
 *
 * Author: Lübbe Onken (http://github.com/luebbe)
 */

#include "LedEffect.h"

LedEffect::LedEffect(CRGB *leds, uint16_t count)
    : _leds(leds), _numLeds(count), _currentPalette(RainbowColors_p)
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

void LedEffect::createRandomPalette()
{
  _randomPalette = CRGBPalette16(
      CHSV(random8(), 255, 255),
      CHSV(random8(), 255, 255),
      CHSV(random8(), 128, 255), // One color with less saturation
      CHSV(random8(), 255, 255));
}

CRGB LedEffect::getColorFromPalette(uint8_t index)
{
  return ColorFromPalette(_currentPalette, index);
}

CRGB LedEffect::getRandomColor()
{
  uint8_t index = random(0, 255);
  return ColorFromPalette(_currentPalette, index);
}
