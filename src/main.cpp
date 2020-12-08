#include <FastLED.h>
#include <ESP8266WiFi.h>
#include <Ticker.h>
#include <AsyncMqttClient.h>

#include "TimeHelper.h"
#include "WordClock.h"
#include "RainbowAnimation.h"

#include "secrets.h"

#define LED_PIN 5 // D1 on D1 Mini

#define COLOR_ORDER GRB
#define CHIPSET WS2812

#define BRIGHTNESS 20

// Params for width, height and number of extra LEDs are defined in WordClock.h
#define NUM_LEDS (MATRIX_WIDTH * MATRIX_HEIGHT) + MINUTE_LEDS + SECOND_LEDS
CRGB leds_plus_safety_pixel[NUM_LEDS + 1];    // The first pixel in this array is the safety pixel for "out of bounds" results. Never use this array directly!
CRGB *const leds(leds_plus_safety_pixel + 1); // This is the "off-by-one" array that we actually work with and which is passed to FastLED!

WordClock wordClock(leds, onGetTime);
RainbowAnimation rainbowAnimation(leds, MATRIX_WIDTH, MATRIX_HEIGHT);

WiFiEventHandler wifiConnectHandler;
WiFiEventHandler wifiDisconnectHandler;
Ticker wifiReconnectTimer;

AsyncMqttClient mqttClient;
Ticker mqttReconnectTimer;

uint8_t clockMode = 0;
bool modeChanged = false;

#define cBrightnessTopic "wordclock/brightness"
#define cBrightnessSetTopic "wordclock/brightness/set"
#define cModeTopic "wordclock/mode"
#define cModeSetTopic "wordclock/mode/set"

void sendState()
{
  Serial.println("Send State");
  mqttClient.publish(cBrightnessTopic, 0, true, String(FastLED.getBrightness()).c_str());
  mqttClient.publish(cModeTopic, 0, true, String(clockMode).c_str());
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
  if ((0 <= i) && (i <= 1))
  {
    clockMode = i;
    modeChanged = true;
    FastLED.clear(true);
    Serial.printf("=%d\r\n", clockMode);
    sendState();
  }
}

void blinkLED(int index)
{
  if (leds[index] != CRGB(0))
    leds[index] = 0;
  else
    leds[index] = CRGB(0xFFFFFFF);
  FastLED.show();
  delay(500);
  Serial.print(".");
}

void connectToMqtt()
{
  Serial.println("Connecting to MQTT");
  mqttClient.connect();
}

void onMqttConnect(bool sessionPresent)
{
  Serial.println("Connected to MQTT.");
  Serial.print("Session present: ");
  Serial.println(sessionPresent);

  mqttClient.subscribe(cBrightnessSetTopic, 0);
  mqttClient.subscribe(cModeSetTopic, 0);
  sendState();
}

void onMqttDisconnect(AsyncMqttClientDisconnectReason reason)
{
  Serial.println("Disconnected from MQTT.");

  if (WiFi.isConnected())
  {
    mqttReconnectTimer.once(2, connectToMqtt);
  }
}

void onMqttMessage(char *topic, char *payload, AsyncMqttClientMessageProperties properties, size_t len, size_t index, size_t total)
{
  Serial.printf("topic: %s: ", topic);

  // THIS IS ONLY FOR SHORT PAYLOADS!!!
  // payload is in facte byte*, NOT char*!!!
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

void connectToWifi()
{
  Serial.println("Connecting to Wi-Fi");
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  while (WiFi.status() != WL_CONNECTED)
  {
    blinkLED(0);
  }
}

void onWifiConnect(const WiFiEventStationModeGotIP &event)
{
  Serial.println("Connected to Wi-Fi.");
  leds[0] = 0;
  FastLED.show();

  // initialize NTP Client after WiFi is connected
  timeClient.begin();
  connectToMqtt();
}

void onWifiDisconnect(const WiFiEventStationModeDisconnected &event)
{
  Serial.println("Disconnected from Wi-Fi.");
  mqttReconnectTimer.detach(); // ensure we don't reconnect to MQTT while reconnecting to Wi-Fi
  wifiReconnectTimer.once(2, connectToWifi);
}

void setup()
{
  Serial.begin(SERIAL_SPEED);

  FastLED.addLeds<CHIPSET, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
  FastLED.setBrightness(BRIGHTNESS);

  wordClock.setup();
  rainbowAnimation.setup();

  wifiConnectHandler = WiFi.onStationModeGotIP(onWifiConnect);
  wifiDisconnectHandler = WiFi.onStationModeDisconnected(onWifiDisconnect);

  mqttClient.onConnect(onMqttConnect);
  mqttClient.onDisconnect(onMqttDisconnect);
  mqttClient.onMessage(onMqttMessage);
  mqttClient.setServer(MQTT_HOST, MQTT_PORT);

  connectToWifi();
}

void loop()
{
  switch (clockMode)
  {
  case 0:
    wordClock.loop(modeChanged);
    break;
  case 1:
    rainbowAnimation.loop(modeChanged);
    break;
  default:
    wordClock.loop(modeChanged);
  }
  modeChanged = false;
}
