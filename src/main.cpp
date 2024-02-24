#include <FastLED.h>
#include <ESP8266WiFi.h>
#include <Ticker.h>
#include <espMqttClient.h>
#include <BH1750.h>
#include <MedianFilterLib.h>
#include <MeanFilterLib.h>

#include "OtaHelper.h"
#include "TimeHelper.h"
#include "WordClock.h"
#include "StatusAnimation.h"
#include "SnakeAnimation.h"
#include "RainbowAnimation.h"
#include "ArduinoBorealis.h"
#include "MatrixAnimation.h"
#include "MoodLight.h"

#include "HaMqttConfigBuilder.h"

#include "debugutils.h"
#include "..\include\Secrets.h"
#include "..\include\Version.h"

#define FW_NAME "Word Clock"
#define FW_MODEL FW_NAME
#define FW_MANUFACTURER "Lübbe Onken"
#define FW_VERSION VERSION
#define FW_DATE BUILD_TIMESTAMP

#define PIN_LED D1 // on D1 Mini
#define PIN_SDA D6 // on D1 Mini
#define PIN_SCL D7 // on D1 Mini
#define PIN_ADC A0 // on D1 Mini

#define COLOR_ORDER GRB
#define CHIPSET WS2812

// LED matrix of 11x10 pixels with 0,0 at the bottom left
// The matrix has to be the *first* section of the LED chain, because of the "safety pixel"
#define MATRIX_WIDTH 11
#define MATRIX_HEIGHT 10

// Minimum and initial brightness for LEDs
#define BRIGHTNESS 20

// Observed maximum daylight lux values around noon. Adjust this to the light levels at your clock's location.
// The LEDs will reach maximum brightness at this lux level and above.
#define DAYLIGHT_LUX 2500

#define SEND_STATS_INTERVAL 60000UL // Send stats every 60 seconds
#define SEND_LIGHT_INTERVAL 5000UL  // Send light level every 5 seconds
#define CHECK_LIGHT_INTERVAL 50UL   // Check light level 20 times per second

#define MEDIAN_WND 7 // A median filter window size of seven should be enough to filter out most spikes
#define MEAN_WND 7   // After filtering the spikes we don't need many samples anymore for the average

// Params for width, height and number of extra LEDs are defined in WordClock.h
#define NUM_LEDS (MATRIX_WIDTH * MATRIX_HEIGHT) + MINUTE_LEDS + SECOND_LEDS
#define FIRST_MINUTE (MATRIX_WIDTH * MATRIX_HEIGHT)

CRGB leds_plus_safety_pixel[NUM_LEDS + 1];    // The first pixel in this array is the safety pixel for "out of bounds" results. Never use this array directly!
CRGB *const leds(leds_plus_safety_pixel + 1); // This is the "off-by-one" array that we actually work with and which is passed to FastLED!

LedMatrix ledMatrix(MATRIX_WIDTH, MATRIX_HEIGHT);
LedEffect *_ledEffect = nullptr;

BH1750 lightMeter;

MedianFilter<float> medianFilterLDR(MEDIAN_WND);
MeanFilter<float> meanFilterLDR(MEAN_WND);

OtaHelper otaHelper(&ledMatrix, leds, NUM_LEDS);

WordClock wordClock(&ledMatrix, leds, NUM_LEDS, onGetTime);

MoodLight moodLight(&ledMatrix, leds, NUM_LEDS);
StatusAnimation statusAnimation(&ledMatrix, leds, NUM_LEDS);
SnakeAnimation snakeAnimation(&ledMatrix, leds, NUM_LEDS);
RainbowAnimation rainbowAnimation(&ledMatrix, leds, NUM_LEDS);
BorealisAnimation borealisAnimation(&ledMatrix, leds, NUM_LEDS);
MatrixAnimation matrixAnimation(&ledMatrix, leds, NUM_LEDS);

WiFiEventHandler wifiConnectHandler;
WiFiEventHandler wifiDisconnectHandler;
Ticker wifiReconnectTimer;

espMqttClient mqttClient;
Ticker mqttReconnectTimer;

String _currLight = "";
String _currPalette = "";
String _currMode = "";
String _prevMode = "";
bool _modeChanged = false;
bool _initialized = false;
uint64_t _lastStatsSent = 0;

bool _lightMeterOK = false;
uint64_t _lastLightLevelCheck = 0;
uint64_t _lastLightSent = 0;
float _lux = NAN;
byte _mtReg = 0;

