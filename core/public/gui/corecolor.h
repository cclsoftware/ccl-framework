//************************************************************************************************
//
// This file is part of Crystal Class Library (R)
// Copyright (c) 2025 CCL Software Licensing GmbH.
// All Rights Reserved.
//
// Licensed for use under either:
//  1. a Commercial License provided by CCL Software Licensing GmbH, or
//  2. GNU Affero General Public License v3.0 (AGPLv3).
// 
// You must choose and comply with one of the above licensing options.
// For more information, please visit ccl.dev.
//
// Filename    : core/public/gui/corecolor.h
// Description : Color classes
//
//************************************************************************************************

#ifndef _corecolor_h
#define _corecolor_h

#include "core/public/coretypes.h"
#include "core/public/coreprimitives.h"

namespace Core {

struct Color;
struct ColorF;

/** Color reference type (8 bit). */
typedef const Color& ColorRef;

/** Color reference type (floating point). */
typedef const ColorF& ColorFRef;

//************************************************************************************************
// Color
/** Color definition (8 bit RGBA).
	\ingroup core_gui */
//************************************************************************************************

struct Color
{
	uint8 red;		///< red channel
	uint8 green;	///< green channel
	uint8 blue;		///< blue channel
	uint8 alpha;	///< alpha channel

	/** Construct color with RGBA values. */
	Color (uint8 r = 0, uint8 g = 0, uint8 b = 0, uint8 a = 0xFF)
	: red (r), green (g), blue (b), alpha (a)
	{}

	/** Compose color from linear gradient. */
	static Color linearGradient (Color start, Color end, float position = 0.5f);

	/** Get color from 32 bit integer. */
	static Color fromInt (uint32 color);

	/** Set color component bounded. */
	static uint8 setC (float f);

	/** Set color from 32 bit integer. */
	Color& operator () (uint32 color);

	/** Get color as 32 bit integer. */
	operator uint32 () const;

	/** Set all componets. */
	Color& set (uint8 r, uint8 g, uint8 b, uint8 a);

	/** Get red component normalized. */
	float getRedF () const;

	/** Get green component normalized. */
	float getGreenF () const;

	/** Get blue component normalized. */
	float getBlueF () const;

	/** Get alpha component normalized. */
	float getAlphaF () const;

	/** Set red component normalized. */
	Color& setRedF (float f);

	/** Set green component normalized. */
	Color& setGreenF (float f);

	/** Set blue component normalized. */
	Color& setBlueF (float f);

	/** Set alpha component normalized. */
	Color& setAlphaF (float f);

	/** Set color normalized. */
	Color& setF (float r, float g, float b, float a);

	/** Get intensity: average of RGB channels. */
	float getIntensity () const;

	/** Get luminance: weighted average of RGB channels. */
	float getLuminance () const;

	/** Scale RGB channels equally by given factor. */
	Color& setIntensity (float intensity);

	/** Add brightnesss to all RGB channels by given amount. */
	Color& addBrightness (float amount);

	/** Grayscale color. */
	Color& grayScale ();

	/** Alpha-blend on given source color. */
	Color& alphaBlend (Color srcColor, float alpha = 1.f);

	/** Render alpha into the color against a given background */
	Color& renderAlpha (Color background);

	/** Apply given factor to alpha value. */
	Color& scaleAlpha (float factor);

	/** Check if color is opaque (alpha = FF). */
	bool isOpaque () const;

	/** Check if color is translucent (alpha < FF). */
	bool isTranslucent () const;
};

//************************************************************************************************
// ColorF
/** Floating point color definition.
	\ingroup core_gui */
//************************************************************************************************

struct ColorF
{
	union
	{
		struct
		{
			float red;		///< red [0,1]
			float green;	///< green [0,1]
			float blue;		///< blue [0,1]
			float alpha;	///< alpha [0,1]
		};
		float values[4];
	};

	/** Construct color with RGBA values. */
	ColorF (float r = 0, float g = 0, float b = 0, float a = 1.f)
	: red (r), green (g), blue (b), alpha (a)
	{}

	/** Construct from 8 bit color. */
	explicit ColorF (ColorRef c)
	: red (c.getRedF ()), green (c.getGreenF ()), blue (c.getBlueF ()), alpha (c.getAlphaF ())
	{}

	/** Compose color from linear gradient. */
	static ColorF linearGradient (ColorFRef start, ColorFRef end, float position = 0.5f);

