#include <FastLED.h>
#include <ESP8266WiFi.h>
#include <Ticker.h>
#include <AsyncMqttClient.h>

#include "OtaHelper.h"
#include "TimeHelper.h"
#include "WordClock.h"
#include "ConnectingAnimation.h"
#include "RainbowAnimation.h"
#include "ArduinoBorealis.h"

#include "debugutils.h"
#include "secrets.h"

// #define DEBUG
#define FW_NAME "Word Clock"
#define FW_VERSION "1.0.0"

#define LED_PIN 5 // D1 on D1 Mini

#define COLOR_ORDER GRB
#define CHIPSET WS2812

// LED matrix of 11x10 pixels with 0,0 at the bottom left
// The matrix has to be the *first* section of the LED chain, because of the "safety pixel"
#define MATRIX_WIDTH 11
#define MATRIX_HEIGHT 10

#define BRIGHTNESS 20

// Params for width, height and number of extra LEDs are defined in WordClock.h
#define NUM_LEDS (MATRIX_WIDTH * MATRIX_HEIGHT) + MINUTE_LEDS + SECOND_LEDS
#define FIRST_MINUTE (MATRIX_WIDTH * MATRIX_HEIGHT)

CRGB leds_plus_safety_pixel[NUM_LEDS + 1];    // The first pixel in this array is the safety pixel for "out of bounds" results. Never use this array directly!
CRGB *const leds(leds_plus_safety_pixel + 1); // This is the "off-by-one" array that we actually work with and which is passed to FastLED!

LedMatrix ledMatrix(MATRIX_WIDTH, MATRIX_HEIGHT);
LedEffect *ledEffect = nullptr;

OtaHelper otaHelper(&ledMatrix, leds, NUM_LEDS);

ConnectingAnimation connectingAnimation(&ledMatrix, leds, NUM_LEDS);
WordClock wordClock(&ledMatrix, leds, NUM_LEDS, onGetTime);
RainbowAnimation rainbowAnimation(&ledMatrix, leds, NUM_LEDS);
BorealisAnimation borealisAnimation(&ledMatrix, leds, NUM_LEDS);

WiFiEventHandler wifiConnectHandler;
WiFiEventHandler wifiDisconnectHandler;
Ticker wifiReconnectTimer;

AsyncMqttClient mqttClient;
Ticker mqttReconnectTimer;

uint8_t displayMode = 0;
bool modeChanged = false;
ulong _lastUpdate = 0;

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
#define cFirmnwareVersion cFirmwareTopic "version"

// Statistics
#define cStatsTopic cMqttPrefix "$stats/"
#define cSignalTopic cStatsTopic "signal"
#define cHeapTopic cStatsTopic "freeheap"
#define cUptimeTopic cStatsTopic "uptime"
#define cUptimeWifiTopic cStatsTopic "uptimewifi"
#define cUptimeMqttTopic cStatsTopic "uptimemqtt"

// Operation mode
#define cBrightnessTopic cMqttPrefix "brightness"
#define cBrightnessSetTopic cMqttPrefix "brightness/set"
#define cModeTopic cMqttPrefix "mode"
#define cModeSetTopic cMqttPrefix "mode/set"

