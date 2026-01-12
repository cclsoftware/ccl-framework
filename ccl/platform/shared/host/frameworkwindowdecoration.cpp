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
// Filename    : ccl/platform/shared/host/frameworkwindowdecoration.cpp
// Description : Window decoration using generic framework controls only
//
//************************************************************************************************

#include "ccl/platform/shared/host/frameworkwindowdecoration.h"

#include "ccl/gui/gui.h"

#include "ccl/public/gui/iparameter.h"

using namespace CCL;

//////////////////////////////////////////////////////////////////////////////////////////////////
// Tags
//////////////////////////////////////////////////////////////////////////////////////////////////

namespace Tag
{
	enum WindowDecorationControllerTags
	{
		kWindowActive = 100,
		kMinimize,
		kMaximize,
		kClose,
		kTitle,
		kIcon
	};
}

//************************************************************************************************
// WindowDecorationController
//************************************************************************************************

DEFINE_CLASS_HIDDEN (WindowDecorationController, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

WindowDecorationController::WindowDecorationController ()
: targetWindow (nullptr),
  titleBarHeight (24),
  borderWidth (2),
  flags (0)
{
	canMinimize (false);
	canMaximize (true);
	canClose (true);
	
	paramList.setController (this);
	paramList.addParam ("active", Tag::kWindowActive);
	paramList.addParam ("minimize", Tag::kMinimize);
	paramList.addParam ("maximize", Tag::kMaximize);
	paramList.addParam ("close", Tag::kClose);
	paramList.addString ("title", Tag::kTitle);
	paramList.addImage ("icon", Tag::kIcon);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WindowDecorationController::attach (Window* newWindow)
{
	if(targetWindow)
		targetWindow->removeHandler (this);
	decorationView.release ();
	
	targetWindow = newWindow;
	if(targetWindow)
	{
		targetWindow->addHandler (this);
		
		Theme& theme = FrameworkTheme::instance ();
		decorationView = unknown_cast<View> (theme.createView ("WindowDecoration", asUnknown ()));
		
		updateDecoration ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

View* WindowDecorationController::getDecorationView ()
{
	return decorationView;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WindowDecorationController::updateDecoration ()
{
	if(targetWindow)
	{
		if(IParameter* param = paramList.byTag (Tag::kTitle))
			param->setValue (targetWindow->getTitle ());
		
		if(IParameter* param = paramList.byTag (Tag::kMaximize))
			param->setValue (targetWindow->isMaximized ());
	}
	deferChanged ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WindowDecorationController::setIcon (IImage* icon)
{
	UnknownPtr<IImageProvider> iconProvider (paramList.byTag (Tag::kIcon));
	if(iconProvider)
		iconProvider->setImage (icon);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API WindowDecorationController::onWindowEvent (WindowEvent& windowEvent)
{
	switch(windowEvent.eventType)
	{
	case WindowEvent::kActivate :
	case WindowEvent::kDeactivate :
		if(IParameter* param = paramList.byTag (Tag::kWindowActive))
			param->setValue (targetWindow->isActive ());
		return true;
		
	case WindowEvent::kMaximize :
	case WindowEvent::kUnmaximize :
		if(IParameter* param = paramList.byTag (Tag::kMaximize))
			param->setValue (targetWindow->isMaximized ());
		return true;
		
	case WindowEvent::kClose :
		targetWindow->removeHandler (this);
		targetWindow = nullptr;
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API WindowDecorationController::getProperty (Variant& var, MemberID propertyId) const
{
	if(propertyId == "titleBarHeight")
	{
		var = getTitleBarHeight ();
		return true;
	}
	else if(propertyId == "hasTitleBar")
	{
		var = getTitleBarHeight () > 0;
		return true;
	}
	else if(propertyId == "canMinimize")
	{
		var = canMinimize ();
		return true;
	}
	else if(propertyId == "canMaximize")
	{
		var = canMaximize ();
		return true;
	}
	else if(propertyId == "canClose")
	{
		var = canClose ();
		return true;
	}
	return SuperClass::getProperty (var, propertyId);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API WindowDecorationController::invokeMethod (Variant& returnValue, MessageRef msg)
{
	if(msg == "moveWindow")
	{
		onMoveWindow ();
		return true;
	}
	else if(msg == "showMenu")
	{
		onShowMenu ();
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API WindowDecorationController::paramChanged (IParameter* param)
{
	if(param == nullptr)
		return false;
	
	switch(param->getTag ())
	{
	case Tag::kMinimize :
		onMinimize ();
		return true;
	case Tag::kMaximize :
		onMaximize (param->getValue ().asBool ());
		return true;
	case Tag::kClose :
		onClose ();
		return true;
	default :
		return false;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WindowDecorationController::onMinimize ()
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WindowDecorationController::onMaximize (bool state)
{
	if(targetWindow)
		targetWindow->maximize (state);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WindowDecorationController::onClose ()
{
	if(targetWindow)
		targetWindow->deferClose ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WindowDecorationController::onMoveWindow ()
{
	if(targetWindow)
		targetWindow->moveWindow ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WindowDecorationController::onShowMenu ()
{}
