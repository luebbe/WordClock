/*
 * Mood light (single color) using the LedMatrix base class.
 * 
 * Version: 1.0
 * Author: Lübbe Onken (http://github.com/luebbe)
 */

#pragma once

#include "LedEffect.h"
#include "LedMatrix.h"

class MoodLight : public LedEffect
{
private:
  const ILedMatrix *_ledMatrix;

public:
  explicit MoodLight(const ILedMatrix *ledMatrix, CRGB *leds, uint16_t count);

  void init(){};
  bool paint(bool force) override;
};