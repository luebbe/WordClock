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

#include "secrets.h"

// #define DEBUG

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
  Serial.println("Sending State");
  mqttClient.publish(cBrightnessTopic, 1, true, String(FastLED.getBrightness()).c_str());
  mqttClient.publish(cModeTopic, 1, true, String(displayMode).c_str());
}

void sendStats()
{
  Serial.println("Sending Statistics");

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
  Serial.println("Set Mode");
  uint8_t i = atoi(value.c_str());
  if ((0 <= i) && (i <= 255) && (i != displayMode))
  {
    displayMode = i;
    modeChanged = true;
    FastLED.clear(true);
    Serial.printf("=%d\r\n", displayMode);
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
  Serial.println("Connecting to MQTT.");
  connectingAnimation.setHue(192);
  ledEffect = &connectingAnimation;
  mqttClient.connect();
}

void onMqttConnect(bool sessionPresent)
{
  Serial.println("Connected to MQTT.");
  _uptimeMqtt.reset();

  ledEffect = &wordClock;
  modeChanged = true;

  mqttClient.subscribe(cBrightnessSetTopic, 1);
  mqttClient.subscribe(cModeSetTopic, 1);

  mqttClient.publish(cIpTopic, 1, true, WiFi.localIP().toString().c_str());
  mqttClient.publish(cMacTopic, 1, true, WiFi.macAddress().c_str());
  mqttClient.publish(cStateTopic, 1, true, "ready");

  sendState();
}

void onMqttDisconnect(AsyncMqttClientDisconnectReason reason)
{
  Serial.println("Disconnected from MQTT.");

  if (WiFi.isConnected())
  {
    connectingAnimation.setHue(192);
    ledEffect = &connectingAnimation;
    mqttReconnectTimer.once(2, connectToMqtt);
  }
}

void onMqttMessage(char *topic, char *payload, AsyncMqttClientMessageProperties properties, size_t len, size_t index, size_t total)
{
  Serial.printf("topic: %s: ", topic);

  // THIS IS ONLY FOR SHORT PAYLOADS!!!
  // payload is in fact byte*, NOT char*!!!
  if (index == 0)
  {
    std::string value;
    value.assign((const char *)payload, len);
    Serial.printf("%s\r\n", value.c_str());

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
  Serial.println("Connecting to Wi-Fi.");
  connectingAnimation.setHue(160);
  ledEffect = &connectingAnimation;
  WiFi.begin(WIFI_SSID, WIFI_PASS);
}

void onWifiConnect(const WiFiEventStationModeGotIP &event)
{
  Serial.println("Connected to Wi-Fi.");
  _uptimeWifi.reset();

  connectToMqtt();

  // initialize NTP Client after WiFi is connected
  timeClient.begin();
}

void onWifiDisconnect(const WiFiEventStationModeDisconnected &event)
{
  Serial.println("Disconnected from Wi-Fi.");
  connectingAnimation.setHue(160);
  ledEffect = &connectingAnimation;
  mqttReconnectTimer.detach(); // ensure we don't reconnect to MQTT while reconnecting to Wi-Fi
  wifiReconnectTimer.once(2, connectToWifi);
}

void setup()
{
  Serial.begin(SERIAL_SPEED);
  Serial.println();
  Serial.println();

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