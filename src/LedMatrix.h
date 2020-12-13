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

#include "Arduino.h"

class ILedMatrix
{
public:
  virtual uint8_t getWidth() const = 0;
  virtual uint8_t getHeight() const = 0;
  virtual uint16_t getCount() const = 0;
  virtual int16_t toStrip(uint8_t x, uint8_t y) const = 0;
};

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

class LedMatrix : public ILedMatrix
{
private:
  uint8_t _width;
  uint8_t _height;
  bool _serpentineLayout;
  bool _vertical;

  // This function will return the right 'led index number' for
  // a given set of X and Y coordinates on your matrix.
  // IT DOES NOT CHECK THE COORDINATE BOUNDARIES.
  // That's up to you.  Don't pass it bogus values.
  //
  // Use the "XY" function like this:
  //
  //    for( uint8_t x = 0; x < Width; x++) {
  //      for( uint8_t y = 0; y < Height; y++) {
  //
  //        // Here's the x, y to 'led index' in action:
  //        leds[ XY( x, y) ] = CHSV( random8(), 255, 255);
  //
  //      }
  //    }
  //
  //
  int16_t XY(uint8_t x, uint8_t y) const 
  {
    int16_t i;

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
  // Once you've gotten the basics working (AND NOT UNTIL THEN!)
  // here's a helpful technique that can be tricky to set up, but
  // then helps you avoid the needs for sprinkling array-bound-checking
  // throughout your code.
  //
  // It requires a careful attention to get it set up correctly, but
  // can potentially make your code smaller and faster.
  //
  // Suppose you have an 8 x 5 matrix of 40 LEDs.  Normally, you'd
  // delcare your leds array like this:
  //    CRGB leds[40];
  // But instead of that, declare an LED buffer with one extra pixel in
  // it, "leds_plus_safety_pixel".  Then declare "leds" as a pointer to
  // that array, but starting with the 2nd element (id=1) of that array:
  //    CRGB leds_with_safety_pixel[41];
  //    CRGB* const leds( leds_plus_safety_pixel + 1);
  // Then you use the "leds" array as you normally would.
  // Now "leds[0..N]" are aliases for "leds_plus_safety_pixel[1..(N+1)]",
  // AND leds[-1] is now a legitimate and safe alias for leds_plus_safety_pixel[0].
  // leds_plus_safety_pixel[0] aka leds[-1] is now your "safety pixel".
  //
  // Now instead of using the XY function above, use the one below, "XYsafe".
  //
  // If the X and Y values are 'in bounds', this function will return an index
  // into the visible led array, same as "XY" does.
  // HOWEVER -- and this is the trick -- if the X or Y values
  // are out of bounds, this function will return an index of -1.
  // And since leds[-1] is actually just an alias for leds_plus_safety_pixel[0],
  // it's a totally safe and legal place to access.  And since the 'safety pixel'
  // falls 'outside' the visible part of the LED array, anything you write
  // there is hidden from view automatically.
  // Thus, this line of code is totally safe, regardless of the actual size of
  // your matrix:
  //    leds[ XYSafe( random8(), random8() ) ] = CHSV( random8(), 255, 255);
  //
  // The only catch here is that while this makes it safe to read from and
  // write to 'any pixel', there's really only ONE 'safety pixel'.  No matter
  // what out-of-bounds coordinates you write to, you'll really be writing to
  // that one safety pixel.  And if you try to READ from the safety pixel,
  // you'll read whatever was written there last, reglardless of what coordinates
  // were supplied.
  int16_t XYSafe(uint8_t x, uint8_t y) const 
  {
    if (x >= _width)
      return -1;
    if (y >= _height)
      return -1;
    return XY(x, y);
  }

public:
  explicit LedMatrix(uint8_t width, uint8_t height, bool serpentineLayout = true, bool vertical = false)
      : _width(width), _height(height), _serpentineLayout(serpentineLayout), _vertical(vertical) {}

  uint8_t getWidth() const { return _width; }
  uint8_t getHeight() const { return _height; }
  uint16_t getCount() const { return _width * _height; }
  int16_t toStrip(uint8_t x, uint8_t y) const { return XYSafe(x, y); }
};
