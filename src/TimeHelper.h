#pragma once

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
