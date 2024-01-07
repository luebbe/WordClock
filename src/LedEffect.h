
/*
 * base class to display animations on a LED strip
 *
 * Author: Lübbe Onken (http://github.com/luebbe)
 */

#pragma once

#include <FastLED.h>

class LedEffect
{
protected:
	CRGB *const _leds;
	const uint16_t _numLeds;
	CRGBPalette16 _currentPalette;

	CRGB getRandomColor();
	CRGB getColorFromPalette(uint8_t index);

public:
	explicit LedEffect(CRGB *leds, uint16_t count);
	virtual ~LedEffect();

	virtual void init();
	virtual bool paint(bool force) = 0;

	void setPalette(CRGBPalette16 value)
	{
		_currentPalette = value;
	}
};