void sendState()
{
  DEBUG_PRINTLN("Sending State");

  mqttClient.publish(cBrightnessTopic, 1, true, String(FastLED.getBrightness()).c_str());
  mqttClient.publish(cModeTopic, 1, true, String(displayMode).c_str());
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

void setBrightness(std::string value)
{
  uint8_t i = atoi(value.c_str());
  FastLED.setBrightness(i);
  sendState();
}

void setMode(std::string value)
{
  DEBUG_PRINTLN("Set Mode");

  uint8_t i = atoi(value.c_str());
  if ((0 <= i) && (i <= 255) && (i != displayMode))
  {
    displayMode = i;
    modeChanged = true;
    FastLED.clear(true);

    DEBUG_PRINTF("=%d\r\n", displayMode);

    switch (displayMode)
    {
    case 0:
      ledEffect = &wordClock;
      break;
    case 1:
      ledEffect = &rainbowAnimation;
      break;
    case 2:
      ledEffect = &borealisAnimation;
      break;
    case 3:
      ledEffect = &connectingAnimation;
      break;
    default:
      ledEffect = &wordClock;
    }
    sendState();
  }
}

// MQTT connection and event handling

void connectToMqtt()
{
  DEBUG_PRINTLN("Connecting to MQTT.");

  connectingAnimation.setHue(192);
  ledEffect = &connectingAnimation;
  mqttClient.connect();
}

void onMqttConnect(bool sessionPresent)
{
  DEBUG_PRINTLN("Connected to MQTT.");

  _uptimeMqtt.reset();

  ledEffect = &wordClock;
  modeChanged = true;

  mqttClient.subscribe(cBrightnessSetTopic, 1);
  mqttClient.subscribe(cModeSetTopic, 1);

  mqttClient.publish(cFirmwareName, 1, true, FW_NAME);
  mqttClient.publish(cFirmnwareVersion, 1, true, FW_VERSION);

  mqttClient.publish(cIpTopic, 1, true, WiFi.localIP().toString().c_str());
  mqttClient.publish(cMacTopic, 1, true, WiFi.macAddress().c_str());
  mqttClient.publish(cStateTopic, 1, true, "ready");

  sendState();
}

void onMqttDisconnect(AsyncMqttClientDisconnectReason reason)
{
  DEBUG_PRINTLN("Disconnected from MQTT.");

  if (WiFi.isConnected())
  {
    connectingAnimation.setHue(192);
    ledEffect = &connectingAnimation;
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

    if (strcmp(topic, cBrightnessSetTopic) == 0)
    {
      setBrightness(value);
    }
    else if (strcmp(topic, cModeSetTopic) == 0)
    {
      setMode(value);
    }
  }
}

// WiFi connection and event handling

void connectToWifi()
{
  DEBUG_PRINTLN("Connecting to Wi-Fi.");

  connectingAnimation.setHue(160);
  ledEffect = &connectingAnimation;
  WiFi.begin(WIFI_SSID, WIFI_PASS);
}

void onWifiConnect(const WiFiEventStationModeGotIP &event)
{
  DEBUG_PRINTLN("Connected to Wi-Fi.");

  _uptimeWifi.reset();
  connectToMqtt();

  // initialize NTP Client after WiFi is connected
  timeClient.begin();
}

void onWifiDisconnect(const WiFiEventStationModeDisconnected &event)
{
  DEBUG_PRINTLN("Disconnected from Wi-Fi.");

  connectingAnimation.setHue(160);
  ledEffect = &connectingAnimation;
  mqttReconnectTimer.detach(); // ensure we don't reconnect to MQTT while reconnecting to Wi-Fi
  wifiReconnectTimer.once(2, connectToWifi);
}

void setup()
{
  Serial.begin(SERIAL_SPEED);
  DEBUG_PRINTLN();
  DEBUG_PRINTLN();

  FastLED.addLeds<CHIPSET, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
  FastLED.setMaxPowerInVoltsAndMilliamps(5, 2000); // FastLED power management set at 5V, 2A
  FastLED.setBrightness(BRIGHTNESS);
  FastLED.setDither(BINARY_DITHER);
  FastLED.clear(true);

  ledEffect = &connectingAnimation;

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
  if (ledEffect && ledEffect->paint(modeChanged))
  {
    FastLED.show();
    modeChanged = false;
  }

  if (WiFi.isConnected())
  {
    if (mqttClient.connected())
      if ((millis() - _lastUpdate >= 60000UL) || (_lastUpdate == 0))
      {
        sendStats();
        _lastUpdate = millis();
      }

    ArduinoOTA.handle();
  }
}