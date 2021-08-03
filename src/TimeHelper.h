#pragma once

#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <NTPClient.h>
#include <TimeZone.h>
#include <TimeLib.h>
#include <time.h>

const char *TC_SERVER = "europe.pool.ntp.org";

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, TC_SERVER);

// For starters use hardwired Central European Time (Berlin, Paris, ...)
TimeChangeRule CEST = {"CEST", Last, Sun, Mar, 2, 120}; // Central European Summer Time
TimeChangeRule CET = {"CET ", Last, Sun, Oct, 3, 60};   // Central European Standard Time
Timezone Europe(CEST, CET);

bool onGetTime(int &hours, int &minutes, int &seconds)
{
  if (!WiFi.isConnected())
    return false;
  if (timeClient.update())
  {
    time_t localTime = Europe.toLocal(timeClient.getEpochTime());

    hours = ((localTime % 86400L) / 3600) % 24;
    minutes = (localTime % 3600) / 60;
    seconds = localTime % 60;

    return true;
  }
  else
  {
    return false;
  }
}

class Uptime
{
public:
  Uptime();
  void reset();
  void update();
  uint64_t getSeconds() const;

private:
  uint64_t _milliseconds;
  uint32_t _lastTick;
};

Uptime::Uptime()
    : _milliseconds(0), _lastTick(0)
{
}

void Uptime::reset()
{
  _milliseconds = 0;
  _lastTick = millis();
}

void Uptime::update()
{
  uint32_t now = millis();
  _milliseconds += (now - _lastTick);
  _lastTick = now;
}

uint64_t Uptime::getSeconds() const
{
  return (_milliseconds / 1000ULL);
}
