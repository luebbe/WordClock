#pragma once

#include <ArduinoOTA.h>
#include "LedMatrix.h"

class OtaHelper : public LedMatrix
{
private:
    void onStart();
    void onEnd();
    void onError(ota_error_t error);
    void onProgress(unsigned int progress, unsigned int total);

public:
    explicit OtaHelper(CRGB *leds, uint8_t width, uint8_t height, bool sepentineLayout = true, bool vertical = false);

    virtual void loop(bool forceUpdate) override;
    virtual void setup() override;
};
