#include <FastLED.h>
#include <ESP8266WiFi.h>
#include <Ticker.h>
#include <AsyncMqttClient.h>
#include <BH1750.h>

#include "OtaHelper.h"
#include "TimeHelper.h"
#include "WordClock.h"
#include "StatusAnimation.h"
#include "SnakeAnimation.h"
#include "RainbowAnimation.h"
#include "ArduinoBorealis.h"
#include "MatrixAnimation.h"

#include "debugutils.h"
#include "secrets.h"
#include "..\include\Version.h"

// #define DEBUG
#define FW_NAME "Word Clock"
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

StatusAnimation statusAnimation(&ledMatrix, leds, NUM_LEDS);
SnakeAnimation snakeAnimation(&ledMatrix, leds, NUM_LEDS);
WordClock wordClock(&ledMatrix, leds, NUM_LEDS, onGetTime);
RainbowAnimation rainbowAnimation(&ledMatrix, leds, NUM_LEDS);
BorealisAnimation borealisAnimation(&ledMatrix, leds, NUM_LEDS);
MatrixAnimation matrixAnimation(&ledMatrix, leds, NUM_LEDS);

WiFiEventHandler wifiConnectHandler;
WiFiEventHandler wifiDisconnectHandler;
Ticker wifiReconnectTimer;

AsyncMqttClient mqttClient;
Ticker mqttReconnectTimer;

uint8_t _displayMode = 0;
bool _modeChanged = false;
ulong _lastStatsSent = 0;

ulong _lastLightLevelCheck = 0;
ulong _lastLightSent = 0;
float _lux = NAN;
byte _mtReg = 0;

Uptime _uptime;
Uptime _uptimeMqtt;
Uptime _uptimeWifi;

#ifdef DEBUG
#define cMqttPrefix "wordclockdebug/"
#else
#define cMqttPrefix "wordclock/"
#endif

// Status
#define cIpTopic cMqttPrefix "$localip"
#define cMacTopic cMqttPrefix "$mac"
#define cStateTopic cMqttPrefix "$state"
#define cFirmwareTopic cMqttPrefix "$fw/"
#define cFirmwareName cFirmwareTopic "name"
#define cFirmwareVersion cFirmwareTopic "version"
#define cFirmwareDate cFirmwareTopic "date"

// Statistics
#define cStatsTopic cMqttPrefix "$stats/"
#define cSignalTopic cStatsTopic "signal"
#define cHeapTopic cStatsTopic "freeheap"
#define cUptimeTopic cStatsTopic "uptime"
#define cUptimeWifiTopic cStatsTopic "uptimewifi"
#define cUptimeMqttTopic cStatsTopic "uptimemqtt"

// Operation mode
#define cLightlevelTopic cMqttPrefix "lightlevel"
#define cBrightnessTopic cMqttPrefix "brightness"
#define cModeTopic cMqttPrefix "mode"
#define cModeSetTopic cMqttPrefix "mode/set"

void sendState()
{
  DEBUG_PRINTLN("Sending State");

  mqttClient.publish(cBrightnessTopic, 1, true, String(FastLED.getBrightness()).c_str());
  mqttClient.publish(cModeTopic, 1, true, String(_displayMode).c_str());
  mqttClient.publish(cLightlevelTopic, 1, true, String(_lux).c_str());
}

void sendStats()
{
  DEBUG_PRINTLN("Sending Statistics");

  mqttClient.publish(cSignalTopic, 1, true, String(WiFi.RSSI()).c_str());
  mqttClient.publish(cHeapTopic, 1, true, String(ESP.getFreeHeap()).c_str());

  _uptime.update();
  _uptimeWifi.update();
  _uptimeMqtt.update();
  char statusStr[20 + 1];
  itoa(_uptime.getSeconds(), statusStr, 10);
  mqttClient.publish(cUptimeTopic, 1, true, statusStr);
  itoa(_uptimeWifi.getSeconds(), statusStr, 10);
  mqttClient.publish(cUptimeWifiTopic, 1, true, statusStr);
  itoa(_uptimeMqtt.getSeconds(), statusStr, 10);
  mqttClient.publish(cUptimeMqttTopic, 1, true, statusStr);
}

void setMode(uint8_t mode)
{
  DEBUG_PRINTLN("Set Mode");

  if ((mode <= 255) && (mode != _displayMode))
  {
    _displayMode = mode;
    _modeChanged = true;
    FastLED.clear(true);

    DEBUG_PRINTF("=%d\r\n", _displayMode);

    switch (_displayMode)
    {
    case 0:
      _ledEffect = &wordClock;
      break;
    case 1:
      _ledEffect = &rainbowAnimation;
      break;
    case 2:
      _ledEffect = &borealisAnimation;
      break;
    case 3:
      _ledEffect = &matrixAnimation;
      break;
    case 4:
      _ledEffect = &snakeAnimation;
      break;
    default:
      _ledEffect = &wordClock;
    }
    sendState();
  }
}

