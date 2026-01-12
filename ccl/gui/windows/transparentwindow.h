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
// Filename    : ccl/gui/windows/transparentwindow.h
// Description : Transparent Window
//
//************************************************************************************************

#ifndef _ccl_transparentwindow_h
#define _ccl_transparentwindow_h

#include "ccl/base/object.h"

#include "ccl/gui/graphics/imaging/bitmap.h"

namespace CCL {

class Window;
class Bitmap;

//************************************************************************************************
// TransparentWindow
/** A transparent window makes use of window compositing provided by the OS. */
//************************************************************************************************

class TransparentWindow: public Object
{
public:
	DECLARE_CLASS (TransparentWindow, Object)

	static TransparentWindow* create (Window* parentWindow = nullptr, int options = 0, StringRef title = nullptr);

	enum Options { kKeepOnTop = 1<<0 };

	~TransparentWindow ();

	PROPERTY_SHARED_AUTO (Bitmap, savedBitmap, SavedBitmap)
	PROPERTY_READONLY_FLAG (options, kKeepOnTop, isKeepOnTop)

	Window* getParentWindow () const;
	float getContentScaleFactor () const;

	virtual void show ();
	virtual void hide ();
	virtual bool isVisible () const;
	
	virtual void update (RectRef size, Bitmap& bitmap, PointRef offset = Point (), float opacity = 1.f);
	virtual void move (PointRef position);

protected:
	Window* parentWindow;
	int options;
	String title;

	TransparentWindow (Window* parentWindow = nullptr, int options = 0, StringRef title = nullptr);
};

} // namespace CCL

#endif // _ccl_transparentwindow_h
