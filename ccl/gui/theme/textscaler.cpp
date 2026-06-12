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
// Filename    : ccl/gui/theme/textscaler.cpp
// Description : Text Scaler
//
//************************************************************************************************

#include "ccl/gui/theme/textscaler.h"

#include "ccl/gui/graphics/markupsupport.h"

#include "ccl/public/math/mathprimitives.h"

using namespace CCL;

//************************************************************************************************
// TextScaler
//************************************************************************************************

TextScaler::TextScaler ()
: cachedFontSize (0),
  explicitMaximalFontSize (100),
  explicitMinimalFontSize (6)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TextScaler::scaleTextFont (Font& font, RectRef r, StringRef text, int options)
{
	if(!(options & kDisableCache) && text == cachedText && r == cachedRect)
	{
		font.setSize (cachedFontSize);
		return;
	}

	float upperBound = ccl_min (explicitMaximalFontSize, (r.getHeight () * 0.75f));
	float lowerBound = explicitMinimalFontSize;
	ASSERT (upperBound >= lowerBound)
	if(upperBound < lowerBound)
		ccl_swap (upperBound, lowerBound);

	float fontSize = ccl_bound (font.getSize (), lowerBound, upperBound);
	font.setSize (fontSize);

	while(true)
	{
		Rect size;
		if(options & kMarkupText)
			MarkupPainter ().measureMarkupString (size, text, font);
		else
		{
			RectF imageSize;
			Font::measureString (imageSize, text, font);
			size = rectFToEnclosingInt (imageSize);
		}

		if(r.getWidth () == size.getWidth ())
			break;

		float newFontSize = fontSize;
		if(r.getWidth () > size.getWidth ())
		{
			lowerBound = fontSize;
			newFontSize = ccl_round<2> ((fontSize + upperBound) / 2.f);
		}
		else
		{
			upperBound = fontSize;
			newFontSize = ccl_round<2> ((fontSize + lowerBound) / 2.f);
		}

		if(fontSize <= newFontSize && ccl_equals (fontSize, newFontSize, .1f))
			break;

		fontSize = ccl_round<2> (newFontSize);
		font.setSize (fontSize);
	}

	font.setSize (floorf (font.getSize () * 2.f) / 2.f); // limit to 0.5pt steps

	if(!(options & kDisableCache))
	{
		cachedRect = r;
		cachedText = text;
		cachedFontSize = font.getSize ();
	}
}
