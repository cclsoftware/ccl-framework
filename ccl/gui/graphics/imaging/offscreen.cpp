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
// Filename    : ccl/gui/graphics/imaging/offscreen.cpp
// Description : Offscreen class
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "ccl/gui/graphics/imaging/offscreen.h"

#include "ccl/gui/graphics/nativegraphics.h"

using namespace CCL;

//************************************************************************************************
// Offscreen
//************************************************************************************************

Offscreen::Offscreen (int width, int height, PixelFormat format, bool global, Window* window)
: format (format),
  global (global),
  window (window)
{
	if(width > 0 && height > 0)
		assign (NativeGraphicsEngine::instance ().createOffscreen (width, height, format, global, window));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

#if DEBUG
bool Offscreen::updateSize (int width, int height, CStringPtr debugName)
#else
bool Offscreen::updateSize (int width, int height)
#endif
{
	int delta = 100; // reallocate every 100 points

	int newWidth  = (width  / delta + 1) * delta;
	int newHeight = (height / delta + 1) * delta;

	if(newWidth != getWidth () || newHeight != getHeight ())
	{
		CCL_PRINTF ("Offscreen %s reallocate from %dx%d to %dx%d\n", debugName ? debugName : "(unnamed)", getWidth (), getHeight (), newWidth, newHeight)

		recreate (newWidth, newHeight);
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Offscreen::recreate (int width, int height)
{
	if(nativeBitmap)
		nativeBitmap->release ();

	assign (NativeGraphicsEngine::instance ().createOffscreen (width, height, format, global, window));
	return true;
}
