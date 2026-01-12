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
// Filename    : ccl/gui/windows/dialog.cpp
// Description : Dialog Window
//
//************************************************************************************************

#include "ccl/gui/windows/dialog.h"
#include "ccl/gui/windows/desktop.h"
#include "ccl/gui/views/focusnavigator.h"
#include "ccl/gui/system/dragndrop.h"
#include "ccl/gui/gui.h"

#include "ccl/base/asyncoperation.h"

using namespace CCL;

//************************************************************************************************
// Dialog
//************************************************************************************************

BEGIN_STYLEDEF (Dialog::dialogButtons)
	{"cancel", Styles::kCancelButton},
	{"okay",   Styles::kOkayButton},
	{"close",  Styles::kCloseButton},
	{"apply",  Styles::kApplyButton},
END_STYLEDEF

//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_CLASS_HIDDEN (Dialog, NativeDialog)

//////////////////////////////////////////////////////////////////////////////////////////////////

Dialog::Dialog (const Rect& size, StyleRef style, StringRef title)
: NativeDialog (size, style, title),
  dialogResult (DialogResult::kNone)
{
	layer = kDialogLayer;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Dialog::setFirstFocusView (IView* view)
{
	firstFocusView = view;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Dialog::initFocusView ()
{
	if(!firstFocusView)
		firstFocusView = FocusNavigator::instance ().getFirstExplicit (this);

	if(firstFocusView)
	{
		firstFocusView->takeFocus (false);
		firstFocusView = nullptr;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Dialog::isCanceled () const
{
	return dialogResult == DialogResult::kCancel;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int Dialog::showModal (IWindow* parentWindow)
{
	Promise promise (showDialog (parentWindow));
	while(promise->getState () == AsyncOperation::kStarted)
		GUI.flushUpdates ();

	return promise->getResult ().asInt ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IAsyncOperation* Dialog::showDialog (IWindow* parentWindow)
{
	#if DEBUG
	if(DragSession::getActiveSession ())
		Debugger::println ("WARNING: Drag'n'Drop still active when opening Dialog. Should be deferred!");
	#endif

	if(!parentWindow)
		parentWindow = Desktop.getDialogParentWindow ();

	Desktop.addWindow (this, layer);
	GUI.hideTooltip ();

	dialogResult = DialogResult::kNone;
	return showPlatformDialog (parentWindow);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Dialog::onKeyDown (const KeyEvent& event)
{
	#if DEBUG_LOG
	String s;
	event.toString (s);
	CCL_PRINTF ("Dialog::onKeyDown:%s\n", MutableCString (s).str ())
	#endif

	if(event.vKey == VKey::kReturn
		|| event.vKey == VKey::kEnter
		|| event.vKey == VKey::kEscape)
	{
		// don't close dialog during drag session
		if(GUI.isDragActive ())
			return true;

		// give focus view a chance to swallow key
		if(focusView && focusView->onKeyDown (event))
			return true;

		dialogResult = (event.vKey == VKey::kEscape) ? DialogResult::kCancel : DialogResult::kOkay;
		close ();
		return true;
	}
	return SuperClass::onKeyDown (event);
}
