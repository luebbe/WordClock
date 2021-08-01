/*
 * Moving LED animation used during connection
 * 
 * Version: 1.0
 * Author: Lübbe Onken (http://github.com/luebbe)
 */

#pragma once

#include "LedEffect.h"
#include "LedMatrix.h"

class SnakeAnimation : public LedEffect
{
private:
#define FACTOR 100

  const ILedMatrix *_ledMatrix;
  uint8_t _hue;
  int _pos16 = 0;   // position of the "fraction-based bar"
  int _delta16 = 1; // how many 16ths of a pixel to move the Fractional Bar

  void drawFractionalBar(int pos16, int width, uint8_t hue);

public:
  explicit SnakeAnimation(const ILedMatrix *ledMatrix, CRGB *leds, uint16_t count);

  void init(){};
  bool paint(bool force) override;
  void setHue(uint8_t hue) { _hue = hue; };
};