	/** Convert to 8 bit color. */
	operator Color () const
	{
		Color c;
		c.setRedF (red);
		c.setGreenF (green);
		c.setBlueF (blue);
		c.setAlphaF (alpha);
		return c;
	}
};

//************************************************************************************************
// ColorHSL
/** Color hue/saturation/lightness representation.
	\ingroup core_gui */
//************************************************************************************************

struct ColorHSL
{
	float h;	///< hue [0,360]
	float s;	///< saturation [0,1]
	float l;	///< lightness [0,1]
	float a;	///< alpha [0,1]

	ColorHSL (float h = 0, float s = 0, float l = 0, float a = 1)
	: h (h), s (s), l (l), a (a)
	{}

	/** Construct from 8 bit color. */
	ColorHSL (ColorRef color)
	{ fromColor (color); }

	/** Assign from 8 bit color. */
	ColorHSL& fromColor (ColorRef color);
	
	/** Convert to 8 bit color. */
	const ColorHSL& toColor (Color& color) const;
	
	/** Convert to 8 bit color. */
	Color toColor () const;
	
	/** Convert to 8 bit color. */
	operator Color () const;

	/** Assign from floating point RGBA values. */
	void fromRGBA (float r, float g, float b, float a);
	
	/** Convert to floating point RGBA values. */
	void toRGBA (float& r, float& g, float& b, float& a) const;

	/** Get difference between two HSL colors. */
	static float getDifference (const ColorHSL& c1, const ColorHSL& c2);

private:
	static float calcRGB (float c, float t1, float t2);
};

//************************************************************************************************
// ColorHSV
/** Color hue/saturation/value representation.
	\ingroup core_gui */
//************************************************************************************************

struct ColorHSV
{
	float h;	///< hue [0,360]
	float s;	///< saturation [0,1]
	float v;	///< value (brightness) [0,1]
	float a;	///< alpha [0,1]

	ColorHSV (float h = 0, float s = 0, float v = 0, float a = 1)
	: h (h), s (s), v (v), a (a)
	{}

	/** Construct from 8 bit color. */
	ColorHSV (ColorRef color)
	{ fromColor (color); }

	/** Construct from 8 bit color. */
	ColorHSV& fromColor (ColorRef color);
	
	/** Convert to 8 bit color. */
	const ColorHSV& toColor (Color& color) const;
	
	/** Convert to 8 bit color. */
	Color toColor () const;

	/** Convert to 8 bit color. */
	operator Color () const;

	/** Assign from floating point RGBA values. */
	void fromRGBA (float r, float g, float b, float a);
	
