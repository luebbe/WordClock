/*
 * Simulates the visual effect of aurora borealis using the LedMatrix base class.
 * 
 * The code which creates the animation is taken from:
 * https://github.com/Mazn1191/Arduino-Borealis/blob/main/main.ino
 *
 * Version: 1.0
 * Author: Lübbe Onken (http://github.com/luebbe)
 */

#include "ArduinoBorealis.h"

// ---------- BorealisWave ----------

BorealisWave::BorealisWave(uint16_t numLeds) : _numLeds(numLeds)
{
  _ttl = random(500, 1501);
  _basecolor = getWeightedColor(W_COLOR_WEIGHT_PRESET);
  _basealpha = random(50, 101) / (float)100;
  _age = 0;
  _width = random(_numLeds / 10, _numLeds / W_WIDTH_FACTOR);
  _center = random(101) / (float)100 * _numLeds;
  _goingleft = random(0, 2) == 0;
  _speed = random(10, 30) / (float)100 * W_SPEED_FACTOR;
  _alive = true;
}

uint8_t BorealisWave::getWeightedColor(uint8_t weighting)
{
  uint8_t sumOfWeights = 0;

  for (uint8_t i = 0; i < sizeof colorweighting[0]; i++)
  {
    sumOfWeights += colorweighting[weighting][i];
  }

  uint8_t randomweight = random(0, sumOfWeights);

  for (uint8_t i = 0; i < sizeof colorweighting[0]; i++)
  {
    if (randomweight < colorweighting[weighting][i])
    {
      return i;
    }

    randomweight -= colorweighting[weighting][i];
  }
  return 0;
}

CRGB *BorealisWave::getColorForLED(int ledIndex)
{
  if (ledIndex < _center - _width / 2 || ledIndex > _center + _width / 2)
  {
    //Position out of range of this wave
    return NULL;
  }
  else
  {
    CRGB *rgb = new CRGB(allowedcolors[_basecolor]);

    //Offset of this led from center of wave
    //The further away from the center, the dimmer the LED
    int offset = abs(ledIndex - _center);
    float offsetFactor = (float)offset / (_width / 2);

    //The age of the wave determines it brightness.
    //At half its maximum age it will be the brightest.
    float ageFactor = 1;
    if ((float)_age / _ttl < 0.5)
    {
      ageFactor = (float)_age / (_ttl / 2);
    }
    else
    {
      ageFactor = (float)(_ttl - _age) / ((float)_ttl * 0.5);
    }

    // Calculate color based on above factors and basealpha value
    float brightness = (1 - offsetFactor) * ageFactor * _basealpha;
    rgb->r *= brightness;
    rgb->g *= brightness;
    rgb->b *= brightness;

    return rgb;
  }
}

void BorealisWave::update()
{
  if (_goingleft)
  {
    _center -= _speed;
  }
  else
  {
    _center += _speed;
  }

  _age++;

  if (_age > _ttl)
  {
    _alive = false;
  }
  else
  {
    if (_goingleft)
    {
      if (_center + _width / 2 < 0)
      {
        _alive = false;
      }
    }
    else
    {
      if (_center - _width / 2 > _numLeds)
      {
        _alive = false;
      }
    }
  }
}

// ---------- BorealisAnimation ----------

BorealisAnimation::BorealisAnimation(CRGB *leds, uint16_t numLeds)
    : _leds(leds), _numLeds(numLeds)
{
  randomSeed(W_RANDOM_SEED);
  //Initial creating of waves
  for (int i = 0; i < W_COUNT; i++)
  {
    waves[i] = new BorealisWave(_numLeds);
  }
}

void BorealisAnimation::loop(bool forceUpdate)
{
  if (forceUpdate)
  {
    FastLED.setBrightness(200);
  }
  DrawWaves();
  FastLED.show();
}

void BorealisAnimation::DrawWaves()
{
  for (int i = 0; i < W_COUNT; i++)
  {
    // Update values of wave
    waves[i]->update();

    if (!(waves[i]->stillAlive()))
    {
      // If a wave dies, remove it from memory and spawn a new one
      delete waves[i];
      waves[i] = new BorealisWave(_numLeds);
    }
  }

  // Loop through LEDs to determine color
  for (int i = 0; i < _numLeds; i++)
  {
    if (i % LED_DENSITY != 0)
    {
      continue;
    }

    CRGB mixedRgb = CRGB::Black;

    // For each LED we must check each wave if it is "active" at this position.
    // If there are multiple waves active on a LED we multiply their values.
    for (int j = 0; j < W_COUNT; j++)
    {
      CRGB *rgb = waves[j]->getColorForLED(i);

      if (rgb != NULL)
      {
        mixedRgb += *rgb;
      }

      delete[] rgb;
    }
    _leds[i] = mixedRgb;
  }
}
