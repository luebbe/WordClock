
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

	CRGB getRandomColor() const;

public:
	explicit LedEffect(CRGB * leds, uint16_t count);
	virtual ~LedEffect();

	virtual void init();
	virtual bool paint(bool force) = 0;

	// virtual operator const char *() const = 0;
};
