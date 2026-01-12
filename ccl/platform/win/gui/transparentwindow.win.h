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
// Filename    : ccl/platform/win/gui/transparentwindow.win.h
// Description : Transparent Window
//
//************************************************************************************************

#ifndef _ccl_transparentwindow_win_h
#define _ccl_transparentwindow_win_h

#include "ccl/gui/windows/transparentwindow.h"
#include "ccl/platform/win/cclwindows.h"

namespace CCL {

//************************************************************************************************
// WindowsTransparentWindow
//************************************************************************************************

class WindowsTransparentWindow: public TransparentWindow
{
public:
	WindowsTransparentWindow (Window* parentWindow, int options, StringRef title);
	~WindowsTransparentWindow ();

	PROPERTY_VARIABLE (HWND, nativeWindow, NativeWindow)

	// TransparentWindow
	void show () override;
	void hide () override;
	bool isVisible () const override;
	void update (RectRef size, Bitmap& bitmap, PointRef offset = Point (), float opacity = 1.f) override;
	void move (PointRef position) override;
};

} // namespace CCL

#endif _ccl_transparentwindow_win_h
