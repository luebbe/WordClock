/*
 * Matrix style animation using the LedMatrix base class.
 * 
 * Version: 1.0
 * Author: Lübbe Onken (http://github.com/luebbe)
 */

#pragma once

#include "LedEffect.h"
#include "LedMatrix.h"

class MatrixAnimation : public LedEffect
{
private:
  const ILedMatrix *_ledMatrix;

  const CRGB _startColor = CRGB(175, 255, 175);
  const CRGB _trailColor = CRGB(27, 130, 39);

public:
  explicit MatrixAnimation(const ILedMatrix *ledMatrix, CRGB *leds, uint16_t count);

  void init(){};
  bool paint(bool force) override;
};