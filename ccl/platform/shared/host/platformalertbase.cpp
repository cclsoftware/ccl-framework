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
// Filename    : ccl/platform/shared/host/platformalertbase.cpp
// Description : Platform Alert Dialog
//
//************************************************************************************************

#include "ccl/platform/shared/host/platformalertbase.h"

#include "ccl/public/cclversion.h"

using namespace CCL;
using namespace PlatformIntegration;

//************************************************************************************************
// Alert::ButtonMapping
//************************************************************************************************

int Alert::ButtonMapping::getResultAtButtonIndex (int buttonIndex) const
{
	switch(buttonIndex)
	{
	case 0 : return defaultResult;
	case 1 : return alternateResult;
	case 2 : return otherResult;
	}
	return kUndefined;
}

//************************************************************************************************
// PlatformAlertBoxBase
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (PlatformAlertBoxBase, AlertBox)

//////////////////////////////////////////////////////////////////////////////////////////////////

PlatformAlertBoxBase::PlatformAlertBoxBase ()
: platformAlert (CCLGUI_PACKAGE_ID)
{
	platformAlert.load ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PlatformAlertBoxBase::closePlatform ()
{
	if(platformAlert)
		platformAlert->close ();
	SuperClass::closePlatform ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IAsyncOperation* PlatformAlertBoxBase::runAsyncPlatform ()
{
	if(platformAlert == nullptr)
		return SuperClass::runAsyncPlatform ();
	
	MutableCString title (getTitle (), Text::kUTF8);
	MutableCString text (getText (), Text::kUTF8);
	MutableCString firstButton (getFirstButton (), Text::kUTF8);
	MutableCString secondButton (getSecondButton (), Text::kUTF8);
	MutableCString thirdButton (getThirdButton (), Text::kUTF8);
	
	int alertType = Alert::kUndefined;
	switch(getAlertType ())
	{
	case Alert::kInformation :
		alertType = PlatformIntegration::IPlatformAlert::kInfo;
		break;
	case Alert::kWarning :
		alertType = PlatformIntegration::IPlatformAlert::kWarning;
		break;
	case Alert::kError :
		alertType = PlatformIntegration::IPlatformAlert::kError;
		break;
	}
	if(!platformAlert->open (*this, title, text, alertType, firstButton, secondButton, thirdButton))
		return nullptr;
		
	operation = NEW AsyncOperation;
	operation->setState (IAsyncOperation::kStarted);
	
    return operation;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PlatformAlertBoxBase::opened (void* nativeWindowHandle)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PlatformAlertBoxBase::closed (int result)
{
	if(operation)
	{
		if(result == PlatformIntegration::IPlatformDialogObserver::kCanceled)
		{
			if(isUsingCustomButtonResults ())
				operation->setResult (Alert::kEscapePressed);
			else
				operation->setResult (Alert::kCancel);
		}
		else
		{
			operation->setResult (getButtonResult (result));
		}
		operation->setStateDeferred (IAsyncOperation::kCompleted);
	}
	operation.release ();	
}
