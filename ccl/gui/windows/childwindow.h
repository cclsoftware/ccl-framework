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
// Filename    : ccl/gui/windows/childwindow.h
// Description : Child Window class
//
//************************************************************************************************

#ifndef _ccl_childwindow_h
#define _ccl_childwindow_h

#include "ccl/gui/windows/nativewindow.h"

namespace CCL {

//************************************************************************************************
// ChildWindow
//************************************************************************************************

class ChildWindow: public NativeWindow
{
public:
	DECLARE_CLASS (ChildWindow, NativeWindow)

	ChildWindow (void* nativeParent, 
				 WindowMode mode,
				 const Rect& size = Rect (),
				 StyleRef style = 0, 
				 StringRef title = nullptr);

	ChildWindow (WindowMode mode = kWindowModeEmbedding,
				 const Rect& size = Rect (),
				 StyleRef style = 0,
				 StringRef title = nullptr);

	void makeNativeWindow (void* nativeParent);

	// NativeWindow
	void onSize (const Point& delta) override;
	void onActivate (bool state) override;
	bool onKeyDown (const KeyEvent& event) override;
	bool onKeyUp (const KeyEvent& event) override;
};

} // namespace CCL

#endif // _ccl_childwindow_h
