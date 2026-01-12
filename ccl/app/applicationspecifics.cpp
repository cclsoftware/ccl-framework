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
// Filename    : ccl/app/applicationspecifics.cpp
// Description : Application Specifics
//
//************************************************************************************************

#include "ccl/app/applicationspecifics.h"

#include "ccl/public/gui/framework/ialert.h"
#include "ccl/public/gui/framework/itheme.h"
#include "ccl/public/gui/framework/iuserinterface.h"
#include "ccl/public/guiservices.h"

#include "ccl/public/system/ifilesystemsecuritystore.h"
#include "ccl/public/systemservices.h"
#include "ccl/public/plugservices.h"

using namespace CCL;

//************************************************************************************************
// ApplicationSpecifics
//************************************************************************************************

ApplicationSpecifics* ApplicationSpecifics::createInstance ()
{
	#if CCL_PLATFORM_WINDOWS
	return NEW Win32ApplicationSpecifics;
	#elif CCL_PLATFORM_MAC
	return NEW MacOSApplicationSpecifics;
	#elif CCL_PLATFORM_IOS
	return NEW IosApplicationSpecifics;
	#elif CCL_PLATFORM_LINUX
	return NEW LinuxApplicationSpecifics;
	#else
	return NEW ApplicationSpecifics;
	#endif
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_CLASS_HIDDEN (ApplicationSpecifics, Component)
DEFINE_STRINGID_MEMBER_ (ApplicationSpecifics, kAppNotifyIcon, "AppNotifyIcon")

//////////////////////////////////////////////////////////////////////////////////////////////////

ApplicationSpecifics::ApplicationSpecifics ()
: Component (String ("ApplicationSpecifics")),
  notifyIcon (nullptr)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

ApplicationSpecifics::~ApplicationSpecifics ()
{
	ASSERT (notifyIcon == nullptr)
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API ApplicationSpecifics::initialize (IUnknown* context)
{
	return SuperClass::initialize (context);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API ApplicationSpecifics::terminate ()
{
	enableNotifyIcon (false, false);
	return SuperClass::terminate ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ApplicationSpecifics::enableNotifyIcon (bool state, bool autoShow)
{
	bool enabled = notifyIcon != nullptr;
	if(state != enabled)
	{
		if(state)
		{
			notifyIcon = ccl_new<INotifyIcon> (CCL::ClassID::NotifyIcon);
			if(notifyIcon)
			{
				auto& root = RootComponent::instance ();
				notifyIcon->setTitle (root.getApplicationTitle ());
				notifyIcon->setImage (getTheme ()->getImage (kAppNotifyIcon)); // init icon from skin

				if(autoShow)
					notifyIcon->setAutoShow (true);
				else
					notifyIcon->setVisible (true);

				System::GetAlertService ().setNotificationReporter (notifyIcon);
			}
		}
		else
		{
			if(notifyIcon)
				System::GetAlertService ().setNotificationReporter (nullptr),
				notifyIcon->release (),
				notifyIcon = nullptr;
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

INotifyIcon* ApplicationSpecifics::getNotifyIcon () const
{
	return notifyIcon;
}

//************************************************************************************************
// Win32ApplicationSpecifics
//************************************************************************************************

DEFINE_CLASS_HIDDEN (Win32ApplicationSpecifics, ApplicationSpecifics)

//////////////////////////////////////////////////////////////////////////////////////////////////

Win32ApplicationSpecifics::Win32ApplicationSpecifics ()
: taskBar (nullptr)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

Win32ApplicationSpecifics::~Win32ApplicationSpecifics ()
{	
	safe_release (taskBar);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Win32::ITaskBar& Win32ApplicationSpecifics::getTaskBar ()
{
	if(taskBar == nullptr)
	{
		taskBar = ccl_new<Win32::ITaskBar> (Win32::ClassID::TaskBar);
		ASSERT (taskBar != nullptr)
	}
	return *taskBar;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API Win32ApplicationSpecifics::initialize (IUnknown* context)
{
	return SuperClass::initialize (context);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API Win32ApplicationSpecifics::terminate ()
{
	enableTaskBarProgressIndicator (nullptr, false);

	return SuperClass::terminate ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Win32ApplicationSpecifics::enableTaskBarProgressIndicator (IWindow* window, bool state)
{
	if(state)
	{
		ASSERT (window != nullptr)
		taskBarProgressIndicator = getTaskBar ().getProgressBar (window); // can return null!
		if(taskBarProgressIndicator)
			System::GetAlertService ().setProgressReporter (taskBarProgressIndicator, true);
	}
	else
	{
		if(taskBarProgressIndicator)
			System::GetAlertService ().setProgressReporter (taskBarProgressIndicator, false);
		taskBarProgressIndicator.release ();
	}
}

//************************************************************************************************
// MacOSApplicationSpecifics
//************************************************************************************************

DEFINE_CLASS_HIDDEN (MacOSApplicationSpecifics, ApplicationSpecifics)

//////////////////////////////////////////////////////////////////////////////////////////////////

MacOSApplicationSpecifics::MacOSApplicationSpecifics ()
{
	System::GetFileSystemSecurityStore ().loadSecurityData ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

MacOSApplicationSpecifics::~MacOSApplicationSpecifics ()
{
	System::GetFileSystemSecurityStore ().saveSecurityData ();
}

//************************************************************************************************
// IosApplicationSpecifics
//************************************************************************************************

DEFINE_CLASS_HIDDEN (IosApplicationSpecifics, ApplicationSpecifics)

//////////////////////////////////////////////////////////////////////////////////////////////////

IosApplicationSpecifics::IosApplicationSpecifics ()
{
	System::GetFileSystemSecurityStore ().loadSecurityData ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IosApplicationSpecifics::~IosApplicationSpecifics ()
{
	System::GetFileSystemSecurityStore ().saveSecurityData ();
}

//************************************************************************************************
// LinuxApplicationSpecifics
//************************************************************************************************

DEFINE_CLASS_HIDDEN (LinuxApplicationSpecifics, ApplicationSpecifics)
