#pragma once

#include <ArduinoOTA.h>
#include "LedEffect.h"
#include "LedMatrix.h"

class OtaHelper : public LedEffect
{
private:
  const ILedMatrix *_ledMatrix;

  void onStart();
  void onEnd();
  void onError(ota_error_t error);
  void onProgress(unsigned int progress, unsigned int total);

public:
  explicit OtaHelper(const ILedMatrix *ledMatrix, CRGB *leds, uint16_t count);

  void init() override;
  bool paint(bool force) override;
};
