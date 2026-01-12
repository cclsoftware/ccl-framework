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
// Filename    : ccl/platform/cocoa/gui/transparentwindow.cocoa.h
// Description : Transparent Window for Cocoa Windows
//
//************************************************************************************************

#ifndef _ccl_transparentwindow_cocoa_h
#define _ccl_transparentwindow_cocoa_h

#include "ccl/gui/windows/transparentwindow.h"

@class NSWindow;

namespace CCL {

//************************************************************************************************
// CocoaTransparentWindow
//************************************************************************************************

class CocoaTransparentWindow: public TransparentWindow
{
public:
	CocoaTransparentWindow (Window* parentWindow, int options, StringRef title);
	~CocoaTransparentWindow ();

	PROPERTY_POINTER (NSWindow, nativeWindow, NativeWindow)

    void suspend (bool state);

	// TransparentWindow
	void show ();
	void hide ();
	bool isVisible () const;
	void update (RectRef size, Bitmap& bitmap, PointRef offset = Point (), float opacity = 1.f);
	void move (PointRef position);

private:
	Rect size;
	bool initialized;
    bool visible;
    bool suspended;
	
	AutoPtr<Window> osxWindow;
};

} // namespace CCL

#endif // _ccl_transparentwindow_cocoa_h
