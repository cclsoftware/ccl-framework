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
// Filename    : ccl/gui/graphics/imaging/offscreen.h
// Description : Offscreen class
//
//************************************************************************************************

#ifndef _ccl_offscreen_h
#define _ccl_offscreen_h

#include "ccl/gui/graphics/imaging/bitmap.h"

namespace CCL {

class Window;

//************************************************************************************************
// Offscreen
/** Bitmap class for offscreen drawing, can change its size. */
//************************************************************************************************

class Offscreen: public Bitmap
{
public:
	/** Construct offscreen of given size and pixel format. */
	Offscreen (int width = 1, int height = 1, PixelFormat format = kAny, bool global = false, Window* window = nullptr);

	/** Reallocate offscreen if necessary. */
	#if DEBUG
	bool updateSize (int width, int height, CStringPtr debugName);
	#else
	bool updateSize (int width, int height);
	#endif

	bool recreate (int width, int height);

protected:
	PixelFormat format;
	bool global;
	Window* window;
};

} // namespace CCL

#endif // _ccl_offscreen_h
