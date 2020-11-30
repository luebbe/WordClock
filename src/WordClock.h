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
#define HAS_MINUTES

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

// Northen German convention uses "quarter past x"/"quarter to x+1" for x:15/x:45
// Southern/eastern German convention uses "quarter x+1"/"three quarters x+1" for x:15/x:45
// Uncomment the following line, if you want the southern German version
#define NORTHERN_GERMAN

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
    {2, 4, 3},  // EIN
    {2, 4, 4},  // EINS
    {0, 4, 4},  // ZWEI
    {1, 3, 4},  // DREI
    {7, 2, 4},  // VIER
    {7, 3, 4},  // FÜNF
    {1, 0, 5},  // SECHS
    {5, 4, 6},  // SIEBEN
    {1, 1, 4},  // ACHT
    {3, 2, 4},  // NEUN
    {5, 1, 4},  // ZEHN
    {0, 2, 3},  // ELF
    {5, 5, 5},  // ZWÖLF
    {7, 9, 4},  // FÜNF Minuten
    {0, 8, 4},  // ZEHN Minuten
    {4, 8, 7},  // ZWANZIG Minuten
    {4, 7, 7},  // VIERTEL
    {0, 5, 4},  // HALB
    {0, 7, 11}, // DREIVIERTEL
    {6, 6, 3},  // VOR
    {2, 6, 4},  // NACH
    {0, 9, 2},  // ES
    {3, 9, 3},  // IST
    {8, 0, 3},  // UHR
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

  CRGB randomRGB();
  void updateHoursAndMinutes();
  void updateSeconds();

  void sendWord(uint8_t index);

public:
  explicit WordClock(CRGB *leds, float utcOffset = 0);

  virtual void loop() override;
  virtual void setup() override;
};