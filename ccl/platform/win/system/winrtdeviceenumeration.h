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
// Filename    : ccl/platform/win/system/winrtdeviceenumeration.h
// Description : WinRT Device Enumeration
//
//************************************************************************************************

#ifndef _winrtdeviceenumeration_h
#define _winrtdeviceenumeration_h

#include "ccl/platform/win/system/cclwinrt.h"

#include <windows.devices.enumeration.h> // requires Windows 10 SDK to compile!

namespace CCL {
namespace WinRT {

using namespace ABI::Windows::Devices::Enumeration;

//************************************************************************************************
// DeviceEnumerationHandler
//************************************************************************************************

class DeviceEnumerationHandler: public Unknown
{
public:
	void setWatcher (IDeviceWatcher* watcher)
	{
		deviceWatcher.share (watcher);
	}

	bool start ()
	{
		ASSERT (deviceWatcher)
		if(!deviceWatcher)
			return false;

		HRESULT hr = deviceWatcher->add_Added (DeviceInformationHandler::make (this, &DeviceEnumerationHandler::onAdded), &addedEventToken);
		ASSERT (SUCCEEDED (hr))
		hr = deviceWatcher->add_Removed (DeviceInformationUpdateHandler::make (this, &DeviceEnumerationHandler::onRemoved), &removedEventToken);
		ASSERT (SUCCEEDED (hr))
		hr = deviceWatcher->add_EnumerationCompleted (EnumerationCompletedHandler::make (this, &DeviceEnumerationHandler::onEnumerationCompleted), &completedEventToken);
		ASSERT (SUCCEEDED (hr))

		return SUCCEEDED (deviceWatcher->Start ());
	}

	void stop ()
	{
		if(!deviceWatcher)
			return;

		HRESULT hr = deviceWatcher->Stop ();
		ASSERT (SUCCEEDED (hr))

		hr = deviceWatcher->remove_Added (addedEventToken);
		ASSERT (SUCCEEDED (hr))
		hr = deviceWatcher->remove_Removed (removedEventToken);
		ASSERT (SUCCEEDED (hr))
		hr = deviceWatcher->remove_EnumerationCompleted (completedEventToken);
		ASSERT (SUCCEEDED (hr))
	}

	virtual HRESULT onAdded (IDeviceWatcher*, IDeviceInformation* information)
	{
		return S_OK;
	}

	virtual HRESULT onRemoved (IDeviceWatcher*, IDeviceInformationUpdate* update)
	{
		return S_OK;
	}
	
	virtual HRESULT onEnumerationCompleted (IDeviceWatcher*, IInspectable* args)
	{
		return S_OK;
	}

	// helpers
	static String getDeviceInstanceId (IDeviceInformation* information)
	{
		String deviceInstanceId;
		ComPtr<__FIMapView_2_HSTRING_IInspectable> properties;
		if(SUCCEEDED (information->get_Properties (properties)))
		{
			ComPtr<IInspectable> value;
			properties->Lookup (PlatformString (L"System.Devices.DeviceInstanceId"), value);
			deviceInstanceId = PropertyVariant (value).asString ();
		}
		return deviceInstanceId;
	}

	#if DEBUG
	static void dumpDeviceProperties (IDeviceInformation* information)
	{
		ComPtr<__FIMapView_2_HSTRING_IInspectable> properties;
		if(SUCCEEDED (information->get_Properties (properties)))
		{
			IterableForEach (KeyValuePair_HSTRING_IInspectable_Iterable, properties, pair)
				PlatformString key;
				pair->get_Key (key);
				String keyString (key.asString ());

				ComPtr<IInspectable> value;
				pair->get_Value (value);
				PropertyVariant valueVariant (value);

				Debugger::println (String () << keyString << " = " << VariantString (valueVariant));
			EndFor
		}
	}
	#endif

protected:
	ComPtr<IDeviceWatcher> deviceWatcher;
	EventRegistrationToken addedEventToken = {0};
	EventRegistrationToken removedEventToken = {0};
	EventRegistrationToken completedEventToken = {0};

	typedef TypedEventHandler<__FITypedEventHandler_2_Windows__CDevices__CEnumeration__CDeviceWatcher_Windows__CDevices__CEnumeration__CDeviceInformation, 
							  IDeviceWatcher*, IDeviceInformation*, 
							  DeviceEnumerationHandler> DeviceInformationHandler;

	typedef TypedEventHandler<__FITypedEventHandler_2_Windows__CDevices__CEnumeration__CDeviceWatcher_Windows__CDevices__CEnumeration__CDeviceInformationUpdate,
							  IDeviceWatcher*, IDeviceInformationUpdate*, 
							  DeviceEnumerationHandler> DeviceInformationUpdateHandler;

	typedef TypedEventHandler<__FITypedEventHandler_2_Windows__CDevices__CEnumeration__CDeviceWatcher_IInspectable, 
							  IDeviceWatcher*, IInspectable*, 
							  DeviceEnumerationHandler> EnumerationCompletedHandler;
};

} // namespace WinRT
} // namespace CCL

#endif // _winrtdeviceenumeration_h
