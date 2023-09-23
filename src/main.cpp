#include <FastLED.h>
#include <ESP8266WiFi.h>
#include <Ticker.h>
#include <espMqttClient.h>
#include <BH1750.h>

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
#include "secrets.h"
#include "..\include\Version.h"

#define FW_NAME "Word Clock"
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

// Params for width, height and number of extra LEDs are defined in WordClock.h
#define NUM_LEDS (MATRIX_WIDTH * MATRIX_HEIGHT) + MINUTE_LEDS + SECOND_LEDS
#define FIRST_MINUTE (MATRIX_WIDTH * MATRIX_HEIGHT)

CRGB leds_plus_safety_pixel[NUM_LEDS + 1];    // The first pixel in this array is the safety pixel for "out of bounds" results. Never use this array directly!
CRGB *const leds(leds_plus_safety_pixel + 1); // This is the "off-by-one" array that we actually work with and which is passed to FastLED!

LedMatrix ledMatrix(MATRIX_WIDTH, MATRIX_HEIGHT);
LedEffect *_ledEffect = nullptr;

BH1750 lightMeter;
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
String _currMode = "";
String _prevMode = "";
bool _modeChanged = false;
uint64_t _lastStatsSent = 0;

uint64_t _lastLightLevelCheck = 0;
uint64_t _lastLightSent = 0;
float _lux = NAN;
byte _mtReg = 0;

Uptime _uptime;
Uptime _uptimeMqtt;
Uptime _uptimeWifi;

#ifdef DEBUG
#define cMqttPrefix "wordclockdebug"
#else
#define cMqttPrefix "wordclock"
#endif

// Home Assistant integration
#define cHaDiscoveryTopic "homeassistant"

// Status
#define cIpTopic cMqttPrefix "/$localip"
#define cMacTopic cMqttPrefix "/$mac"
#define cStateTopic "/$state"
#define cFirmwareTopic cMqttPrefix "/$fw"
#define cFirmwareName cFirmwareTopic "/name"
#define cFirmwareVersion cFirmwareTopic "/version"
#define cFirmwareDate cFirmwareTopic "/date"
#define cPlAvailable "ready"
#define cPlNotAvailable "lost"

// Statistics
#define cStatsTopic "/$stats"
#define cSignal "signal"
#define cFreeHeap "freeheap"
#define cUptime "uptime"
#define cUptimeWifi "uptimewifi"
#define cUptimeMqtt "uptimemqtt"

// Operation mode
#define cLightlevel "lightlevel"
#define cBrightness "brightness"
#define cMode "mode"
#define cModeSet cMode "/set"
#define cLight "light"
#define cLightSet cLight "/set"

void sendHAConfig(const char *confType, const char *confName, const char *config)
{
  const uint8_t MAX_MQTT_LENGTH = 255;
  char mqttTopic[MAX_MQTT_LENGTH];
  snprintf(mqttTopic, MAX_MQTT_LENGTH, cHaDiscoveryTopic "/%s/" cMqttPrefix "/%s/config", confType, confName);

  mqttClient.publish(mqttTopic, 1, true, config);
}

void createHASensor(const char *deviceId, const char *device, const char *deviceClass, const char *confName, const char *statTopic, const char *confUnit, const char *confIcon)
{
  String config =
      HaMqttConfigBuilder()
          .add("name", confName)
          .add("uniq_id", String(deviceId) + String("_") + confName)
          .add("dev_cla", deviceClass, false)
          .add("~", cMqttPrefix)
          .add("avty_t", "~" cStateTopic)
          .add("pl_avail", cPlAvailable)
          .add("pl_not_avail", cPlNotAvailable)
          .add("stat_t", statTopic)
          .add("unit_of_meas", confUnit, false)
          .add("ic", confIcon, false)
          .addSource("dev", device)
          .generatePayload();

  sendHAConfig("sensor", confName, config.c_str());
}