	/** Convert to floating point RGBA values. */
	void toRGBA (float& r, float& g, float& b, float& a) const;
};

//////////////////////////////////////////////////////////////////////////////////////////////////
// Color inline
//////////////////////////////////////////////////////////////////////////////////////////////////

inline Color::operator uint32 () const
{ return red | (green << 8) | (blue << 16) | (alpha << 24); }

inline Color& Color::set (uint8 r, uint8 g, uint8 b, uint8 a)
{ red = r; green = g; blue = b; alpha = a; return *this; }

inline float Color::getRedF () const
{ return (float)red / 255.f; }

inline float Color::getGreenF () const
{ return (float)green / 255.f; }

inline float Color::getBlueF () const
{ return (float)blue / 255.f; }

inline float Color::getAlphaF () const
{ return (float)alpha / 255.f; }

inline Color& Color::setRedF (float f)
{ red = setC (f * 255.f); return *this; }

inline Color& Color::setGreenF (float f)
{ green = setC (f * 255.f); return *this; }

inline Color& Color::setBlueF (float f)
{ blue = setC (f * 255.f); return *this; }

inline Color& Color::setAlphaF (float f)
{ alpha = setC (f * 255.f); return *this; }

inline Color& Color::setF (float r, float g, float b, float a)
{ return setRedF (r).setGreenF (g).setBlueF (b).setAlphaF (a); }

inline bool Color::isOpaque () const
{ return alpha == 0xFF; }

inline bool Color::isTranslucent () const
{ return alpha != 0xFF; }

inline uint8 Color::setC (float value)
{ return value > 255.f ? 0xFF : value < 0.f ? 0x00 : uint8(value); }

//////////////////////////////////////////////////////////////////////////////////////////////////

inline Color Color::linearGradient (Color start, Color end, float position)
{
	return Color (
		setC ((float)start.red + position * (float)(end.red - start.red)),
		setC ((float)start.green + position * (float)(end.green - start.green)),
		setC ((float)start.blue + position * (float)(end.blue - start.blue)),
		setC ((float)start.alpha + position * (float)(end.alpha - start.alpha)));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline Color Color::fromInt (uint32 color)
{
	Color c;
	c (color);
	return c;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline Color& Color::operator () (uint32 color)
{
	red   = (uint8)((color >>  0) & 0xFF);
	green = (uint8)((color >>  8) & 0xFF);
	blue  = (uint8)((color >> 16) & 0xFF);
	alpha = (uint8)((color >> 24) & 0xFF);
	return *this;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline float Color::getIntensity () const
{
	return ((float)red + (float)green + (float)blue) / (3.f * 255.f);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline float Color::getLuminance () const
{
	return (.3f / 255.f) * (float)red + (.59f / 255.f) * green + (.11f / 255.f) * (float)blue;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline Color& Color::setIntensity (float intensity)
{
	red   = setC (red * intensity);
	green = setC (green * intensity);
	blue  = setC (blue * intensity);
	return *this;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline Color& Color::addBrightness (float amount)
{
	red   = setC (red + amount * 255.f);
	green = setC (green + amount * 255.f);
	blue  = setC (blue + amount * 255.f);
	return *this;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline Color& Color::grayScale ()
{
	uint8 l = setC (getLuminance () * 255.f);
	red = l;
	green = l;
	blue = l;
	return *this;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline Color& Color::alphaBlend (Color _src, float alphaFactor)
{
	Color src (_src);
	if(alphaFactor < 1.f)
	{
		src.setIntensity (alphaFactor);
		src.setAlphaF (alphaFactor);
	}

	float srcAlpha = src.getAlphaF ();

	this->red = setC ((float)src.red + (1.f - srcAlpha) * (float)red);
	this->green = setC ((float)src.green + (1.f - srcAlpha) * (float)green);
	this->blue = setC ((float)src.blue + (1.f - srcAlpha) * (float)blue);
	this->alpha = setC ((float)src.alpha + (1.f - srcAlpha) * (float)alpha);
	return *this;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline Color& Color::renderAlpha (Color background)
{
	ASSERT (background.getAlphaF () == 1.f) // background has to be opaque
	background.alphaBlend (*this, getAlphaF ());
	*this = background;
	setAlphaF (1.f);
	return *this;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline Color& Color::scaleAlpha (float factor)
{
	alpha = setC (alpha * factor);
	return *this;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// ColorF inline
//////////////////////////////////////////////////////////////////////////////////////////////////

inline ColorF ColorF::linearGradient (ColorFRef start, ColorFRef end, float position)
{
	return ColorF (
		bound (start.red + position * (end.red - start.red), 0.f, 1.f),
		bound (start.green + position * (end.green - start.green), 0.f, 1.f),
		bound (start.blue + position * (end.blue - start.blue), 0.f, 1.f),
		bound (start.alpha + position * (end.alpha - start.alpha), 0.f, 1.f));
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// ColorHSL inline
//////////////////////////////////////////////////////////////////////////////////////////////////

inline ColorHSL& ColorHSL::fromColor (ColorRef c)
{
	fromRGBA (c.getRedF (), c.getGreenF (), c.getBlueF (), c.getAlphaF ());
	return *this;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline const ColorHSL& ColorHSL::toColor (Color& c) const
{
	float r = 0, g = 0, b = 0, a = 0;
	toRGBA (r, g, b, a);
	c.setRedF (r);
	c.setGreenF (g);
	c.setBlueF (b);
	c.setAlphaF (a);
	return *this;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline Color ColorHSL::toColor () const
{
	Color c;
	toColor (c);
	return c;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline ColorHSL::operator Color () const
{ return toColor (); }

//////////////////////////////////////////////////////////////////////////////////////////////////

inline void ColorHSL::fromRGBA (float r, float g, float b, float _a)
{
	float min = get_min (get_min (r, g), b);
	float max = get_max (get_max (r, g), b);
	float delta = max - min;

	h = 0;
	s = 0;
	l = (max + min) / 2.0f;
	a = _a;

	if(delta != 0)
	{
		if(l < 0.5f)
			s = delta / (max + min);
		else
			s = delta / (2.0f - max - min);

		if(r == max)
			h = (g - b) / delta;
		else if(g == max)
			h = 2.f + (b - r) / delta;
		else if(b == max)
			h = 4.f + (r - g) / delta;

		h *= 60;
		if(h < 0)
			h += 360;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline float ColorHSL::calcRGB (float c, float t1, float t2)
{
	if(c < 0) c += 1;
	if(c > 1) c -= 1;
	if(6.f * c < 1.f) return t1 + (t2 - t1) * 6.f * c;
	if(2.f * c < 1.f) return t2;
	if(3.f * c < 2.f) return t1 + (t2 - t1) * (2.f / 3.f - c) * 6.f;
	return t1;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline void ColorHSL::toRGBA (float& r, float& g, float& b, float& _a) const
{
	_a = a;

	if(s == 0)
	{
		r = g = b = l;
	}
	else
	{
		float t2;
		if(l < 0.5)
			t2 = l * (1 + s);
		else
			t2 = (l + s) - (l * s);
		float t1 = 2 * l - t2;

		float th = h / 360.f;
		float tr = th + (1.f / 3.f);
		float tg = th;
		float tb = th - (1.f / 3.f);

		r = calcRGB (tr, t1, t2);
		g = calcRGB (tg, t1, t2);
		b = calcRGB (tb, t1, t2);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline float ColorHSL::getDifference (const ColorHSL& c1, const ColorHSL& c2)
{
	float h1 = c1.h;
	float h2 = c2.h;
	float hMin = h1 < h2 ? h1 : h2;
	float hMax = h1 > h2 ? h1 : h2;
	if(hMax - hMin > 180)
		hMin += 360;

	static const float hWeight = 1.0f;
	static const float sWeight = 50.0f;
	static const float lWeight = 50.0f;

	float hVal = (hMax - hMin) * hWeight;
	float sVal = (c1.s - c2.s) * sWeight;
	float lVal = (c1.l - c2.l) * lWeight;
	return (hVal * hVal) + (sVal * sVal) + (lVal * lVal);
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// ColorHSV inline
//////////////////////////////////////////////////////////////////////////////////////////////////

inline ColorHSV& ColorHSV::fromColor (ColorRef c)
{
	fromRGBA (c.getRedF (), c.getGreenF (), c.getBlueF (), c.getAlphaF ());
	return *this;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline const ColorHSV& ColorHSV::toColor (Color& c) const
{
	float r = 0, g = 0, b = 0, a = 0;
	toRGBA (r, g, b, a);
	c.setRedF (r);
	c.setGreenF (g);
	c.setBlueF (b);
	c.setAlphaF (a);
	return *this;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline Color ColorHSV::toColor () const
{
	Color c;
	toColor (c);
	return c;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline ColorHSV::operator Color () const
{ return toColor (); }

//////////////////////////////////////////////////////////////////////////////////////////////////

inline void ColorHSV::fromRGBA (float r, float g, float b, float _a)
{
	float min = get_min (get_min (r, g), b);
	float max = get_max (get_max (r, g), b);
	float delta = max - min;

	v = max;	// value
	s = 0;
	h = 0;
	a = _a;

	if(max != 0)
		s = delta / max;	// saturation

	if(delta != 0)
	{
		if(r == max)
			h = (g - b) / delta;		// between yellow & magenta
		else if(g == max)
			h = 2.f + (b - r) / delta;	// between cyan & yellow
		else
			h = 4.f + (r - g) / delta;	// between magenta & cyan

		h *= 60;						// degrees
		if(h < 0)
			h += 360;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline void ColorHSV::toRGBA (float& r, float& g, float& b, float& _a) const
{
	_a = a;

	if(s == 0)
	{
		// achromatic (grey)
		r = g = b = v;
		return;
	}

	float sector = h / 60;      // sector 0 to 5
	int i = (int)sector;
	float f = sector - i;		  // factorial part of h
	float p = v * (1 - s );
	float q = v * (1 - s * f);
	float t = v * (1 - s * (1 - f));

	switch(i)
	{
	case 6 :
	case 0 : r = v; g = t; b = p; break;
	case 1 : r = q; g = v; b = p; break;
	case 2 : r = p; g = v; b = t; break;
	case 3 : r = p; g = q; b = v; break;
	case 4 : r = t; g = p; b = v; break;
	default:		// case 5 :
		r = v;
		g = p;
		b = q;
		break;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

} // namespace Core

#endif // _corecolor_h
