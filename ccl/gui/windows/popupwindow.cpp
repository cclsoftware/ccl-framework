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
// Filename    : ccl/gui/windows/popupwindow.cpp
// Description : Popup Window
//
//************************************************************************************************

#include "ccl/gui/windows/popupwindow.h"

using namespace CCL;

//************************************************************************************************
// PopupWindow
//************************************************************************************************

DEFINE_CLASS_HIDDEN (PopupWindow, NativeWindow)

//////////////////////////////////////////////////////////////////////////////////////////////////

PopupWindow::PopupWindow (const Rect& size, StyleRef style, StringRef title, IWindow* parent)
: NativeWindow (size, style, title)
{
	if(style.isCustomStyle (Styles::kWindowBehaviorIntermediate))
		layer = kWindowLayerIntermediate;
	else if(style.isCustomStyle (Styles::kWindowBehaviorProgressDialog) || style.isCustomStyle (Styles::kWindowBehaviorPopupSelector))
		layer = kPopupLayer;
	else if(style.isCustomStyle (Styles::kWindowBehaviorFloating) || style.isCustomStyle (Styles::kWindowAppearanceCustomFrame))
		layer = kWindowLayerFloating;
	else
		layer = kWindowLayerBase;

	makeNativePopupWindow (parent);
}
