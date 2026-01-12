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
// Filename    : ccl/platform/win/gui/textbox.win.h
// Description : Platform-specific Text Control implementation
//
//************************************************************************************************

#ifndef _ccl_textbox_win_h
#define _ccl_textbox_win_h

#include "ccl/gui/controls/editbox.h"
#include "ccl/gui/system/systemevent.h"

namespace CCL {

//************************************************************************************************
// WindowsTextControl
//************************************************************************************************

class WindowsTextControl: public NativeTextControl,
						  public SystemEventHandler
{
public:
	WindowsTextControl (Control& owner, const Rect& clientRect, int returnKeyType, int keyboardType);
	~WindowsTextControl ();

	static WindowsTextControl* fromHWND (void* hwnd);

	void* getBrush () const;
	unsigned int getColor () const;
	unsigned int getBackColor () const;

	// NativeTextControl
	void updateText () override;
	void getControlText (String& string) override;
	void setSelection (int start, int length) override;
	void setScrollPosition (PointRef where) override;
	Point getScrollPosition () const override;
	void setSize (RectRef clientRect) override;
	void updateVisualStyle () override;
	void CCL_API notify (ISubject* subject, MessageRef msg) override;

	// SystemEventHandler
	EventResult handleEvent (SystemEvent& e) override;

protected:
	void* handle;
	void* hFont;
	void* hBrush;
	Point lastScrollPos;
	Color backColor;

	float getContentScaleFactor () const;
};

} // namespace CCL

#endif // _ccl_textbox_win_h
