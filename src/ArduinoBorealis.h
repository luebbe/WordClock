/*
 * Simulates the visual effect of aurora borealis using the LedMatrix base class.
 * 
 * The code which creates the animation is taken from:
 * https://github.com/Mazn1191/Arduino-Borealis/blob/main/main.ino
 *
 * Version: 1.0
 * Author: Lübbe Onken (http://github.com/luebbe)
 */

#pragma once

#include "LedMatrix.h"

// LED CONFIG
#define LED_DENSITY 1 //1 = Every LED is used, 2 = Every second LED is used.. and so on

// WAVE CONFIG
#define W_COUNT 8               //Number of simultaneous waves
#define W_SPEED_FACTOR 3        //Higher number, higher speed
#define W_WIDTH_FACTOR 3        //Higher number, smaller waves
#define W_COLOR_WEIGHT_PRESET 1 //What color weighting to choose
#define W_RANDOM_SEED 11        //Change this seed for a different pattern. If you read from an analog input here you can get a different pattern everytime.

class BorealisWave
{
private:
  //List of colors allowed for waves
  //The first dimension of this array must match the second dimension of the colorwighting array
  byte allowedcolors[5][3] = {
      {17, 177, 13},   //Greenish
      {148, 242, 5},   //Greenish
      {25, 173, 121},  //Turquoise
      {250, 77, 127},  //Pink
      {171, 101, 221}, //Purple
  };

  //Colorweighing allows to give some colors more weight so it is more likely to be choosen for a wave.
  //The second dimension of this array must match the first dimension of the allowedcolors array
  //Here are 3 presets.
  byte colorweighting[3][5] = {
      {10, 10, 10, 10, 10}, //Weighting equal (every color is equally likely)
      {2, 2, 2, 6, 6},      //Weighting reddish (red colors are more likely)
      {6, 6, 6, 2, 2}       //Weighting greenish (green colors are more likely)
  };

  uint16_t _numLeds;
  int _ttl;
  byte _basecolor;
  float _basealpha;
  int _age;
  int _width;
  float _center;
  bool _goingleft;
  float _speed;
  bool _alive;

  uint8_t getWeightedColor(uint8_t weighting);

public:
  explicit BorealisWave(uint16_t numLeds);

  CRGB *getColorForLED(int ledIndex);

  //Change position and age of wave
  //Determine if its still "alive"
  void update();
  bool stillAlive() { return _alive; };
};

class BorealisAnimation
{
private:
  CRGB *_leds;
  uint16_t _numLeds;
  BorealisWave *waves[W_COUNT];

  void DrawWaves();

public:
  explicit BorealisAnimation(CRGB *leds, uint16_t numLeds);

  void loop(bool forceUpdate);
};
