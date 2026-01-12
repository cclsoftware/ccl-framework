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
// Filename    : winusbhidmanager.h
// Description : Windows USB HID Manager
//
//************************************************************************************************

#ifndef _winusbhidmanager_h
#define _winusbhidmanager_h

#include "../shared/hidintegration.h"

namespace Core {
class WinDeviceNotificationHandler; }

namespace CCL {
namespace Usb {

//************************************************************************************************
// WindowsUsbHidManager
//************************************************************************************************

class WindowsUsbHidManager: public HidApiDeviceManager
{
public:
	DECLARE_CLASS (WindowsUsbHidManager, HidApiDeviceManager)

	WindowsUsbHidManager ();
	~WindowsUsbHidManager ();

	// HidApiDeviceManager
	Core::ErrorCode registerObserver (IUsbHidObserver* observer, const UsbIdFilter& filter) override;
	Core::ErrorCode unregisterObserver (IUsbHidObserver* observer) override;
	void CCL_API notify (ISubject* subject, MessageRef msg) override;

protected:
	DECLARE_STRINGID_MEMBER (kEnumerate)

	static constexpr int kEnumerationRetries = 3;
	static constexpr int kEnumerationDelayMs = 10;

	Core::WinDeviceNotificationHandler* notificationHandler;
	Vector<InternalUsbDeviceInfo> deviceInfos;

	struct RegisteredObserver
	{
		IUsbHidObserver* observer = nullptr;
		UsbIdFilter filter;
		Vector<InternalUsbDeviceInfo*> knownDeviceInfos;
	};

	Vector<RegisteredObserver> registeredObservers;

	void enableNotifications (bool state);
	void enumerate (int retries);
	void onDevicesChanged ();
};

} // namespace Usb
} // namespace CCL

#endif // _winusbhidmanager_h
