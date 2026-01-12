//************************************************************************************************
//
// USB Support
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
// Filename    : winusbhidmanager.cpp
// Description : Windows USB HID Manager
//
//************************************************************************************************

#include "winusbhidmanager.h"

#include "ccl/base/message.h"

#include "ccl/public/base/ccldefpush.h"
#define INITGUID
#include "core/platform/win/windevicenotificationhandler.h"
#include <Usbiodef.h>
#include "ccl/public/base/ccldefpop.h"

using namespace Core;
using namespace Core::Errors;
using namespace CCL;
using namespace CCL::Usb;

//************************************************************************************************
// WindowsUsbHidManager
//************************************************************************************************

DEFINE_CLASS_HIDDEN (WindowsUsbHidManager, HidApiDeviceManager)
DEFINE_STRINGID_MEMBER_ (WindowsUsbHidManager, kEnumerate, "enumerate")

//////////////////////////////////////////////////////////////////////////////////////////////////

WindowsUsbHidManager::WindowsUsbHidManager ()
: notificationHandler (nullptr)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

WindowsUsbHidManager::~WindowsUsbHidManager ()
{
	ASSERT (notificationHandler == nullptr)
	cancelSignals ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ErrorCode WindowsUsbHidManager::registerObserver (IUsbHidObserver* observer, const UsbIdFilter& filter)
{
	ErrorCode result = SuperClass::registerObserver (observer, filter);
	if(result != kError_NoError)
		return result;

	bool startNotificationHandler = registeredObservers.isEmpty ();
	registeredObservers.add ({observer, filter, {}});

	if(startNotificationHandler)
		enableNotifications (true);

	enumerate (0);
	return kError_NoError;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ErrorCode WindowsUsbHidManager::unregisterObserver (IUsbHidObserver* observer)
{
	ErrorCode result = SuperClass::unregisterObserver (observer);
	if(result != kError_NoError)
		return result;

	for(int i = 0; i < registeredObservers.count (); i++)
	{
		if(registeredObservers[i].observer == observer)
		{
			registeredObservers.removeAt (i);
			if(registeredObservers.isEmpty ())
				enableNotifications (false);
			return kError_NoError;
		}
	}

	return kError_ItemNotFound;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API WindowsUsbHidManager::notify (ISubject* subject, MessageRef msg)
{
	if(msg == kEnumerate)
		enumerate (msg.getArg (0));
	else
		SuperClass::notify (subject, msg);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WindowsUsbHidManager::enableNotifications (bool state)
{
	struct CB
	{
		static void onDevicesChanged (void* context, bool devicesRemoved)
		{
			static_cast<WindowsUsbHidManager*> (context)->onDevicesChanged ();
		}
	};

	if(state)
	{
		if(!notificationHandler)
		{
			notificationHandler = NEW WinDeviceNotificationHandler (CB::onDevicesChanged, this);
			notificationHandler->registerNotification (GUID_DEVINTERFACE_USB_DEVICE);
		}
	}
	else
	{
		if(notificationHandler)
		{
			notificationHandler->unregisterNotifications ();
			delete notificationHandler;
			notificationHandler = nullptr;
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WindowsUsbHidManager::enumerate (int retries)
{
	Vector<UsbIdPair> mergedFilter;
	for(const RegisteredObserver& registeredObserver : registeredObservers)
	{
		for(int idIndex = 0; idIndex < registeredObserver.filter.numIds; idIndex++)
		{
			const UsbIdPair& idPair = registeredObserver.filter.ids[idIndex];
			if(!UsbIdFilter {mergedFilter.getItems (), mergedFilter.count ()}.contains (idPair))
				mergedFilter.add (idPair);
		}
	}

	Vector<InternalUsbDeviceInfo> newDeviceInfos;
	for(int i = 0; i < mergedFilter.count (); i++)
		SuperClass::enumerate (newDeviceInfos, mergedFilter[i]);

	bool changed = false;

	// remove lost devices
	for(const InternalUsbDeviceInfo& info : deviceInfos)
	{
		if(!newDeviceInfos.contains (info))
		{
			changed = true;
			for(RegisteredObserver& registeredObserver : registeredObservers)
				if(registeredObserver.filter.contains (info.ids))
					registeredObserver.observer->onDeviceRemoved ({info.ids.vendorId, info.ids.productId, info.serialNumberString});
		}
	}

	// add found devices
	for(const InternalUsbDeviceInfo& newInfo : newDeviceInfos)
	{
		if(!deviceInfos.contains (newInfo))
		{
			changed = true;
			for(RegisteredObserver& registeredObserver : registeredObservers)
				if(registeredObserver.filter.contains (newInfo.ids))
					registeredObserver.observer->onDeviceAdded ({newInfo.ids.vendorId, newInfo.ids.productId, newInfo.serialNumberString});
		}
	}

	if(changed)
		deviceInfos = newDeviceInfos;

	// if the device list has not changed, but we expected a change (because we
	// received a notification), try again
	else if(retries > 0)
		(NEW Message (kEnumerate, retries - 1))->post (this, kEnumerationDelayMs);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WindowsUsbHidManager::onDevicesChanged ()
{
	// When receiving a notification, the hidapi doesn't return the updated list
	// yet. Therefore, we need to delay the device enumeration
	(NEW Message (kEnumerate, kEnumerationRetries))->post (this, kEnumerationDelayMs);
}
