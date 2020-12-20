#include "OtaHelper.h"

OtaHelper::OtaHelper(const ILedMatrix *ledMatrix, CRGB * leds, uint16_t count)
    : LedEffect(leds, count), _ledMatrix(ledMatrix)
{
}

void OtaHelper::onStart()
{
  FastLED.clear(true);
}

void OtaHelper::onEnd()
{
  uint8_t y = uint8_t(_ledMatrix->getHeight() / 2);
  for (uint8_t x = 0; x < _ledMatrix->getWidth(); x++)
    _leds[_ledMatrix->toStrip(x, y)] = CRGB::LimeGreen;
  FastLED.show();
}

void OtaHelper::onError(ota_error_t error)
{
  if (error == OTA_AUTH_ERROR)
  {
    Serial.println("Auth Failed");
  }
  else if (error == OTA_BEGIN_ERROR)
  {
    Serial.println("Begin Failed");
  }
  else if (error == OTA_CONNECT_ERROR)
  {
    Serial.println("Connect Failed");
  }
  else if (error == OTA_RECEIVE_ERROR)
  {
    Serial.println("Receive Failed");
  }
  else if (error == OTA_END_ERROR)
  {
    Serial.println("End Failed");
  }
  else
  {
    Serial.println("Unknown Error");
  }
}

void OtaHelper::onProgress(unsigned int progress, unsigned int total)
{
  float_t curProgress = float_t(progress * _ledMatrix->getWidth()) / total;
  uint8_t brightness = int(curProgress * 256) % 256;
  uint8_t x = uint8_t(curProgress);
  uint8_t y = uint8_t(_ledMatrix->getHeight() / 2);

  // 100% would lead to overflow, just draw the bar until 99%
  if (x < _ledMatrix->getWidth())
  {
    _leds[_ledMatrix->toStrip(x, y)] = CHSV(220, 100, brightness);
  }
  FastLED.show();
}

void OtaHelper::init()
{
  ArduinoOTA.onStart([this]() {
    onStart();
  });

  ArduinoOTA.onEnd([this]() {
    onEnd();
  });

  ArduinoOTA.onProgress([this](unsigned int progress, unsigned int total) {
    onProgress(progress, total);
  });

  ArduinoOTA.onError([this](ota_error_t error) {
    onError(error);
  });

  ArduinoOTA.begin();
}

bool OtaHelper::paint(bool force)
{
  return false;
}