// MQTT connection and event handling

void connectToMqtt()
{
  DEBUG_PRINTLN("Connecting to MQTT.");

  statusAnimation.setStatus(CLOCK_STATUS::MQTT_DISCONNECTED);
  mqttClient.connect();
}

void onMqttConnect(bool sessionPresent)
{
  DEBUG_PRINTLN("Connected to MQTT.");

  statusAnimation.setStatus(CLOCK_STATUS::MQTT_CONNECTED);
  _uptimeMqtt.reset();

  _ledEffect = &wordClock;
  _modeChanged = true;

  mqttClient.subscribe(cModeSetTopic, 1);

  mqttClient.publish(cFirmwareName, 1, true, FW_NAME);
  mqttClient.publish(cFirmwareVersion, 1, true, FW_VERSION);
  mqttClient.publish(cFirmwareDate, 1, true, FW_DATE);

  mqttClient.publish(cIpTopic, 1, true, WiFi.localIP().toString().c_str());
  mqttClient.publish(cMacTopic, 1, true, WiFi.macAddress().c_str());
  mqttClient.publish(cStateTopic, 1, true, "ready");

  sendState();
}

void onMqttDisconnect(AsyncMqttClientDisconnectReason reason)
{
  DEBUG_PRINTLN("Disconnected from MQTT.");

  statusAnimation.setStatus(CLOCK_STATUS::MQTT_DISCONNECTED);
  if (WiFi.isConnected())
  {
    mqttReconnectTimer.once(2, connectToMqtt);
  }
}

void onMqttMessage(char *topic, char *payload, AsyncMqttClientMessageProperties properties, size_t len, size_t index, size_t total)
{
  DEBUG_PRINTF("topic: %s: ", topic);

  // THIS IS ONLY FOR SHORT PAYLOADS!!!
  // payload is in fact byte*, NOT char*!!!
  if (index == 0)
  {
    std::string value;
    value.assign((const char *)payload, len);

    DEBUG_PRINTF("%s\r\n", value.c_str());

    if (strcmp(topic, cModeSetTopic) == 0)
    {
      setMode(atoi(value.c_str()));
    }
  }
}

// WiFi connection and event handling

void connectToWifi()
{
  DEBUG_PRINTLN("Connecting to Wi-Fi.");

  statusAnimation.setStatus(CLOCK_STATUS::WIFI_DISCONNECTED);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
}

void onWifiConnect(const WiFiEventStationModeGotIP &event)
{
  DEBUG_PRINTLN("Connected to Wi-Fi.");

  statusAnimation.setStatus(CLOCK_STATUS::WIFI_CONNECTED);
  _uptimeWifi.reset();
  connectToMqtt();

  // initialize NTP Client after WiFi is connected
  timeClient.begin();
}

void onWifiDisconnect(const WiFiEventStationModeDisconnected &event)
{
  DEBUG_PRINTLN("Disconnected from Wi-Fi.");

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
    //   // DEBUG_PRINTF("Setting MTReg to %d\r\n", mtReg);
    // }
    // else
    // {
    //   DEBUG_PRINTF("Error setting MTReg to %d\r\n", mtReg);
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
  randomSeed(analogRead(PIN_ADC));
  setMode(random(1, 5));

  otaHelper.init();
  wordClock.init();

  wifiConnectHandler = WiFi.onStationModeGotIP(onWifiConnect);
  wifiDisconnectHandler = WiFi.onStationModeDisconnected(onWifiDisconnect);

  mqttClient.onConnect(onMqttConnect);
  mqttClient.onDisconnect(onMqttDisconnect);
  mqttClient.onMessage(onMqttMessage);
  mqttClient.setServer(MQTT_HOST, MQTT_PORT);
  mqttClient.setWill(cStateTopic, 1, true, "lost");

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

  ulong _millis = millis();

  // Check light level every five seconds
  if ((_millis - _lastLightLevelCheck >= CHECK_LIGHT_INTERVAL) || (_lastLightLevelCheck == 0))
  {
    checkLightLevel();
    _lastLightLevelCheck = _millis;
  }

  if (WiFi.isConnected())
  {
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
        sendState();
        _lastLightSent = _millis;
      }
    }
    ArduinoOTA.handle();
  }
}