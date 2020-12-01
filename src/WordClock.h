/*
 * Word clock using the LedMatrix base class.
 * 
 * Version: 1.0
 * Author: Lübbe Onken (http://github.com/luebbe)
 */

#pragma once

#include <FastLED.h>
#include "LedMatrix.h"
#include "TimeClient.h"

#define DEBUG

// LED matrix of 11x10 pixels with 0,0 at the bottom left
// The matrix has to be the *first* section of the LED chain, because of the "safety pixel"
#define MATRIX_WIDTH 11
#define MATRIX_HEIGHT 10

// Uncomment the following line if you have LEDs for the minutes *after* the matrix
// #define HAS_MINUTES

#ifdef HAS_MINUTES
#define MINUTE_LEDS 4 // One LED in each corner for minutes 1..4
#else
#define MINUTE_LEDS 0 // No LEDs for the minutes
#endif

// Uncomment the following line if you have LEDs for the seconds *after* the LEDS for the minutes
// #define HAS_SECONDS

#ifdef HAS_SECONDS
#define SECOND_LEDS 60
#define SECOND_LEDS 30
#else
#define SECOND_LEDS 0
#endif

enum TWORDS
{
  // Dummy
  _NULL_,
  // Values for hours
  _H_EIN_,
  _H_EINS_,
  _H_ZWEI_,
  _H_DREI_,
  _H_VIER_,
  _H_FUENF_,
  _H_SECHS_,
  _H_SIEBEN_,
  _H_ACHT_,
  _H_NEUN_,
  _H_ZEHN_,
  _H_ELF_,
  _H_ZWOELF_,
  // Values for minute distance towards the nearest (half) hour
  _M_FUENF_,
  _M_ZEHN_,
  _M_ZWANZIG_,
  _M_VIERTEL_,
  _M_HALB_,
  _M_DREIVIERTEL_,
  // Other words
  _O_VOR_,
  _O_NACH_,
  _O_ES_,
  _O_IST_,
  _O_UHR_,
  _X_DREI_,
  _X_VIER_,
  _X_VIERTEL_,
  // Dummy
  _LAST_
};

struct TWORDINFO
{
  uint8_t x;
  uint8_t y;
  uint8_t len;
};

// Word positions by LED numbers.
// This is for a matrix of 11x10 pixels with 0,0
// at the bottom left.
static const TWORDINFO TLEDS[] = {
    // x, y, length
    {0, 0, 0},  // NULL
    {2, 4, 3},  // _H_EIN
    {2, 4, 4},  // _H_EINS
    {0, 4, 4},  // _H_ZWEI
    {1, 3, 4},  // _H_DREI
    {7, 2, 4},  // _H_VIER
    {7, 3, 4},  // _H_FÜNF
    {1, 0, 5},  // _H_SECHS
    {5, 4, 6},  // _H_SIEBEN
    {1, 1, 4},  // _H_ACHT
    {3, 2, 4},  // _H_NEUN
    {5, 1, 4},  // _H_ZEHN
    {0, 2, 3},  // _H_ELF
    {5, 5, 5},  // _H_ZWÖLF
    {7, 9, 4},  // _M_FÜNF Minuten
    {0, 8, 4},  // _M_ZEHN Minuten
    {4, 8, 7},  // _M_ZWANZIG Minuten
    {4, 7, 7},  // _M_VIERTEL
    {0, 5, 4},  // _M_HALB
    {0, 7, 11}, // _M_DREIVIERTEL
    {6, 6, 3},  // _O_VOR
    {2, 6, 4},  // _O_NACH
    {0, 9, 2},  // _O_ES
    {3, 9, 3},  // _O_IST
    {8, 0, 3},  // _O_UHR
    {0, 7, 4},  // _X_DREI_
    {5, 7, 4},  // _X_VIER_
    {5, 7, 7},  // _X_VIERTEL_
    {0, 0, 0}   // LAST
};

class WordClock : public LedMatrix
{
private:
#define cMaxWords 10 // Should be enough ;)

  TimeClient *_timeClient;

  CRGB *_minuteLEDs; // Pointer to the start of the buffer for the minute LEDs
  CRGB *_secondLEDs; // Pointer to the start of the buffer for the second LEDs

  unsigned long _updateInterval;
  unsigned long _lastUpdate;
  std::array<uint8_t, cMaxWords> _lastWords; // Buffer for the last words that have been sent to the matrix
  bool _useThreeQuarters;                    // Use "quarter to"/"quarter past" or "quarter"/"three quarters" depending on region

  CRGB randomRGB();
  void updateHoursAndMinutes();
  void updateSeconds();

  void sendWord(uint8_t index);

public:
  explicit WordClock(CRGB *leds, TimeClient *timeClient);

  virtual void loop() override;
  virtual void setup() override;

  void setUseThreeQuarters(bool value) { _useThreeQuarters = value; }
};