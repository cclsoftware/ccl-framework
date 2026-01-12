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
// Filename    : ccl/public/gui/framework/ialert.cpp
// Description : Alert Interface
//
//************************************************************************************************

#include "ccl/public/gui/framework/ialert.h"

#include "ccl/public/system/ierrorhandler.h"

#include "ccl/public/systemservices.h"
#include "ccl/public/plugservices.h"
#include "ccl/public/guiservices.h"

namespace CCL {

namespace Alert
{
	static int runWithTypeModal (StringRef text, AlertType type)
	{
		AutoPtr<IAlertBox> alert = ccl_new<IAlertBox> (ClassID::AlertBox);
		alert->initWithType (text, type);
		return alert->run ();
	}

	static IAsyncOperation* runWithTypeAsync (StringRef text, AlertType type)
	{
		AutoPtr<IAlertBox> alert = ccl_new<IAlertBox> (ClassID::AlertBox);
		alert->initWithType (text, type);
		return alert->runAsync ();
	}
}

} // namespace CCL

using namespace CCL;

//************************************************************************************************
// Alert
//************************************************************************************************

void Alert::warn (StringRef text)
{
	runWithTypeModal (text, kWarning);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IAsyncOperation* Alert::warnAsync (StringRef text)
{
	return runWithTypeAsync (text, kWarning);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Alert::info (StringRef text)
{
	runWithTypeModal (text, kInformation);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IAsyncOperation* Alert::infoAsync (StringRef text)
{
	return runWithTypeAsync (text, kInformation);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Alert::error (StringRef text)
{
	runWithTypeModal (text, kError);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IAsyncOperation* Alert::errorAsync (StringRef text)
{
	return runWithTypeAsync (text, kError);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int Alert::ask (StringRef text, int type)
{
	AutoPtr<IAlertBox> alert = ccl_new<IAlertBox> (ClassID::AlertBox);
	alert->initWithQuestion (text, type);
	return alert->run ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IAsyncOperation* Alert::askAsync (StringRef text, int type)
{
	AutoPtr<IAlertBox> alert = ccl_new<IAlertBox> (ClassID::AlertBox);
	alert->initWithQuestion (text, type);
	return alert->runAsync ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int Alert::ask (StringRef text, StringRef firstButton, StringRef secondButton, StringRef thirdButton)
{
	AutoPtr<IAlertBox> alert = ccl_new<IAlertBox> (ClassID::AlertBox);
	alert->initWithButtons (text, firstButton, secondButton, thirdButton);
	return alert->run ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IAsyncOperation* Alert::askAsync (StringRef text, StringRef firstButton, StringRef secondButton, StringRef thirdButton)
{
	AutoPtr<IAlertBox> alert = ccl_new<IAlertBox> (ClassID::AlertBox);
	alert->initWithButtons (text, firstButton, secondButton, thirdButton);
	return alert->runAsync ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Alert::notify (StringRef text, int type)
{
	return System::GetAlertService ().showNotification (text, type) != 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Alert::IReporter& Alert::getReporter (bool silent)
{
	if(silent)
		return System::GetErrorHandler ();
	else
		return System::GetAlertService ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Alert::errorWithContext (StringRef text, bool forceDialog)
{
	// suppress dialog if multiple error contexts are nested
	if(forceDialog == false)
		if(System::GetErrorHandler ().getContextDepth () > 1)
			return;
			
	IErrorContext* context = System::GetErrorHandler ().peekContext (); // can be null
	AutoPtr<IAlertBox> alert = ccl_new<IAlertBox> (ClassID::AlertBox);
	alert->initWithContext (text, context);
	alert->run ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IAsyncOperation* Alert::errorWithContextAsync (StringRef text,  bool forceDialog)
{
	// suppress dialog if multiple error contexts are nested
	if(forceDialog == false)
		if(System::GetErrorHandler ().getContextDepth () > 1)
			return nullptr;

	IErrorContext* context = System::GetErrorHandler ().peekContext (); // can be null
	AutoPtr<IAlertBox> alert = ccl_new<IAlertBox> (ClassID::AlertBox);
	alert->initWithContext (text, context);
	return alert->runAsync ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int Alert::askWithContext (StringRef text, int type)
{
	IErrorContext* context = System::GetErrorHandler ().peekContext (); // can be null
	AutoPtr<IAlertBox> alert = ccl_new<IAlertBox> (ClassID::AlertBox);
	alert->initWithContext (text, context, type);
	return alert->run ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IAsyncOperation* Alert::askWithContextAsync (StringRef text, int type)
{
	IErrorContext* context = System::GetErrorHandler ().peekContext (); // can be null
	AutoPtr<IAlertBox> alert = ccl_new<IAlertBox> (ClassID::AlertBox);
	alert->initWithContext (text, context, type);
	return alert->runAsync ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringRef Alert::button (Alert::StandardResult result)
{
	return System::GetAlertService ().getButtonTitle (result);
}
