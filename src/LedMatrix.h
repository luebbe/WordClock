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

#pragma once

#include <FastLED.h>

// Set 'serpentineLayout' to false if your pixels are
// laid out all running the same way, like this:
//
//     0 >  1 >  2 >  3 >  4
//                         |
//     .----<----<----<----'
//     |
//     5 >  6 >  7 >  8 >  9
//                         |
//     .----<----<----<----'
//     |
//    10 > 11 > 12 > 13 > 14
//                         |
//     .----<----<----<----'
//     |
//    15 > 16 > 17 > 18 > 19
//
// Set 'serpentineLayout' to true if your pixels are
// laid out back-and-forth, like this:
//
//     0 >  1 >  2 >  3 >  4
//                         |
//                         |
//     9 <  8 <  7 <  6 <  5
//     |
//     |
//    10 > 11 > 12 > 13 > 14
//                        |
//                        |
//    19 < 18 < 17 < 16 < 15
//
// Bonus vocabulary word: anything that goes one way
// in one row, and then backwards in the next row, and so on
// is call "boustrophedon", meaning "as the ox plows."

class LedMatrix
{
protected:
  CRGB *_leds;
  uint8_t _width;
  uint8_t _height;
  bool _serpentineLayout;
  bool _vertical;
  uint8_t _brightness;

  void clearAll();
  uint16_t XY(uint8_t x, uint8_t y);
  uint16_t XYsafe(uint8_t x, uint8_t y);

public:
  explicit LedMatrix(CRGB *leds, uint8_t width, uint8_t height, bool sepentineLayout = true, bool vertical = false);

  virtual void loop();
  virtual void setup();

  void setBrightness(uint8_t value) { _brightness = value; }
};