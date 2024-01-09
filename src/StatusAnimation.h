/*
 * Status overlay using the LedMatrix base class.
 * 
 * Version: 1.0
 * Author: Lübbe Onken (http://github.com/luebbe)
 */

#pragma once

#include "LedEffect.h"
#include "LedMatrix.h"

#define MINUTE_LEDS 4 // One LED in each corner for minutes 1..4

enum CLOCK_STATUS
{
    SETUP,
    WIFI_DISCONNECTED,
    WIFI_CONNECTED,
    MQTT_DISCONNECTED,
    MQTT_CONNECTED
};

class StatusAnimation : public LedEffect
{
private:
    const ILedMatrix *_ledMatrix;
    CRGB *_minuteLEDs; // Pointer to the start of the buffer for the minute LEDs
    CLOCK_STATUS _status = CLOCK_STATUS::SETUP;
    unsigned long _lastUpdate;

public:
    explicit StatusAnimation(const ILedMatrix *ledMatrix, CRGB *leds, uint16_t count);

    void init(){};
    bool paint(bool force) override;
    void setStatus(CLOCK_STATUS status) { _status = status; };
};
