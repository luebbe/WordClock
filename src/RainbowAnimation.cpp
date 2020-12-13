/*
 * Rainbow animation using the LedMatrix base class.
 * 
 * The code which creates the animation is taken from:
 * https://github.com/FastLED/FastLED/blob/master/examples/XYMatrix/XYMatrix.ino
 *
 * Version: 1.0
 * Author: Lübbe Onken (http://github.com/luebbe)
 */

#include "RainbowAnimation.h"

RainbowAnimation::RainbowAnimation(const ILedMatrix *ledMatrix, CRGB * leds, uint16_t count)
    : LedEffect(leds, count), _ledMatrix(ledMatrix)
{
}

bool RainbowAnimation::paint(bool force)
{
  uint32_t ms = millis();
  int32_t yHueDelta32 = ((int32_t)cos16(ms * (27 / 1)) * (350 / _ledMatrix->getWidth()));
  int32_t xHueDelta32 = ((int32_t)cos16(ms * (39 / 1)) * (310 / _ledMatrix->getHeight()));
  DrawOneFrame(ms / 65536, yHueDelta32 / 32768, xHueDelta32 / 32768);
  return true;
}

void RainbowAnimation::DrawOneFrame(byte startHue8, int8_t yHueDelta8, int8_t xHueDelta8)
{
  byte lineStartHue = startHue8;
  for (byte y = 0; y < _ledMatrix->getHeight(); y++)
  {
    lineStartHue += yHueDelta8;
    byte pixelHue = lineStartHue;
    for (byte x = 0; x < _ledMatrix->getWidth(); x++)
    {
      pixelHue += xHueDelta8;
      _leds[_ledMatrix->toStrip(x, y)] = CHSV(pixelHue, 255, 255);
    }
  }
}
