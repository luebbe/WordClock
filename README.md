# WordClock

An ESP8266 based Word Clock built with PlatformIO that fetches the time from a NTP server.

This is firmware for the 11x10 LED Matrix word clock case from [WordClock 12h (desk clock edition) from Thingiverse](https://www.thingiverse.com/thing:2130830).

It currently has three modes:

- word clock with seconds as ambilight
- rainbow animation
- aurora borealis animation

The modes and brightness can be set via mqtt by writing to the following mqtt topics:

- `wordclock/brightness/set` (0..255) - very low brighness values (e.g. less than 20) may lead to colors not being shown properly
- `wordclock/mode/set` (0..2)

When the word clock is powered up, it starts in mode 0 (word clock) with brightness 20.

The word clock reports its state via the following mqtt topics:

- `wordclock/$localip` - the ip address assigned to the word clodk
- `wordclock/$mac` - the mac address of the ESP8266
- `wordclock/$state` - (ready|lost). `ready` when connected to wifi and mqtt. The state switches to `lost` automatically, when the connection is lost.
- `wordclock/brightness` - the current brightness
- `wordclock/mode` - the current mode

Dependencies are:

- FastLed
- NtpClient
- Time
- TimeZone
- AsyncMqttClient

If you build the firmware using PlatformIO, all dependencies are pulled in automatically. If you prefer a different IDE you have to take care of everything yourself.

The wifi and mqtt credentials are stored in an external "secrets.h" file.

| ![Open case](./images/img_case_open_1.jpg) | ![Open case](./images/img_case_open_2.jpg) |
| :----------------------------------------: | :----------------------------------------: |
| Open case                                  | Open case                                  |

The pictures below are taken with almost minimal brightness of the LEDs.

| ![No diffusor](./images/img_no_diffusor.jpg) | ![Paper diffusor](./images/img_paper_diffusor.jpg) | ![With front](./images/img_with_front.jpg) |
| :------------------------------------------: | :------------------------------------------------: | :----------------------------------------: |
| No diffusor                                  | A sheet of paper as diffusor                       | Paper and front                            |
