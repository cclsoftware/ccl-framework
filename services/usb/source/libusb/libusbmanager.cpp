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
// Filename    : libusbmanager.cpp
// Description : USB hotplug support
//
//************************************************************************************************

#include "libusbmanager.h"

#include "core/public/corestringbuffer.h"

#include "ccl/public/systemservices.h"
#include "ccl/public/gui/framework/idleclient.h"
#include "ccl/public/collections/linkedlist.h"

#include "ccl/public/base/ccldefpush.h"
#include "libusb.h"
#include "ccl/public/base/ccldefpop.h"

using namespace Core;
using namespace Core::Errors;

using namespace CCL;
using namespace CCL::Usb;

namespace CCL {
namespace Usb {

//************************************************************************************************
// HotplugEvent
//************************************************************************************************

struct HotplugEvent
{
	::libusb_context* context;
	::libusb_device* device;
	::libusb_hotplug_event type;
	void* userData;
	::libusb_device_descriptor descriptor;
};

//////////////////////////////////////////////////////////////////////////////////////////////////

static LinkedList<HotplugEvent> events;

//////////////////////////////////////////////////////////////////////////////////////////////////

static int hotplugCallback (::libusb_context* ctx, ::libusb_device* dev, ::libusb_hotplug_event event, void* userData)
{
	::libusb_device_descriptor descriptor;
	int returnCode = ::libusb_get_device_descriptor (dev, &descriptor);
	ASSERT (returnCode == ::LIBUSB_SUCCESS)

	ASSERT (System::IsInMainThread ())
	events.append ( {ctx, dev, event, userData, descriptor} ); // cannot call functions like ::libusb_get_string_descriptor_ascii within the callback -> process events later

	return 0;
}

//************************************************************************************************
// UsbEventListener
//************************************************************************************************

class UsbEventListener: public Object,
					    public IdleClient
{
public:
	DECLARE_CLASS (UsbEventListener, Object)
	
	UsbEventListener (::libusb_context* context = nullptr);

	// IdleClient
	void onIdleTimer () override;
	
	CLASS_INTERFACE (ITimerTask, Object)
	
private:
	struct DeviceSerial
	{
		::libusb_device* device = nullptr;
		CString256 serial;

		bool operator == (const DeviceSerial& other) { return device == other.device; }
	};
	Vector<DeviceSerial> deviceSerials;

	::libusb_context* libUsbContext;
	bool inIdleEvent;

	void processEvents ();
};

} // namespace Usb
} // namespace CCL

