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
// Filename    : ccl/public/gui/graphics/color.cpp
// Description : Color class
//
//************************************************************************************************

#include "ccl/public/gui/graphics/color.h"
#include "ccl/public/gui/graphics/igraphicshelper.h"

#include "ccl/public/text/cclstring.h"
#include "ccl/public/math/mathprimitives.h"

using namespace CCL;

//************************************************************************************************
// Colors
//************************************************************************************************

Color Colors::kTransparentBlack (0x00, 0x00, 0x00, 0x00);
ColorF Colors::kTransparentBlackF (0.f, 0.f, 0.f, 0.f);
Color Colors::kBlack (0x00, 0x00, 0x00);
Color Colors::kWhite (0xFF, 0xFF, 0xFF);
Color Colors::kRed (0xFF, 0x00, 0x00);
Color Colors::kGreen (0x00, 0xFF, 0x00);
Color Colors::kBlue (0x00, 0x00, 0xFF);
Color Colors::kYellow (0xFF, 0xFF, 0x00);
Color Colors::kGray (0x86, 0x86, 0x86);
Color Colors::kLtGray (0xD3, 0xD3, 0xD3);

//////////////////////////////////////////////////////////////////////////////////////////////////

void Colors::toCString (ColorRef color, char* cString, int cStringSize, bool withAlpha)
{
	System::GetGraphicsHelper ().Color_toCString (color, cString, cStringSize, 
												  withAlpha ? Internal::IGraphicsHelper::kColorWithAlpha : 0);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Colors::fromCString (Color& color, CStringPtr cString)
{
	return System::GetGraphicsHelper ().Color_fromCString (color, cString) != 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Colors::toString (ColorRef color, String& string, bool withAlpha)
{
	char cString[255] = {0};
	toCString (color, cString, sizeof(cString), withAlpha);
	string.empty ();
	string.appendASCII (cString);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Colors::fromString (Color& color, StringRef string)
{
	char cString[255] = {0};
	string.toASCII (cString, sizeof(cString));
	return fromCString (color, cString);
}

