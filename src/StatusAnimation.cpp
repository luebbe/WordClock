/*
 * Status overlay using the LedMatrix base class.
 * 
 * Version: 1.0
 * Author: Lübbe Onken (http://github.com/luebbe)
 */

#include "StatusAnimation.h"

StatusAnimation::StatusAnimation(const ILedMatrix *ledMatrix, CRGB *leds, uint16_t count)
    : LedEffect(leds, count),
      _ledMatrix(ledMatrix),
      _status(SETUP),
      _lastUpdate(0)
{
  _minuteLEDs = (_leds + _ledMatrix->getCount()); // Pointer to the start of the buffer for the minute LEDs
}

bool StatusAnimation::paint(bool force)
{
  uint64_t now = millis();
  if ((now - _lastUpdate >= UPDATE_MS) || (_lastUpdate == 0))
  {
    _lastUpdate = now;
    uint8_t index = uint8_t(round(now / 250)) % 4;

    switch (_status)
    {
    case CLOCK_STATUS::SETUP:
    case CLOCK_STATUS::WIFI_DISCONNECTED:
      memset8(_minuteLEDs, 0, sizeof(struct CRGB) * MINUTE_LEDS);
      _minuteLEDs[index] = CHSV(160, 255, 255);
      return true;
    case CLOCK_STATUS::WIFI_CONNECTED:
    case CLOCK_STATUS::MQTT_DISCONNECTED:
      memset8(_minuteLEDs, 0, sizeof(struct CRGB) * MINUTE_LEDS);
      _minuteLEDs[index] = CHSV(192, 255, 255);
      return true;
    case CLOCK_STATUS::MQTT_CONNECTED:
      return false;
    default:
      return false;
    }
  }
  return false;
}
