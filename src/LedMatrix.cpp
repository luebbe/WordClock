/*
 * Base class for accessing a RGB LED matrix.
 * All classes that draw on the matrix inherit from this one.
 * 
 * The code which takes care of the matrix layout and out of range checks is taken from:
 * https://github.com/FastLED/FastLED/blob/master/examples/XYMatrix/XYMatrix.ino
 * and wrapped into a class.
 *
 * Version: 1.0
 * Author: Lübbe Onken (http://github.com/luebbe)
 */

#include "LedMatrix.h"

LedMatrix::LedMatrix(CRGB *leds, uint8_t width, uint8_t height, bool serpentineLayout, bool vertical)
    : _matrixLEDs(leds),
      _width(width), _height(height),
      _serpentineLayout(serpentineLayout),
      _vertical(vertical),
      _brightness(0)
{
}

void LedMatrix::loop()
{
}

void LedMatrix::setup()
{
  FastLED.setBrightness(_brightness);
  clearAll();
}

void LedMatrix::clearAll()
{
  memset(_matrixLEDs, 0, sizeof(struct CRGB) * _width * _height);
  FastLED.show();
}

void LedMatrix::scrollLeft()
{
  for (uint8_t y = 0; y < _height; y++)
  {
    CRGB save = _matrixLEDs[XY(0, y)];
    for (uint8_t x = 0; x < _width - 1; x++)
    {
      _matrixLEDs[XY(x, y)] = _matrixLEDs[XY(x + 1, y)];
    }
    _matrixLEDs[XY(_width - 1, y)] = save;
  }
}

void LedMatrix::scrollRight()
{
  for (uint8_t y = 0; y < _height; y++)
  {
    CRGB save = _matrixLEDs[XY(_width - 1, y)];
    for (uint8_t x = _width - 1; x > 0; x--)
    {
      _matrixLEDs[XY(x, y)] = _matrixLEDs[XY(x - 1, y)];
    }
    _matrixLEDs[XY(0, y)] = save;
  }
}

void LedMatrix::scrollUp()
{
  for (uint8_t x = 0; x < _width; x++)
  {
    CRGB save = _matrixLEDs[XY(x, _height - 1)];
    for (uint8_t y = _height - 1; y > 0; y--)
    {
      _matrixLEDs[XY(x, y)] = _matrixLEDs[XY(x, y - 1)];
    }
    _matrixLEDs[XY(x, 0)] = save;
  }
}

void LedMatrix::scrollDown()
{
  for (uint8_t x = 0; x < _width; x++)
  {
    CRGB save = _matrixLEDs[XY(x, 0)];
    for (uint8_t y = 0; y < _height - 1; y++)
    {
      _matrixLEDs[XY(x, y)] = _matrixLEDs[XY(x, y + 1)];
    }
    _matrixLEDs[XY(x, _height - 1)] = save;
  }
}

uint16_t LedMatrix::XY(uint8_t x, uint8_t y)
{
  uint16_t i;

  if (_serpentineLayout == false)
  {
    if (_vertical == false)
    {
      i = (y * _width) + x;
    }
    else
    {
      i = _height * (_width - (x + 1)) + y;
    }
  }

  if (_serpentineLayout == true)
  {
    if (_vertical == false)
    {
      if (y & 0x01)
      {
        // Odd rows run backwards
        uint8_t reverseX = (_width - 1) - x;
        i = (y * _width) + reverseX;
      }
      else
      {
        // Even rows run forwards
        i = (y * _width) + x;
      }
    }
    else
    { // vertical positioning
      if (x & 0x01)
      {
        i = _height * (_width - (x + 1)) + y;
      }
      else
      {
        i = _height * (_width - x) - (y + 1);
      }
    }
  }

  return i;
}

uint16_t LedMatrix::XYsafe(uint8_t x, uint8_t y)
{
  if (x >= _width)
    return -1;
  if (y >= _height)
    return -1;
  return XY(x, y);
}
