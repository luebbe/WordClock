/*
 * Matrix style animation using the LedMatrix base class.
 * 
 * Based on: https://gist.github.com/gnkarn/908383d5b81444362bc2e5421566fdd1
 * 
 * Version: 1.0
 * Author: Lübbe Onken (http://github.com/luebbe)
 */

#include "MatrixAnimation.h"

MatrixAnimation::MatrixAnimation(const ILedMatrix *ledMatrix, CRGB *_leds, uint16_t count)
    : LedEffect(_leds, count), _ledMatrix(ledMatrix)
{
}

bool MatrixAnimation::paint(bool force)
{
  bool result = false;

  // falling speed
  EVERY_N_MILLIS(100)
  {
    // move code downward
    // start with lowest row to allow proper overlapping on each column
    // for (int8_t row = _ledMatrix->getHeight() - 1; row >= 0; row--)
    for (int8_t row = 0; row < _ledMatrix->getHeight(); row++)
    {
      for (int8_t col = 0; col < _ledMatrix->getWidth(); col++)
      {
        if (_leds[_ledMatrix->toStrip(col, row)] == _startColor)
        {
          _leds[_ledMatrix->toStrip(col, row)] = CRGB(27, 130, 39); // create trail
          if (row > 0)
            _leds[_ledMatrix->toStrip(col, row - 1)] = _startColor;
        }
      }
    }

    // fade all _leds
    for (int i = 0; i < _ledMatrix->getCount(); i++)
    {
      if (_leds[i].g != 255)
        _leds[i].nscale8(192); // only fade trail
    }

    // check for empty screen to ensure code spawn
    // bool emptyScreen = true;
    // for (int i = 0; i < _ledMatrix->getCount(); i++)
    // {
    //     if (_leds[i])
    //     {
    //         emptyScreen = false;
    //         break;
    //     }
    // }

    // spawn new falling code
    // if (random8(8) == 0 || emptyScreen) // lower number == more frequent spawns
    // {
    int8_t spawnX = random8(_ledMatrix->getWidth());
    _leds[_ledMatrix->toStrip(spawnX, _ledMatrix->getHeight()-1)] = _startColor;
    // }
    result = true;
  }
  return result;
}
