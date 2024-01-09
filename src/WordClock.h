/*
 * Word clock using the LedMatrix base class.
 *
 * Version: 1.0
 * Author: Lübbe Onken (http://github.com/luebbe)
 */

#pragma once

#include "LedEffect.h"
#include "LedMatrix.h"
#include "debugutils.h"

// #define DEBUG

// The data flow through this word clock is the following:
// INPUT -> Matrix LEDs -> Minute LEDs -> Second LEDs (-> Other LEDs).

// Uncomment the following line if you have LEDs for the minutes *after* the matrix
#define HAS_MINUTES

#ifdef HAS_MINUTES
#define MINUTE_LEDS 4 // One LED in each corner for minutes 1..4
#else
#define MINUTE_LEDS 0 // No LEDs for the minutes
#endif

// Uncomment the following line if you have LEDs for the seconds *after* the LEDS for the minutes
#define HAS_SECONDS

#ifdef HAS_SECONDS
// The number of LEDs that you have to display the seconds. The steps are calculated accordingly e.g.:
// 60 -> one step every second
// 30 -> one step every 2 seconds
// 15 -> one step every 4 seconds
#define SECOND_LEDS 60
// The modulo offset for the LED which stands for second zero.
// This depends on where your LED ring starts (in my case it's the bottom center of the clock)
#define SECOND_OFFSET 30
#else
// No LEDs for the seconds
#define SECOND_LEDS 0
#define SECOND_OFFSET 0
#endif

#define UPDATE_MS 50 // Update the display 20 times per second in order to follow the brightness changes quicker

typedef std::function<bool(int &hours, int &minutes, int &seconds)> TGetTimeFunction;

class WordClock : public LedEffect
{
private:
  typedef std::vector<uint8_t> TWORDBUF;

  const ILedMatrix *_ledMatrix;

  TGetTimeFunction _onGetTime;
  CRGB _minuteColor;      // Color for the minute LEDs
  CRGB _secondColor;      // Color for the second LEDs
  bool _useThreeQuarters; // Use "quarter to"/"quarter past" or "quarter"/"three quarters" depending on region
  unsigned long _lastUpdate;

  CRGB *_minuteLEDs; // Pointer to the start of the buffer for the minute LEDs
  CRGB *_secondLEDs; // Pointer to the start of the buffer for the second LEDs

  TWORDBUF _currentWords; // Buffer for the last words that have been sent to the matrix

  void createWords(TWORDBUF &currentWords, int &currentHour, int &currentMinute);
  void sendWords();
  void sendWord(uint8_t index);

  void updateHours(int &hours, int &minutes, bool force);
  void updateMinutes(int &minutes, bool force);
  void updateSeconds(int &seconds);

public:
  explicit WordClock(const ILedMatrix *ledMatrix, CRGB *leds, uint16_t count, TGetTimeFunction onGetTime);

  void init() override;
  bool paint(bool force) override;

  void setUseThreeQuarters(bool value) { _useThreeQuarters = value; }
  bool getUseThreeQuarters() { return _useThreeQuarters; }
};

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

#ifdef DEBUG
// Dummy
const char C_NULL[] PROGMEM = "_null_";
// Hours, capitalized to distinguish from the other numbers
const char C_H_EIN[] PROGMEM = "EIN";
const char C_H_EINS[] PROGMEM = "EINS";
const char C_H_ZWEI[] PROGMEM = "ZWEI";
const char C_H_DREI[] PROGMEM = "DREI";
const char C_H_VIER[] PROGMEM = "VIER";
const char C_H_FUENF[] PROGMEM = "FÜNF";
const char C_H_SECHS[] PROGMEM = "SECHS";
const char C_H_SIEBEN[] PROGMEM = "SIEBEN";
const char C_H_ACHT[] PROGMEM = "ACHT";
const char C_H_NEUN[] PROGMEM = "NEUN";
const char C_H_ZEHN[] PROGMEM = "ZEHN";
const char C_H_ELF[] PROGMEM = "ELF";
const char C_H_ZWOELF[] PROGMEM = "ZWÖLF";
// Minute distance towards the nearest (half) hour
const char C_M_FUENF[] PROGMEM = "fünf";
const char C_M_ZEHN[] PROGMEM = "zehn";
const char C_M_ZWANZIG[] PROGMEM = "zwanzig";
const char C_M_VIERTEL[] PROGMEM = "viertel";
const char C_M_HALB[] PROGMEM = "halb";
const char C_M_DREIVIERTEL[] PROGMEM = "dreiviertel";
// Other words
const char C_O_VOR[] PROGMEM = "vor";
const char C_O_NACH[] PROGMEM = "nach";
const char C_O_ES[] PROGMEM = "es";
const char C_O_IST[] PROGMEM = "ist";
const char C_O_UHR[] PROGMEM = "uhr";
const char C_X_DREI[] PROGMEM = "drei";
const char C_X_VIER[] PROGMEM = "vier";
const char C_X_VIERTEL[] PROGMEM = "viertel";
// Dummy
const char C_LAST[] PROGMEM = "_last_";

const char *const DEBUGTWORDS[] PROGMEM = {
    C_NULL,
    C_H_EIN,
    C_H_EINS,
    C_H_ZWEI,
    C_H_DREI,
    C_H_VIER,
    C_H_FUENF,
    C_H_SECHS,
    C_H_SIEBEN,
    C_H_ACHT,
    C_H_NEUN,
    C_H_ZEHN,
    C_H_ELF,
    C_H_ZWOELF,
    C_M_FUENF,
    C_M_ZEHN,
    C_M_ZWANZIG,
    C_M_VIERTEL,
    C_M_HALB,
    C_M_DREIVIERTEL,
    C_O_VOR,
    C_O_NACH,
    C_O_ES,
    C_O_IST,
    C_O_UHR,
    C_X_DREI,
    C_X_VIER,
    C_X_VIERTEL,
    C_LAST};
#endif

struct TWORDINFO
{
  uint8_t x;
  uint8_t y;
  uint8_t len;
};

// Word positions by LED numbers.
// This is for a matrix of 11x10 pixels
// with 0,0 at the bottom left.
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
