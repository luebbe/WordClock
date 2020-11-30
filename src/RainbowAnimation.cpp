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

RainbowAnimation::RainbowAnimation(CRGB *leds, uint8_t width, uint8_t height, bool serpentineLayout, bool vertical)
    : LedMatrix(leds, width, height, serpentineLayout, vertical)
{
}

void RainbowAnimation::loop()
{
  LedMatrix::loop();

  uint32_t ms = millis();
  int32_t yHueDelta32 = ((int32_t)cos16(ms * (27 / 1)) * (350 / _width));
  int32_t xHueDelta32 = ((int32_t)cos16(ms * (39 / 1)) * (310 / _height));
  DrawOneFrame(ms / 65536, yHueDelta32 / 32768, xHueDelta32 / 32768);
  if (ms < 5000)
  {
    FastLED.setBrightness(scale8(_brightness, (ms * 256) / 5000));
  }
  else
  {
    FastLED.setBrightness(_brightness);
  }
  FastLED.show();
}

void RainbowAnimation::DrawOneFrame(byte startHue8, int8_t yHueDelta8, int8_t xHueDelta8)
{
  byte lineStartHue = startHue8;
  for (byte y = 0; y < _height; y++)
  {
    lineStartHue += yHueDelta8;
    byte pixelHue = lineStartHue;
    for (byte x = 0; x < _width; x++)
    {
      pixelHue += xHueDelta8;
      _matrixLEDs[XY(x, y)] = CHSV(pixelHue, 255, 255);
    }
  }
}