//************************************************************************************************
// UsbEventListener
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (UsbEventListener, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

UsbEventListener::UsbEventListener (::libusb_context* context)
: libUsbContext (context),
  inIdleEvent (false)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void UsbEventListener::onIdleTimer ()
{
	if(!inIdleEvent)
	{
		ScopedVar<bool> guard (inIdleEvent, true);

		::timeval zeroTimeval = {0};
		::libusb_handle_events_timeout_completed (libUsbContext, &zeroTimeval, nullptr);
		processEvents ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void UsbEventListener::processEvents ()
{
	ListIterator<HotplugEvent> iter = ListIterator<HotplugEvent> (events);
	while(!iter.done ())
	{
		const HotplugEvent& event = iter.next ();
		if(event.userData == nullptr)
			return;

		IUsbHidObserver* observer = static_cast<IUsbHidObserver*> (event.userData);

		CString256 serial;
		if(event.type == ::LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED)
		{
			::libusb_device_handle* handle = nullptr;
			int returnCode = ::libusb_open (event.device, &handle);
			if(returnCode != ::LIBUSB_SUCCESS)
				return;

			returnCode = ::libusb_get_string_descriptor_ascii (handle, event.descriptor.iSerialNumber, reinterpret_cast<unsigned char*> (serial.getBuffer ()), serial.getSize ());
			ASSERT (returnCode > 0)
			::libusb_close (handle);
			deviceSerials.add ({event.device, serial});
		}
		else if(event.type == ::LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT)
		{
			VectorForEachFast (deviceSerials, DeviceSerial, deviceSerial)
			if(deviceSerial.device == event.device)
			{
				serial = deviceSerial.serial;
				deviceSerials.remove (deviceSerial);
				break;
			}
			EndFor
		}
		ASSERT (!serial.isEmpty ())

		UsbDeviceInfo info;
		info.vendorId = event.descriptor.idVendor;
		info.productId = event.descriptor.idProduct;
		info.serialNumber = serial;

		ASSERT (System::IsInMainThread ())
		if(event.type == ::LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED)
		{
			System::ThreadSleep (50); // allow some time for the device to be fully registered in the system
			observer->onDeviceAdded (info);
		}
		else if(event.type == ::LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT)
			observer->onDeviceRemoved (info);
	}
	events.removeAll ();
}

//************************************************************************************************
// LibUsbDeviceManager::HotplugObserver
//************************************************************************************************

LibUsbDeviceManager::HotplugObserver::HotplugObserver (IUsbHidObserver* _observer, int _hotplugHandle, ::libusb_context* _context)
: observer (_observer),
  hotplugHandle (_hotplugHandle),
  context (_context)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

LibUsbDeviceManager::HotplugObserver::~HotplugObserver ()
{
	::libusb_hotplug_deregister_callback (context, hotplugHandle);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool LibUsbDeviceManager::HotplugObserver::matches (IUsbHidObserver* obs) const
{
	return obs == observer;
}

//************************************************************************************************
// LibUsbDeviceManager
//************************************************************************************************

DEFINE_CLASS_HIDDEN (LibUsbDeviceManager, HidApiDeviceManager)

//////////////////////////////////////////////////////////////////////////////////////////////////

ErrorCode LibUsbDeviceManager::startup ()
{
	ErrorCode result = SuperClass::startup ();
	if(result != kError_NoError)
		return result;
	
	if(useCount == 1)
	{
		::libusb_init_context (&libUsbContext, nullptr, 0);
		if(::libusb_has_capability (LIBUSB_CAP_HAS_HOTPLUG))
			eventListener = NEW UsbEventListener (libUsbContext);
	}
	
	return kError_NoError;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ErrorCode LibUsbDeviceManager::shutdown ()
{
	ErrorCode result = SuperClass::shutdown ();
	if(result != kError_NoError)
		return result;
	
	if(useCount == 0)
	{
		if(eventListener)
		{
			eventListener->release ();
			eventListener = nullptr;
		}
		::libusb_exit (nullptr);
	}
	
	return kError_NoError;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ErrorCode LibUsbDeviceManager::registerObserver (IUsbHidObserver* observer, const UsbIdFilter& filter)
{
	ErrorCode result = SuperClass::registerObserver (observer, filter);
	if(result != kError_NoError)
		return result;
	if(observer == nullptr)
		return kError_Failed;

	if(::libusb_has_capability (LIBUSB_CAP_HAS_HOTPLUG))
	{
		ASSERT (eventListener)
		bool wasEmpty = observers.isEmpty ();
		for(int i = 0; i < filter.numIds; i++)
		{
			::libusb_hotplug_callback_handle handle;
			int returnCode = ::libusb_hotplug_register_callback (libUsbContext, ::LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED | ::LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT, ::LIBUSB_HOTPLUG_ENUMERATE, filter.ids[i].vendorId, filter.ids[i].productId, LIBUSB_HOTPLUG_MATCH_ANY, hotplugCallback, observer, &handle);
			if(returnCode != ::LIBUSB_SUCCESS)
				return kError_Failed;
			observers.add (NEW HotplugObserver (observer, handle, libUsbContext));
		}
		if(wasEmpty && !observers.isEmpty ())
			eventListener->startTimer ();
	}
	else
	{
		ASSERT (false)
	}
		
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ErrorCode LibUsbDeviceManager::unregisterObserver (IUsbHidObserver* observer)
{
	ErrorCode result = SuperClass::unregisterObserver (observer);
	if(result != kError_NoError)
		return result;
	
	if(::libusb_has_capability (LIBUSB_CAP_HAS_HOTPLUG))
	{
		ASSERT (eventListener)
		observers.removeIf ([&] (const HotplugObserver* o)
		{
			return o->matches (observer);
		});
		if(observers.isEmpty ())
			eventListener->stopTimer ();
	}
	else
	{
		ASSERT (false)
	}
		
	return result;
}

