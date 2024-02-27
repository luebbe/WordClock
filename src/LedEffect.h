
/*
 * base class to display animations on a LED strip
 *
 * Author: Lübbe Onken (http://github.com/luebbe)
 */

#pragma once

#include <FastLED.h>

#define UPDATE_MS 50 // Update the display 20 times per second in order to follow the brightness changes quicker

class LedEffect
{
protected:
	CRGB *const _leds;
	const uint16_t _numLeds;
	CRGBPalette16 _currentPalette;
	CRGBPalette16 _randomPalette;

	CRGB getRandomColor();
	CRGB getColorFromPalette(uint8_t index);

public:
	explicit LedEffect(CRGB *leds, uint16_t count);
	virtual ~LedEffect();

	virtual void init();
	virtual bool paint(bool force) = 0;

	void createRandomPalette();

	void setPalette(CRGBPalette16 value)
	{
		_currentPalette = value;
		paint(true);
	}

	void setRandomPalette()
	{
		// Always create a random palette before assigning it
		createRandomPalette();
		setPalette(_randomPalette);
	}
};