Uptime _uptime;
Uptime _uptimeMqtt;
Uptime _uptimeWifi;

#ifdef DEBUG
#define cBaseTopic "debug"
#else
#define cBaseTopic "wordclock"
#endif

// Status
#define cIpTopic "$localip"
#define cMacTopic "$mac"
#define cFirmwareName "$fw/name"
#define cFirmwareVersion "$fw/version"
#define cFirmwareDate "$fw/date"

// Statistics
#define cStatsTopic "$stats"
#define cSignal "signal"
#define cFreeHeap "freeheap"
#define cUptime "uptime"
#define cUptimeWifi "uptimewifi"
#define cUptimeMqtt "uptimemqtt"

// Operation mode
#define cLightlevel "lightlevel"
#define cBrightness "brightness"
#define cMode "mode"
#define cModeOptions "[\"Off\",\"Clock\",\"Rainbow\",\"Borealis\",\"Matrix\",\"Snake\"]"
#define cLight "light"
#define cPalette "palette"
#define cPaletteOptions "[\"Rainbow\",\"Lava\",\"Cloud\",\"Ocean\",\"Forest\",\"Party\",\"Heat\"]"
#define cThreeQuarters "threequarters"

void sendHAConfig(const char *topic, const char *payload)
{
  DEBUG_PRINTLN(topic);
  DEBUG_PRINTLN(payload);

  mqttClient.publish(topic, 1, true, payload);
}

