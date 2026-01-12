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
// Filename    : ccl/gui/windows/childwindow.cpp
// Description : Child Window class
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "ccl/gui/windows/childwindow.h"

#include "ccl/base/message.h"
#include "ccl/public/guiservices.h"

using namespace CCL;

//////////////////////////////////////////////////////////////////////////////////////////////////
// GUI Service APIs
//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_EXPORT IWindow* CCL_API System::CCL_ISOLATED (CreateChildWindow) (IView* _view, void* nativeParent)
{
	CCL::View* view = unknown_cast<View> (_view);
	if(view == nullptr)
		return nullptr;

	ChildWindow* window = NEW ChildWindow (nativeParent, Window::kWindowModeEmbedding, view->getSize ());

	window->setTheme (const_cast<Theme*> (&view->getTheme ()));

	if(!window->addView (view))
	{
		window->release ();
		return nullptr;
	}

	if(view->hasVisualStyle ())
		window->setVisualStyle (unknown_cast<VisualStyle> (&view->getVisualStyle ()));

	view->retain ();

	return window;
}

//************************************************************************************************
// ChildWindow
//************************************************************************************************

DEFINE_CLASS_HIDDEN (ChildWindow, NativeWindow)

//////////////////////////////////////////////////////////////////////////////////////////////////

ChildWindow::ChildWindow (void* nativeParent, WindowMode mode, const Rect& size, StyleRef style, StringRef title)
: NativeWindow (size, style, title)
{
	ASSERT (mode != Window::kWindowModeRegular)
	windowMode = mode;
	makeNativeWindow (nativeParent);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ChildWindow::ChildWindow (WindowMode mode, const Rect& size, StyleRef style, StringRef title)
: NativeWindow (size, style, title)
{
	ASSERT (mode != Window::kWindowModeRegular);
	windowMode = mode;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ChildWindow::makeNativeWindow (void* nativeParent)
{
	makeNativeChildWindow (nativeParent);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ChildWindow::onSize (const Point& delta)
{
	CCL_PRINTF ("ChildWindow::onSize (%d, %d)  -> %d,%d\n", delta.x, delta.y, size.getWidth (), size.getHeight ())

	SuperClass::onSize (delta);
	signal (Message (kSizeChanged));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ChildWindow::onActivate (bool state)
{
	CCL_ADD_INDENT (2)
	CCL_PRINTF ("%sChildWindow::onActivate (%s)\n", CCL_INDENT, state ? "true" : "false");

	SuperClass::onActivate (state);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ChildWindow::onKeyDown (const KeyEvent& event)
{
	if(View* view = getFirst ())
		if(view->onKeyDown (event))
			return true;

	return SuperClass::onKeyDown (event);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ChildWindow::onKeyUp (const KeyEvent& event)
{
	if(View* view = getFirst ())
		if(view->onKeyUp (event))
			return true;

	return SuperClass::onKeyUp (event);
}
