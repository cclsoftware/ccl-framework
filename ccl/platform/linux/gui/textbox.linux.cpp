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
// Filename    : ccl/platform/linux/gui/textbox.linux.cpp
// Description : Platform-specific Text Control implementation
//
//************************************************************************************************

#include "ccl/platform/linux/gui/textbox.linux.h"
#include "ccl/platform/linux/gui/window.linux.h"

#include "ccl/gui/keyevent.h"
#include "ccl/gui/help/helpmanager.h"
#include "ccl/gui/theme/visualstyle.h"
#include "ccl/gui/theme/themerenderer.h"
#include "ccl/gui/windows/window.h"

#include "ccl/public/base/buffer.h"
#include "ccl/public/math/mathprimitives.h"
#include "ccl/public/gui/iparameter.h"
#include "ccl/public/gui/graphics/dpiscale.h"

using namespace CCL;
using namespace Linux;

//************************************************************************************************
// NativeTextControl
//************************************************************************************************

NativeTextControl* NativeTextControl::create (Control& owner, const Rect& clientRect, int returnKeyType, int keyboardType)
{
	return NEW LinuxTextControl (owner, clientRect, returnKeyType, keyboardType);
}

//************************************************************************************************
// LinuxTextControl
//************************************************************************************************

LinuxTextControl::LinuxTextControl (Control& owner, const Rect& clientRect, int returnKeyType, int keyboardType)
: NativeTextControl (owner, returnKeyType, keyboardType),
  editBox (owner.getWindow (), clientRect)
{
	Point position (clientRect.getLeftTop ());
	
	editBox.setPosition (owner.clientToWindow (position));
	editBox.show ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

LinuxTextControl::~LinuxTextControl ()
{
	editBox.hide ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void LinuxTextControl::updateText ()
{
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void LinuxTextControl::getControlText (String& string)
{
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void LinuxTextControl::setSelection (int start, int length)
{
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void LinuxTextControl::setScrollPosition (PointRef _where)
{
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Point LinuxTextControl::getScrollPosition () const
{
	Point where (0, 0);
	return where;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void LinuxTextControl::setSize (RectRef clientRect)
{
	Point position (clientRect.getLeftTop ());
	editBox.setPosition (owner.clientToWindow (position));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void LinuxTextControl::updateVisualStyle ()
{
}

//************************************************************************************************
// LinuxEditBox
//************************************************************************************************

LinuxEditBox::LinuxEditBox (IWindow* parent, const Rect& size)
: LinuxWindow (size),
  listener (*this),
  textInput (nullptr)
{
	style.setCustomStyle (Styles::kWindowBehaviorPopupSelector, true);
	
	makeNativePopupWindow (parent);
	
	zwp_text_input_manager_v3* manager = WaylandClient::instance ().getTextInputManager ();
	wl_seat* seat = WaylandClient::instance ().getSeat ();
	if(manager && seat)
		textInput = zwp_text_input_manager_v3_get_text_input (manager, seat);
	
	if(textInput)
	{
		zwp_text_input_v3_add_listener (textInput, &listener, &listener);
		zwp_text_input_v3_enable (textInput);
		zwp_text_input_v3_commit (textInput);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

LinuxEditBox::~LinuxEditBox ()
{
	if(textInput && WaylandClient::instance ().isInitialized ())
	{
		zwp_text_input_v3_disable (textInput);
		zwp_text_input_v3_destroy (textInput);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API LinuxEditBox::close ()
{
	if(onClose ())
	{
		hide ();
		setInCloseEvent ();
		setInDestroyEvent (true);
		
		removed (nullptr);
		onDestroy ();
		setInCloseEvent (false);
		
		return true;
	}
	return false;
}

//************************************************************************************************
// LinuxEditBox::Listener
//************************************************************************************************

LinuxEditBox::Listener::Listener (LinuxEditBox& window)
: window (window)
{
	enter = onEnter;
	leave = onLeave;
	preedit_string = onPreeditString;
	commit_string = onCommitString;
	delete_surrounding_text = onDeleteSurroundingText;
	done = onDone;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void LinuxEditBox::Listener::onEnter (void* data, zwp_text_input_v3* textInput, struct wl_surface *surface)
{
	Listener* This = static_cast<Listener*> (data);
	if(This->window.getWaylandSurface () == surface && This->window.textInput == textInput)
	{
		zwp_text_input_v3_enable (textInput);
		zwp_text_input_v3_commit (textInput);
		This->window.onFocus (FocusEvent::kSetFocus);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void LinuxEditBox::Listener::onLeave (void* data, zwp_text_input_v3* textInput, struct wl_surface *surface)
{
	Listener* This = static_cast<Listener*> (data);
	if(This->window.getWaylandSurface () == surface && This->window.textInput == textInput)
	{
		zwp_text_input_v3_disable (textInput);
		zwp_text_input_v3_commit (textInput);
		This->window.onFocus (FocusEvent::kKillFocus);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void LinuxEditBox::Listener::onPreeditString (void* data, zwp_text_input_v3* textInput, const char* text, int32_t cursorBegin, int32_t cursorEnd)
{
	
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void LinuxEditBox::Listener::onCommitString (void* data, zwp_text_input_v3* textInput, const char* text)
{
	
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void LinuxEditBox::Listener::onDeleteSurroundingText (void* data, zwp_text_input_v3* textInput, uint32_t beforeLength, uint32_t afterLength)
{
	
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void LinuxEditBox::Listener::onDone (void* data, zwp_text_input_v3* textInput, uint32_t serial)
{
	
}

