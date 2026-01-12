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
// Filename    : ccl/gui/windows/popupwindow.h
// Description : Popup Window
//
//************************************************************************************************

#ifndef _ccl_popupwindow_h
#define _ccl_popupwindow_h

#include "ccl/gui/windows/nativewindow.h"

namespace CCL {

//************************************************************************************************
// PopupWindow
//************************************************************************************************

class PopupWindow: public NativeWindow
{
public:
	DECLARE_CLASS (PopupWindow, NativeWindow)

	PopupWindow (const Rect& size = Rect (), StyleRef style = 0, StringRef title = nullptr, IWindow* parent = nullptr);
};

} // namespace CCL

#endif // _ccl_popupwindow_h
