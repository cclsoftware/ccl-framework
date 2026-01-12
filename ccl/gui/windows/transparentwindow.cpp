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
// Filename    : ccl/gui/windows/transparentwindow.cpp
// Description : Transparent Window
//
//************************************************************************************************

#include "ccl/gui/windows/transparentwindow.h"
#include "ccl/gui/windows/window.h"

using namespace CCL;

//************************************************************************************************
// TransparentWindow
//************************************************************************************************

DEFINE_CLASS_HIDDEN (TransparentWindow, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

TransparentWindow::TransparentWindow (Window* _parentWindow, int _options, StringRef _title)
: parentWindow (_parentWindow),
  options (_options),
  title (_title)
{
	ASSERT (parentWindow != nullptr)

	if(parentWindow)
		parentWindow->addTransparentWindow (this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

TransparentWindow::~TransparentWindow ()
{
	if(parentWindow)
		parentWindow->removeTransparentWindow (this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Window* TransparentWindow::getParentWindow () const
{
	return parentWindow;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

float TransparentWindow::getContentScaleFactor () const
{
	return parentWindow ? parentWindow->getContentScaleFactor () : 1.f;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TransparentWindow::show ()
{
	CCL_NOT_IMPL ("TransparentWindow::show")
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TransparentWindow::hide ()
{
	CCL_NOT_IMPL ("TransparentWindow::hide")
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool TransparentWindow::isVisible () const
{
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TransparentWindow::update (RectRef size, Bitmap& bitmap, PointRef offset, float opacity)
{
	CCL_NOT_IMPL ("TransparentWindow::update")
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TransparentWindow::move (PointRef position)
{
	CCL_NOT_IMPL ("TransparentWindow::move")
}
