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

enum TWORDS
{
  _NULL_,
  _EIN_,
  _EINS_,
  _ZWEI_,
  _DREI_,
  _VIER_,
  _FUENF_,
  _SECHS_,
  _SIEBEN_,
  _ACHT_,
  _NEUN_,
  _ZEHN_,
  _ELF_,
  _ZWOELF_,
  _FUENF_M_,
  _ZEHN_M_,
  _ZWANZIG_M_,
  _VIERTEL_M_,
  _HALB_M_,
  _DREIVIERTEL_M_,
  _VOR_,
  _NACH_,
  _ES_,
  _IST_,
  _UHR_,
  _LAST_
};

struct TWORDINFO
{
  uint8_t x;
  uint8_t y;
  uint8_t len;
};

// Word positions by led numbers.
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
  TimeClient *_timeClient;

  unsigned long _updateInterval;
  unsigned long _lastUpdate;
  std::array<uint8_t, 10> _lastWords;

  int randomRGB();
  void updateHoursAndMinutes();
  void updateSeconds();

  void sendWord(uint8_t index);

public:
  explicit WordClock(CRGB *leds, uint8_t width, uint8_t height, bool sepentineLayout = true, bool vertical = false, float utcOffset = 0);

  virtual void loop() override;
  virtual void setup() override;
};