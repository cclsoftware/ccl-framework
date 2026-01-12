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
// Filename    : core/public/devices/coreusbhid.h
// Description : USB HID Interfaces
//
//************************************************************************************************

#ifndef _coreusbhid_h
#define _coreusbhid_h

#include "core/public/coreplugin.h"

namespace Core {
namespace Usb {

/*
	The IUsbHidManager interface facilitates communication with HID peripherals over USB.

	USB HID devices are identified by three values:
	- The vendorId is a value assigned by the USB Implementers Forum (USB-IF) to the manufacturer.
	- The productId is used to differentiate between different products from the same manufacturer.
	- The serialNumber is unique for every device.

	The IUsbHidManager accepts an optional filter to only notify observers about devices with a
	specific vendorId and productId. If the filter is empty, all devices will be reported.

	All functions provided by this interface must be invoked from the main thread. Additionally,
	any callbacks registered with this interface will also be executed on the main thread.
*/

//************************************************************************************************
// UsbIdPair
//************************************************************************************************

struct UsbIdPair
{
	uint16 vendorId = 0;
	uint16 productId = 0;

	bool operator == (const UsbIdPair& other) const
	{
		return vendorId == other.vendorId && productId == other.productId;
	}
};

//************************************************************************************************
// UsbIdFilter
//************************************************************************************************

struct UsbIdFilter
{
	const UsbIdPair* ids = nullptr;
	int numIds = 0;

	bool contains (const UsbIdPair& id) const
	{
		for(int i = 0; i < numIds; i++)
			if(ids[i] == id)
				return true;
		return false;
	}
};

//************************************************************************************************
// UsbDeviceInfo
//************************************************************************************************

struct UsbDeviceInfo: UsbIdPair
{
	CStringPtr serialNumber = nullptr;
};

//************************************************************************************************
// IUsbHidObserver
//************************************************************************************************

struct IUsbHidObserver
{
	/** When an observer is registered using IUsbHidManager::registerObserver(),
		the onDeviceAdded() method will be called for all USB HID devices that
		match the provided filter. Additionally, if a new device that matches the
		filter becomes available, onDeviceAdded() will be called automatically. */
	virtual void onDeviceAdded (const UsbDeviceInfo& device) = 0;

	/** This method is called automatically when a previously added device
		becomes unavailable. Removing an observer does not trigger the
		onDeviceRemoved() callback. */
	virtual void onDeviceRemoved (const UsbDeviceInfo& device) = 0;
};

//************************************************************************************************
// IUsbHidInstance
//************************************************************************************************

struct IUsbHidInstance
{
	/** Get the name of the device manufacturer. This may or may not correspond
		to the name associated with the vendorId. */
	virtual void getManufacturer (StringResult& string) const = 0;

	/** Get the name of the product. */
	virtual void getProduct (StringResult& string) const = 0;

	/** Get the serial number. This string is not necessarily alpha-numeric. */
	virtual void getSerialNumber (StringResult& string) const = 0;

	/** Get the vendorId of this device. */
	virtual uint16 getVendorId () const = 0;

	/** Get the productId of this device. */
	virtual uint16 getProductId () const = 0;

	/** Write data to the device. The device must be open.
		The first byte must contain the Report ID, or 0 if the device only has
		one report. It returns the number of bytes written. */
	virtual int writeToDevice (const uint8* data, int length) = 0;

	/** Synchronously poll data from the device. The device must be open.
		If (and only if) the device has multiple reports, the first byte is the
		Report ID. It polls at most `length` bytes and returns the number of
		bytes read in at most `timeout` milliseconds. */
	virtual int readFromDevice (uint8* data, int length, int timeout = 0) = 0;
};

//************************************************************************************************
// IUsbHidManager
//************************************************************************************************

struct IUsbHidManager: IPropertyHandler
{
	/** This method must be called before any other method in the IUsbHidManager
		interface. */
	virtual ErrorCode startup () = 0;

	/** Release all allocated memory owned by the IUsbHidManager. */
	virtual ErrorCode shutdown () = 0;

	/** Registers an observer to receive notifications about USB HID devices.
		The IUsbHidManager will call onDeviceAdded() for all currently available
		devices that match the provided filter. If no filter is provided, it will
		call onDeviceAdded() for all available devices. */
	virtual ErrorCode registerObserver (IUsbHidObserver* observer, const UsbIdFilter& filter) = 0;

	/** Unregister an observer. */
	virtual ErrorCode unregisterObserver (IUsbHidObserver* observer) = 0;

	/** Open a device for writing and reading data. The caller is responsible for
		closing the device using closeDevice() when done. */
	virtual ErrorCode openDevice (IUsbHidInstance*& device, const UsbDeviceInfo& info) = 0;

	/**	Close a device. */
	virtual ErrorCode closeDevice (IUsbHidInstance* device) = 0;

	static const InterfaceID kIID = FOUR_CHAR_ID ('U', 'H', 'I', 'M');
};

} // namespace Usb
} // namespace Core

#endif // _coreusbhid_h