void createAutoDiscovery()
{
  const uint8_t MAX_MAC_LENGTH = 6;
  const uint8_t MAC_STRING_LENGTH = (MAX_MAC_LENGTH * 2) + 1;
  const uint8_t SHRT_STRING_LENGTH = MAX_MAC_LENGTH + 1;

  uint8_t mac[MAX_MAC_LENGTH];
  char uniqueId[MAC_STRING_LENGTH];
  char deviceId[MAC_STRING_LENGTH];

  WiFi.macAddress(mac);
  snprintf(uniqueId, MAC_STRING_LENGTH, "%02x%02x%02x%02x%02x%02x", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  snprintf(deviceId, SHRT_STRING_LENGTH, "%02x%02x%02x", mac[3], mac[4], mac[5]);

  String device =
      HaMqttConfigBuilder()
          .add("ids", uniqueId)
          .add("name", FW_NAME)
          .add("sw", FW_VERSION)
          .add("mf", FW_MANUFACTURER)
          .add("mdl", FW_NAME)
          .generatePayload();

  // Stats sensors
  // Free memory
  createHASensor(deviceId, device.c_str(), "data_size", cFreeHeap, "~" cStatsTopic "/" cFreeHeap, "B", "mdi:memory");
  // Wifi signal strength
  createHASensor(deviceId, device.c_str(), "signal_strength", cSignal, "~" cStatsTopic "/" cSignal, "dB", "");
  // Up time
  createHASensor(deviceId, device.c_str(), "duration", cUptime, "~" cStatsTopic "/" cUptime, "s", "");

  // Light level (sensor)
  createHASensor(deviceId, device.c_str(), "illuminance", cLightlevel, "~/" cLightlevel, "lx", "mdi:sun-wireless");

  // Brightness of the matrix (sensor)
  createHASensor(deviceId, device.c_str(), "", cBrightness, "~/" cBrightness, "", "");

  // Display mode
  String config =
      HaMqttConfigBuilder()
          .add("name", "Display mode")
          .add("uniq_id", String(deviceId) + String("_mode"))
          .add("~", cMqttPrefix)
          .add("avty_t", "~" cStateTopic)
          .add("pl_avail", cPlAvailable)
          .add("pl_not_avail", cPlNotAvailable)
          .add("stat_t", "~/" cMode)
          .add("cmd_t", "~/" cModeSet)
          .addSource("options", "[\"Off\",\"Clock\",\"Rainbow\",\"Borealis\",\"Matrix\",\"Snake\"]")
          .add("ic", "mdi:auto-fix")
          .addSource("dev", device)
          .generatePayload();

  sendHAConfig("select", "displaymode", config.c_str());

  // Light On/Off
  config =
      HaMqttConfigBuilder()
          .add("name", "Light")
          .add("uniq_id", String(deviceId) + String("_light"))
          .add("~", cMqttPrefix)
          .add("avty_t", "~" cStateTopic)
          .add("pl_avail", cPlAvailable)
          .add("pl_not_avail", cPlNotAvailable)
          .add("stat_t", "~/" cLight)
          .add("cmd_t", "~/" cLightSet)
          .add("pl_off", "Off")
          .add("pl_on", "On")
          .addSource("dev", device)
          .generatePayload();

  sendHAConfig("light", cLight, config.c_str());
}

void sendBrightness()
{
  DEBUG_PRINTLN(F("Sending State"));

  mqttClient.publish(cMqttPrefix "/" cBrightness, 1, true, String(FastLED.getBrightness()).c_str());
  mqttClient.publish(cMqttPrefix "/" cLightlevel, 1, true, String(_lux).c_str());
}

void sendStats()
{
  DEBUG_PRINTLN(F("Sending Statistics"));

  mqttClient.publish(cMqttPrefix cStatsTopic "/" cSignal, 1, true, String(WiFi.RSSI()).c_str());
  mqttClient.publish(cMqttPrefix cStatsTopic "/" cFreeHeap, 1, true, String(ESP.getFreeHeap()).c_str());

  _uptime.update();
  _uptimeWifi.update();
  _uptimeMqtt.update();
  char statusStr[20 + 1];
  itoa(_uptime.getSeconds(), statusStr, 10);
  mqttClient.publish(cMqttPrefix cStatsTopic "/" cUptime, 1, true, statusStr);
  itoa(_uptimeWifi.getSeconds(), statusStr, 10);
  mqttClient.publish(cMqttPrefix cStatsTopic "/" cUptimeWifi, 1, true, statusStr);
  itoa(_uptimeMqtt.getSeconds(), statusStr, 10);
  mqttClient.publish(cMqttPrefix cStatsTopic "/" cUptimeMqtt, 1, true, statusStr);
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

    mqttClient.publish(cMqttPrefix "/" cLight, 1, true, _currLight.c_str());
    mqttClient.publish(cMqttPrefix "/" cMode, 1, true, _currMode.c_str());
  }
}

void setLight(String light)
{
  DEBUG_PRINTF("Set Light %s\r\n", light.c_str());

  if (light != _currLight)
  {
    if (light == "Off")
      setMode("Off");
    else
      setMode(_prevMode);
  }
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

  mqttClient.subscribe(cMqttPrefix "/" cModeSet, 1);
  mqttClient.subscribe(cMqttPrefix "/" cLightSet, 1);

  mqttClient.publish(cFirmwareName, 1, true, FW_NAME);
  mqttClient.publish(cFirmwareVersion, 1, true, FW_VERSION);
  mqttClient.publish(cFirmwareDate, 1, true, FW_DATE);

  mqttClient.publish(cIpTopic, 1, true, WiFi.localIP().toString().c_str());
  mqttClient.publish(cMacTopic, 1, true, WiFi.macAddress().c_str());
  mqttClient.publish(cMqttPrefix cStateTopic, 1, true, cPlAvailable);

  createAutoDiscovery();

  setMode("Clock");
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

    if (strcmp(topic, cMqttPrefix "/" cModeSet) == 0)
      setMode(strval);
    else if (strcmp(topic, cMqttPrefix "/" cLightSet) == 0)
      setLight(strval);

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
  DEBUG_PRINTLN();
  DEBUG_PRINTLN();

  Wire.begin(PIN_SDA, PIN_SCL);

  lightMeter.begin(BH1750::CONTINUOUS_LOW_RES_MODE); // Run in Low-Res mode to allow faster sampling

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
  mqttClient.setWill(cMqttPrefix cStateTopic, 1, true, "lost");

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
  if ((_millis - _lastLightLevelCheck >= CHECK_LIGHT_INTERVAL) || (_lastLightLevelCheck == 0))
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