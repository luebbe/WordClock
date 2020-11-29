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

#include <FastLED.h>
#include "LedMatrix.h"

class RainbowAnimation : public LedMatrix
{
private:
  void DrawOneFrame(byte startHue8, int8_t yHueDelta8, int8_t xHueDelta8);

public:
  explicit RainbowAnimation(CRGB *leds, uint8_t width, uint8_t height, bool sepentineLayout = true, bool vertical = false);

  virtual void loop() override;
  virtual void setup() override;
};