void createAutoDiscovery()
{
  const uint8_t MAX_MAC_LENGTH = 6;
  const uint8_t MAC_STRING_LENGTH = (MAX_MAC_LENGTH * 2) + 1;

  uint8_t mac[MAX_MAC_LENGTH];
  char uniqueId[MAC_STRING_LENGTH];

  WiFi.macAddress(mac);
  snprintf(uniqueId, MAC_STRING_LENGTH, "%02x%02x%02x%02x%02x%02x", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

  DeviceConfigBuilder *haConfig;

  haConfig = new DeviceConfigBuilder(uniqueId, FW_NAME, FW_VERSION, FW_MANUFACTURER, FW_MODEL);
  haConfig->setSendCallback(sendHAConfig)
      .setDeviceTopic(cBaseTopic);

  // Stats sensors
  // Free memory
  haConfig->createSensor("Free heap", cFreeHeap, cStatsTopic "/" cFreeHeap, "mdi:memory", "B", "data_size");

  // Wifi signal strength
  haConfig->createSensor("Signal strength", cSignal, cStatsTopic "/" cSignal, "mdi:wifi", "dB", "signal_strength");

  // Up time
  haConfig->createSensor("Uptime", cUptime, cStatsTopic "/" cUptime, "mdi:clock-outline", "s", "duration");

  // Light level (sensor)
  haConfig->createSensor("Light level", cLightlevel, cLightlevel, "mdi:sun-wireless", "lx", "illuminance");

  // Brightness of the matrix (sensor)
  haConfig->createSensor("Brightness level", cBrightness, cBrightness, "mdi:brightness-auto", "", "");

  // Display On/Off
  haConfig->createLight("Matrix", cLight, cLight, "");

  // Display mode
  haConfig->createSelect("Display mode", cMode, cMode, "mdi:auto-fix", cModeOptions);

  // Color palette for word clock
  haConfig->createSelect("Color palette", cPalette, cPalette, "mdi:palette", cPaletteOptions);

  // Swabian/Northen German time
  haConfig->createSwitch("Swabian time", cThreeQuarters, cThreeQuarters, "");

  delete (haConfig);
}

void sendBrightness()
{
  DEBUG_PRINTLN(F("Sending State"));

  mqttClient.publish(cBaseTopic "/" cBrightness, 1, true, String(FastLED.getBrightness()).c_str());
  mqttClient.publish(cBaseTopic "/" cLightlevel, 1, true, String(_lux).c_str());
}

void sendStats()
{
  DEBUG_PRINTLN(F("Sending Statistics"));

  mqttClient.publish(cBaseTopic "/" cStatsTopic "/" cSignal, 1, true, String(WiFi.RSSI()).c_str());
  mqttClient.publish(cBaseTopic "/" cStatsTopic "/" cFreeHeap, 1, true, String(ESP.getFreeHeap()).c_str());

  _uptime.update();
  _uptimeWifi.update();
  _uptimeMqtt.update();
  char statusStr[20 + 1];
  itoa(_uptime.getSeconds(), statusStr, 10);
  mqttClient.publish(cBaseTopic "/" cStatsTopic "/" cUptime, 1, true, statusStr);
  itoa(_uptimeWifi.getSeconds(), statusStr, 10);
  mqttClient.publish(cBaseTopic "/" cStatsTopic "/" cUptimeWifi, 1, true, statusStr);
  itoa(_uptimeMqtt.getSeconds(), statusStr, 10);
  mqttClient.publish(cBaseTopic "/" cStatsTopic "/" cUptimeMqtt, 1, true, statusStr);
}

void setMode(String mode)
{
  DEBUG_PRINTF("Set Mode l:%s p:%s c:%s->%s\r\n", _currLight.c_str(), _prevMode.c_str(), _currMode.c_str(), mode.c_str());

  if (mode != _currMode)
  {
    FastLED.clear(true);

    if (mode == "Off")
    {
      _prevMode = _currMode;
      _ledEffect = &moodLight;
    }
    else if (mode == "Clock")
      _ledEffect = &wordClock;
    else if (mode == "Rainbow")
      _ledEffect = &rainbowAnimation;
    else if (mode == "Borealis")
      _ledEffect = &borealisAnimation;
    else if (mode == "Matrix")
      _ledEffect = &matrixAnimation;
    else if (mode == "Snake")
      _ledEffect = &snakeAnimation;
    else
    {
      _ledEffect = &wordClock;
      mode = "Clock";
    }
    _currLight = (mode == "Off") ? "Off" : "On";
    _currMode = mode;
    _modeChanged = true;

    mqttClient.publish(cBaseTopic "/" cLight, 1, true, _currLight.c_str());
    mqttClient.publish(cBaseTopic "/" cMode, 1, true, _currMode.c_str());
  }
}

void setPalette(String palette)
{
  // Color palette is up to now only used for the word clock
  DEBUG_PRINTF("Palette:%s->%s\r\n", _currPalette.c_str(), palette.c_str());

  if (palette != _currPalette)
  {
    if (palette == "Rainbow")
      _ledEffect->setPalette(RainbowColors_p);
    else if (palette == "Lava")
      _ledEffect->setPalette(LavaColors_p);
    else if (palette == "Cloud")
      _ledEffect->setPalette(CloudColors_p);
    else if (palette == "Ocean")
      _ledEffect->setPalette(OceanColors_p);
    else if (palette == "Forest")
      _ledEffect->setPalette(ForestColors_p);
    else if (palette == "Party")
      _ledEffect->setPalette(PartyColors_p);
    else if (palette == "Heat")
      _ledEffect->setPalette(HeatColors_p);
    else
      _ledEffect->setPalette(RainbowColors_p);

    DEBUG_PRINTF("Palette:%s->%s\r\n", _currPalette.c_str(), palette.c_str());

    _currPalette = palette;

    mqttClient.publish(cBaseTopic "/" cPalette, 1, true, _currPalette.c_str());
  }
}

void setLight(String cmd)
{
  DEBUG_PRINTF("Set Light %s\r\n", cmd.c_str());

  if (cmd != _currLight)
  {
    if (cmd == "Off")
      setMode("Off");
    else
      setMode(_prevMode);
  }
}

void setThreeQuarters(bool on)
{
  wordClock.setUseThreeQuarters(on);
  mqttClient.publish(cBaseTopic "/" cThreeQuarters, 1, true, on ? "On" : "Off");
}

// MQTT connection and event handling

void connectToMqtt()
{
  DEBUG_PRINTLN(F("Connecting to MQTT."));

  statusAnimation.setStatus(CLOCK_STATUS::MQTT_DISCONNECTED);
  mqttClient.connect();
}

void onMqttConnect(bool sessionPresent)
{
  DEBUG_PRINTLN(F("Connected to MQTT."));

  statusAnimation.setStatus(CLOCK_STATUS::MQTT_CONNECTED);

  _uptimeMqtt.reset();

  mqttClient.subscribe(cBaseTopic "/" cMode "/set", 1);
  mqttClient.subscribe(cBaseTopic "/" cLight "/set", 1);
  mqttClient.subscribe(cBaseTopic "/" cPalette "/set", 1);
  mqttClient.subscribe(cBaseTopic "/" cThreeQuarters "/set", 1);

  mqttClient.publish(cBaseTopic "/" cFirmwareName, 1, true, FW_NAME);
  mqttClient.publish(cBaseTopic "/" cFirmwareVersion, 1, true, FW_VERSION);
  mqttClient.publish(cBaseTopic "/" cFirmwareDate, 1, true, FW_DATE);

  mqttClient.publish(cBaseTopic "/" cIpTopic, 1, true, WiFi.localIP().toString().c_str());
  mqttClient.publish(cBaseTopic "/" cMacTopic, 1, true, WiFi.macAddress().c_str());
  mqttClient.publish(cBaseTopic "/" cAvailabilityTopic, 1, true, cPlAvailable);

  createAutoDiscovery();

  // Set palette and mode to force sending their status to MQTT
  // Default values on first connect
  setPalette(_initialized ? _currPalette : "Rainbow");
  setMode(_initialized ? _currMode : "Clock");
  setThreeQuarters(wordClock.getUseThreeQuarters());
  _initialized = true;
}

void onMqttDisconnect(espMqttClientTypes::DisconnectReason reason)
{
  DEBUG_PRINTLN(F("Disconnected from MQTT."));

  statusAnimation.setStatus(CLOCK_STATUS::MQTT_DISCONNECTED);
  if (WiFi.isConnected())
  {
    mqttReconnectTimer.once(2, connectToMqtt);
  }
}

void onMqttMessage(const espMqttClientTypes::MessageProperties &properties, const char *topic, const uint8_t *payload, size_t len, size_t index, size_t total)
{
  // THIS IS ONLY FOR SHORT PAYLOADS!!!
  // payload is in fact byte*, NOT char*!!!
  if (index == 0)
  {
    char *strval = new char[len + 1];
    memcpy(strval, payload, len);
    strval[len] = 0;
    DEBUG_PRINTF("Message %s: %s\r\n", topic, strval);

    if (strcmp(topic, cBaseTopic "/" cLight "/set") == 0)
      setLight(strval);
    else if (strcmp(topic, cBaseTopic "/" cMode "/set") == 0)
      setMode(strval);
    else if (strcmp(topic, cBaseTopic "/" cPalette "/set") == 0)
      setPalette(strval);
    else if (strcmp(topic, cBaseTopic "/" cThreeQuarters "/set") == 0)
      setThreeQuarters((strcmp(strval, "On") == 0) || (strcmp(strval, "on") == 0) || (strcmp(strval, "1") == 0));

    delete[] strval;
  }
}

// WiFi connection and event handling

void connectToWifi()
{
  DEBUG_PRINTLN(F("Connecting to Wi-Fi."));

  statusAnimation.setStatus(CLOCK_STATUS::WIFI_DISCONNECTED);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
}

void onWifiConnect(const WiFiEventStationModeGotIP &event)
{
  DEBUG_PRINTLN(F("Connected to Wi-Fi."));

  statusAnimation.setStatus(CLOCK_STATUS::WIFI_CONNECTED);
  _uptimeWifi.reset();
  connectToMqtt();

  // initialize NTP Client after WiFi is connected
  timeClient.begin();
}

void onWifiDisconnect(const WiFiEventStationModeDisconnected &event)
{
  DEBUG_PRINTLN(F("Disconnected from Wi-Fi."));

  statusAnimation.setStatus(CLOCK_STATUS::WIFI_DISCONNECTED);
  mqttReconnectTimer.detach(); // ensure we don't reconnect to MQTT while reconnecting to Wi-Fi
  wifiReconnectTimer.once(2, connectToWifi);
}

// Automatic brightness adjustment for LEDs via BH1750 light sensor

void adjustBrightness(float lux)
{
  // lux = 1..65535 convert to 1..255
  // Limit to observed maximum value.
  if (lux > DAYLIGHT_LUX)
  {
    lux = DAYLIGHT_LUX;
  }

  // Apply filtering to get rid of spikes
  lux = meanFilterLDR.AddValue(medianFilterLDR.AddValue(lux));

  // scale to 1..255
  uint8_t brightness = round(lux * 255 / DAYLIGHT_LUX);

  // never go below the minimal brightness
  if (brightness < BRIGHTNESS)
  {
    brightness = BRIGHTNESS;
  }
  FastLED.setBrightness(brightness);
}

void setMTreg(uint8_t mtReg)
{
  if (_mtReg != mtReg)
  {
    _mtReg = mtReg;
    lightMeter.setMTreg(mtReg);
    // if (lightMeter.setMTreg(mtReg))
    // {
    //   DEBUG_PRINTF(Setting MTReg to %d\r\n", mtReg);
    // }
    // else
    // {
    //   DEBUG_PRINTF(Error setting MTReg to %d\r\n", mtReg);
    // }
  }
}

void checkLightLevel()
{
  const byte cMtRegBright = 32;
  const byte cMtRegTypical = 69;
  const byte cMtRegDark = 138;

  if (lightMeter.measurementReady(true))
  {
    _lux = lightMeter.readLightLevel();
    // DEBUG_PRINTF("Light: %7.1f lx mtReg=%d\r\n", _lux, _mtReg);

    if (_lux < 0)
    {
      DEBUG_PRINTLN(F("Light: Error condition detected"));
    }
    else if (_lux > 40000.0)
    {
      // reduce measurement time - needed in direct sun light
      setMTreg(cMtRegBright);
      adjustBrightness(_lux);
    }
    else if (_lux > 10.0)
    {
      // typical light environment
      setMTreg(cMtRegTypical);
      adjustBrightness(_lux);
    }
    else // 0 <= _lux <= 10.0
    {
      // very low light environment
      setMTreg(cMtRegDark);
      adjustBrightness(_lux);
    }
  }
}

void setup()
{
  Serial.begin(SERIAL_SPEED);
  DEBUG_PRINTF("\r\n\r\n%s %s\r\n\r\n", FW_NAME, FW_VERSION);

  Wire.begin(PIN_SDA, PIN_SCL);

  _lightMeterOK = lightMeter.begin(BH1750::CONTINUOUS_LOW_RES_MODE); // Run in Low-Res mode to allow faster sampling
  DEBUG_PRINTF("BH1750 sensor %s\r\n", _lightMeterOK ? "found" : "not found");

  FastLED.addLeds<CHIPSET, PIN_LED, COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
  FastLED.setMaxPowerInVoltsAndMilliamps(5, 2000); // FastLED power management set at 5V, 2A
  FastLED.setBrightness(BRIGHTNESS);
  FastLED.setDither(BINARY_DITHER);
  FastLED.clear(true);

  // Initialize random number generator
  setMode("Rainbow");

  otaHelper.init();
  wordClock.init();

  wifiConnectHandler = WiFi.onStationModeGotIP(onWifiConnect);
  wifiDisconnectHandler = WiFi.onStationModeDisconnected(onWifiDisconnect);

  mqttClient.onConnect(onMqttConnect);
  mqttClient.onDisconnect(onMqttDisconnect);
  mqttClient.onMessage(onMqttMessage);
  mqttClient.setServer(MQTT_HOST, MQTT_PORT);
  mqttClient.setWill(cBaseTopic "/" cAvailabilityTopic, 1, true, cPlNotAvailable);

  _uptime.reset();
  connectToWifi();
}

void loop()
{
  bool update = false;

  // Do it in two steps in order to always get the overlay if there is one. A simple '||' will skip the second check if the first evaluates to true
  if ((_ledEffect && _ledEffect->paint(_modeChanged)))
  {
    // Reset "force" repaint flag
    _modeChanged = false;
    update = true;
  }

  if (statusAnimation.paint(_modeChanged))
  {
    update = true;
  }

  if (update)
  {
    FastLED.show();
  }

  uint64_t _millis = millis();

  // Check light level 20 times per second
  if (_lightMeterOK && ((_millis - _lastLightLevelCheck >= CHECK_LIGHT_INTERVAL) || (_lastLightLevelCheck == 0)))
  {
    checkLightLevel();
    _lastLightLevelCheck = _millis;
  }

  if (WiFi.isConnected())
  {
    mqttClient.loop();
    if (mqttClient.connected())
    {
      // Send status every 60 seconds
      if ((_millis - _lastStatsSent >= SEND_STATS_INTERVAL) || (_lastStatsSent == 0))
      {
        sendStats();
        _lastStatsSent = _millis;
      }
      // Send state (brightness/mode) every 5 seconds
      if ((_millis - _lastLightSent >= SEND_LIGHT_INTERVAL) || (_lastLightSent == 0))
      {
        sendBrightness();
        _lastLightSent = _millis;
      }
    }
    ArduinoOTA.handle();
  }
}