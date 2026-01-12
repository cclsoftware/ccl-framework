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
// Filename    : ccl/platform/win/gui/fontresource.android.cpp
// Description : Android Font Resource
//
//************************************************************************************************

#include "ccl/gui/system/fontresource.h"

#include "ccl/platform/android/graphics/frameworkgraphics.h"

using namespace CCL;
using namespace Android;

namespace CCL {

//************************************************************************************************
// AndroidFontResource
//************************************************************************************************

class AndroidFontResource: public FontResource
{
public:
	AndroidFontResource (CCL::IStream& stream, StringRef name, int fontStyle);
	~AndroidFontResource ();

private:
	AndroidFont* font;
};

} // namespace CCL

using namespace CCL;

//************************************************************************************************
// AndroidFontResource
//************************************************************************************************

FontResource* FontResource::install (CCL::IStream& stream, StringRef name, int fontStyle)
{
	return NEW AndroidFontResource (stream, name, fontStyle);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

AndroidFontResource::AndroidFontResource (CCL::IStream& stream, StringRef name, int fontStyle)
: font (gGraphicsFactory->loadFont (stream, name, fontStyle))
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

AndroidFontResource::~AndroidFontResource ()
{
	/*
	if(font)
		gGraphicsFactory->unloadFont (font);
	*/
}
