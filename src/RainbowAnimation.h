/*
 * Rainbow animation using the LedMatrix base class.
 * 
 * The code which creates the animation is taken from:
 * https://github.com/FastLED/FastLED/blob/master/examples/XYMatrix/XYMatrix.ino
 *
 * Version: 1.0
 * Author: Lübbe Onken (http://github.com/luebbe)
 */

#pragma once

#include "LedEffect.h"
#include "LedMatrix.h"

class RainbowAnimation : public LedEffect
{
private:
  const ILedMatrix *_ledMatrix;

  void DrawOneFrame(byte startHue8, int8_t yHueDelta8, int8_t xHueDelta8);

public:
  explicit RainbowAnimation(const ILedMatrix *ledMatrix, CRGB *leds, uint16_t count);

  void init(){};
  bool paint(bool force) override;
};