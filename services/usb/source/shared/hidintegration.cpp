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
// Filename    : hidintegration.cpp
// Description : HIDAPI Integration
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "hidintegration.h"

#include "ccl/public/text/stringbuilder.h"

#include "hidapi.h"
#if CCL_PLATFORM_MAC
#include "hidapi_darwin.h"
#endif

namespace CCL {
namespace Usb {

//************************************************************************************************
// HidApiDevice
//************************************************************************************************

class HidApiDevice: public Object,
					public IUsbHidInstance
{
public:
	HidApiDevice (::hid_device* device = nullptr);
	~HidApiDevice ();

	static HidApiDevice* cast (IUsbHidInstance* device) { return static_cast<HidApiDevice*> (device); }

	static void toStringResult (StringResult& string, const wchar_t* chars);
	static void toCString (MutableCString& cString, const wchar_t* chars);

	// IUsbHidInstance
	void getManufacturer (StringResult& string) const override;
	void getProduct (StringResult& string) const override;
	void getSerialNumber (StringResult& string) const override;
	uint16 getVendorId () const override;
	uint16 getProductId () const override;
	int writeToDevice (const uint8* data, int length) override;
	int readFromDevice (uint8* data, int length, int timeout = 0) override;

protected:
	::hid_device* device = nullptr;
};

} // namespace Usb
} // namespace CCL

using namespace Core;
using namespace Core::Errors;
using namespace CCL;
using namespace CCL::Usb;

//************************************************************************************************
// HidApiDevice
//************************************************************************************************

void HidApiDevice::toStringResult (StringResult& result, const wchar_t* chars)
{
	String string = StringWriter<>::fromWideChars (chars);
	string.toCString (Text::kUTF8, result.charBuffer, result.charBufferSize);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void HidApiDevice::toCString (MutableCString& cString, const wchar_t* chars)
{
	cString.empty ();
	cString.append (StringWriter<>::fromWideChars (chars), Text::kUTF8);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

HidApiDevice::HidApiDevice (::hid_device* _device)
: device (_device)
{
	ASSERT (device)
}

//////////////////////////////////////////////////////////////////////////////////////////////////

HidApiDevice::~HidApiDevice ()
{
	::hid_close (device);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void HidApiDevice::getManufacturer (StringResult& string) const
{
	if(const hid_device_info* info = ::hid_get_device_info (device))
		toStringResult (string, info->manufacturer_string);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void HidApiDevice::getProduct (StringResult& string) const
{
	if(const hid_device_info* info = ::hid_get_device_info (device))
		toStringResult (string, info->product_string);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void HidApiDevice::getSerialNumber (StringResult& string) const
{
	if(const hid_device_info* info = ::hid_get_device_info (device))
		toStringResult (string, info->serial_number);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

uint16 HidApiDevice::getVendorId () const
{
	if(const hid_device_info* info = ::hid_get_device_info (device))
		return info->vendor_id;
	return 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

uint16 HidApiDevice::getProductId () const
{
	if(const hid_device_info* info = ::hid_get_device_info (device))
		return info->product_id;
	return 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int HidApiDevice::writeToDevice (const uint8* data, int length)
{
	if(length < 0)
		return -1;

	if(length == 0)
		return 0;

	size_t bytesWritten = ::hid_write (device, data, length);
	#if DEBUG
	if(bytesWritten == -1)
		CCL_PRINTF ("HidApiDeviceManager::writeToDevice : %s\n", ::hid_error (device))
	#endif

	return int(bytesWritten);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int HidApiDevice::readFromDevice (uint8* data, int length, int timeout)
{
	size_t bytesRead = ::hid_read_timeout (device, data, length, timeout);
	#if DEBUG
	if(bytesRead == -1)
		CCL_PRINTF ("HidApiDeviceManager::readFromDevice : %s\n", ::hid_error (device))
	#endif

	return int(bytesRead);
}

//************************************************************************************************
// HidApiDeviceManager
//************************************************************************************************

DEFINE_CLASS_HIDDEN (HidApiDeviceManager, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

ErrorCode HidApiDeviceManager::startup ()
{
	if(++useCount == 1)
	{
		::hid_init ();

		#if CCL_PLATFORM_MAC
		::hid_darwin_set_open_exclusive (0); // do not use exclusive mode on the Mac
		#endif
	}

	return kError_NoError;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ErrorCode HidApiDeviceManager::shutdown ()
{
	if(useCount == 0)
		return kError_InvalidState;

	if(--useCount == 0)
		::hid_exit ();

	return kError_NoError;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ErrorCode HidApiDeviceManager::registerObserver (IUsbHidObserver* observer, const UsbIdFilter& filter)
{
	if(useCount == 0)
		return kError_InvalidState;
	return (observer == nullptr) ? kError_InvalidArgument : kError_NoError;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ErrorCode HidApiDeviceManager::unregisterObserver (IUsbHidObserver* observer)
{
	if(useCount == 0)
		return kError_InvalidState;
	return (observer == nullptr) ? kError_InvalidArgument : kError_NoError;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ErrorCode HidApiDeviceManager::openDevice (IUsbHidInstance*& device, const UsbDeviceInfo& info)
{
	if(useCount == 0)
		return kError_InvalidState;

	ErrorCode result = kError_NoError;

	String serialNumber (Text::kUTF8, info.serialNumber);
	WideCharString serialNumberWideString (serialNumber);

	if(hid_device* hid = ::hid_open (info.vendorId, info.productId, serialNumberWideString.str ()))
		device = NEW HidApiDevice (hid);
	else
	{
		#if DEBUG_LOG
		MutableCString error = StringWriter<>::fromWideChars (::hid_error (hid));
		CCL_PRINTF ("HidApiDeviceManager::openDevice : %s\n", error.str ())
		#endif
		device = nullptr;
		result = kError_Failed;
	}

	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ErrorCode HidApiDeviceManager::closeDevice (IUsbHidInstance* device)
{
	if(useCount == 0)
		return kError_InvalidState;

	ErrorCode result = kError_NoError;
	if(HidApiDevice* hidDevice = HidApiDevice::cast (device))
		hidDevice->release ();
	else
		result = kError_InvalidArgument;
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void HidApiDeviceManager::enumerate (Vector<InternalUsbDeviceInfo>& deviceInfos, const UsbIdPair& ids)
{
	::hid_device_info* enumeration = ::hid_enumerate (ids.vendorId, ids.productId);
	
	for(::hid_device_info* hidDevice = enumeration; hidDevice != nullptr; hidDevice = hidDevice->next)
	{
		MutableCString serialNumber;
		HidApiDevice::toCString (serialNumber, hidDevice->serial_number);
		deviceInfos.add ({hidDevice->vendor_id, hidDevice->product_id, serialNumber, hidDevice->path});
	}
	
	::hid_free_enumeration (enumeration);
}

//************************************************************************************************
// HidApiDeviceManager::InternalUsbDeviceInfoí
//************************************************************************************************

HidApiDeviceManager::InternalUsbDeviceInfo::InternalUsbDeviceInfo (uint16 vendorId, uint16 productId,
																   CStringPtr serialNumber, CStringPtr path)
: ids {vendorId, productId},
  serialNumberString (serialNumber),
  pathString (path)
{
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool HidApiDeviceManager::InternalUsbDeviceInfo::operator == (const InternalUsbDeviceInfo& other) const
{
	return ids == other.ids &&
		   serialNumberString == other.serialNumberString &&
		   pathString == other.pathString;
}
