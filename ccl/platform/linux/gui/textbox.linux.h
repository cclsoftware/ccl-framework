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
// Filename    : ccl/platform/linux/gui/textbox.linux.h
// Description : Platform-specific Text Control implementation
//
//************************************************************************************************

#ifndef _ccl_textbox_linux_h
#define _ccl_textbox_linux_h

#include "ccl/platform/linux/gui/window.linux.h"

#include "ccl/gui/controls/editbox.h"

namespace CCL {

//************************************************************************************************
// LinuxEditBox
//************************************************************************************************

class LinuxEditBox: public LinuxWindow
{
public:
	LinuxEditBox (IWindow* parent, const Rect& size = Rect ());
	~LinuxEditBox ();
	
	// LinuxWindow
	tbool CCL_API close () override;
	
private:
	struct Listener: zwp_text_input_v3_listener
	{
		Listener (LinuxEditBox& window);
		
		static void onEnter (void* data, zwp_text_input_v3* textInput, struct wl_surface *surface);
		static void onLeave (void* data, zwp_text_input_v3* textInput, struct wl_surface *surface);
		static void onPreeditString (void* data, zwp_text_input_v3* textInput, const char* text, int32_t cursorBegin, int32_t cursorEnd);
		static void onCommitString (void* data, zwp_text_input_v3* textInput, const char* text);
		static void onDeleteSurroundingText (void* data, zwp_text_input_v3* textInput, uint32_t beforeLength, uint32_t afterLength);
		static void onDone (void* data, zwp_text_input_v3* textInput, uint32_t serial);
		
	private:
		LinuxEditBox& window;
	};
	Listener listener;
	
	zwp_text_input_v3* textInput;
};

//************************************************************************************************
// LinuxTextControl
//************************************************************************************************

class LinuxTextControl: public NativeTextControl
{
public:
	LinuxTextControl (Control& owner, const Rect& clientRect, int returnKeyType, int keyboardType);
	~LinuxTextControl ();

	// NativeTextControl
	void updateText () override;
	void getControlText (String& string) override;
	void setSelection (int start, int length) override;
	void setScrollPosition (PointRef where) override;
	Point getScrollPosition () const override;
	void setSize (RectRef clientRect) override;
	void updateVisualStyle () override;
	
protected:
	LinuxEditBox editBox;
};

} // namespace CCL

#endif // _ccl_textbox_linux_h
