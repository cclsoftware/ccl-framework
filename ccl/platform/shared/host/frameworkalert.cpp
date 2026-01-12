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
// Filename    : ccl/platform/shared/host/frameworkalert.cpp
// Description : Alert box implementation using generic framework controls only
//
//************************************************************************************************

#include "ccl/platform/shared/host/frameworkalert.h"

#include "ccl/public/gui/iparameter.h"

using namespace CCL;

//////////////////////////////////////////////////////////////////////////////////////////////////
// Tags
//////////////////////////////////////////////////////////////////////////////////////////////////

namespace Tag
{
	enum FrameworkAlertTags
	{
		kFirstButton = 100,
		kSecondButton,
		kThirdButton
	};
}

//************************************************************************************************
// FrameworkAlertBox
//************************************************************************************************

DEFINE_CLASS (FrameworkAlertBox, AlertBox)

//////////////////////////////////////////////////////////////////////////////////////////////////

FrameworkAlertBox::FrameworkAlertBox ()
{
	paramList.setController (this);
	paramList.addParam ("firstButton", Tag::kFirstButton);
	paramList.addParam ("secondButton", Tag::kSecondButton);
	paramList.addParam ("thirdButton", Tag::kThirdButton);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

FrameworkAlertBox::~FrameworkAlertBox ()
{
	if(operation)
		closePlatform ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IAsyncOperation* FrameworkAlertBox::runAsyncPlatform ()
{
	Theme& theme = FrameworkTheme::instance ();
	View* view = unknown_cast<View> (theme.createView ("AlertBox", asUnknown ()));
	ASSERT (view)
	if(!view)
		return AsyncOperation::createFailed ();

	view->setTitle (getTitle ());

	dialogBuilder = NEW DialogBuilder;
	dialogBuilder->setTheme (theme);

	AsyncOperation* alertOperation = NEW AsyncOperation;
	operation = alertOperation;
	operation->setState (IAsyncOperation::kStarted);

	Promise promise (dialogBuilder->runDialogAsync (view));
	promise.then ([&] (IAsyncOperation& op)
	{
		if(operation)
		{
			if(isUsingCustomButtonResults ())
				operation->setResult (Alert::kEscapePressed);
			else
				operation->setResult (Alert::kCancel);
			operation->setStateDeferred (IAsyncOperation::kCompleted);
			operation.release ();
		}
	});

	return alertOperation;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void FrameworkAlertBox::closePlatform ()
{
	if(dialogBuilder)
	{
		dialogBuilder->close ();
		dialogBuilder.release ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API FrameworkAlertBox::getProperty (Variant& var, MemberID propertyId) const
{
	if(propertyId == "text")
	{
		var.fromString (getText ());
		return true;
	}
	else if(propertyId == "firstButton")
	{
		var.fromString (getFirstButton ());
		return true;
	}
	else if(propertyId == "secondButton")
	{
		var.fromString (getSecondButton ());
		return true;
	}
	else if(propertyId == "thirdButton")
	{
		var.fromString (getThirdButton ());
		return true;
	}
	else if(propertyId == "hasFirstButton")
	{
		var = !getFirstButton ().isEmpty ();
		return true;
	}
	else if(propertyId == "hasSecondButton")
	{
		var = !getSecondButton ().isEmpty ();
		return true;
	}
	else if(propertyId == "hasThirdButton")
	{
		var = !getThirdButton ().isEmpty ();
		return true;
	}
	return SuperClass::getProperty (var, propertyId);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API FrameworkAlertBox::paramChanged (IParameter* param)
{
	if(param == nullptr)
		return false;
	
	int buttonIndex = -1;
	switch(param->getTag ())
	{
	case Tag::kFirstButton :
		buttonIndex = 0;
		break;
	case Tag::kSecondButton :
		buttonIndex = 1;
		break;
	case Tag::kThirdButton :
		buttonIndex = 2;
		break;
	default :
		return false;
	}

	operation->setResult (getButtonResult (buttonIndex));
	operation->setStateDeferred (IAsyncOperation::kCompleted);
	operation.release ();

	closePlatform ();
	
	return true;
}
