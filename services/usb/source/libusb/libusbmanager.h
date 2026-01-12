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
// Filename    : libusbmanager.h
// Description : USB hotplug support
//
//************************************************************************************************

#ifndef _libusbmanager_h
#define _libusbmanager_h

#include "../shared/hidintegration.h"

#include "ccl/public/collections/vector.h"

struct libusb_context;

namespace CCL {
namespace Usb {

class UsbEventListener;

//************************************************************************************************
// LibUsbDeviceManager
//************************************************************************************************

class LibUsbDeviceManager: public HidApiDeviceManager
{
public:
	DECLARE_CLASS (LibUsbDeviceManager, HidApiDeviceManager)

	// IUsbHidManager
	Core::ErrorCode startup () override;
	Core::ErrorCode shutdown () override;
	Core::ErrorCode registerObserver (IUsbHidObserver* observer, const UsbIdFilter& filter) override;
	Core::ErrorCode unregisterObserver (IUsbHidObserver* observer) override;

protected:
	class HotplugObserver: public Object
	{
	public:
		HotplugObserver (IUsbHidObserver* observer, int hotplugHandle, ::libusb_context* context);
		~HotplugObserver ();

		bool matches (IUsbHidObserver* observer) const;

	private:
		IUsbHidObserver* observer;
		int hotplugHandle;
		::libusb_context* context;
	};

	Vector<AutoPtr<HotplugObserver>> observers;
	::libusb_context* libUsbContext = nullptr;
	UsbEventListener* eventListener = nullptr;
};

} // namespace Usb
} // namespace CCL

#endif // _libusbmanager_h
