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
// Filename    : hidintegration.h
// Description : HIDAPI Integration
//
//************************************************************************************************

#ifndef _hidintegration_h
#define _hidintegration_h

#include "ccl/base/object.h"

#include "ccl/public/text/cstring.h"
#include "ccl/public/collections/vector.h"
#include "ccl/public/plugins/icoreplugin.h"

#include "ccl/public/devices/iusbhidstatics.h"

namespace CCL {
namespace Usb {

//************************************************************************************************
// HidApiDeviceManager
//************************************************************************************************

class HidApiDeviceManager: public CorePropertyHandler<IUsbHidManager, Object, IObject>
{
public:
	DECLARE_CLASS_ABSTRACT (HidApiDeviceManager, Object)

	// IUsbHidManager
	Core::ErrorCode startup () override;
	Core::ErrorCode shutdown () override;
	Core::ErrorCode registerObserver (IUsbHidObserver* observer, const UsbIdFilter& filter) override;
	Core::ErrorCode unregisterObserver (IUsbHidObserver* observer) override;
	Core::ErrorCode openDevice (IUsbHidInstance*& device, const UsbDeviceInfo& info) override;
	Core::ErrorCode closeDevice (IUsbHidInstance* device) override;

protected:
	int useCount = 0;

	struct InternalUsbDeviceInfo
	{
		Core::Usb::UsbIdPair ids;
		MutableCString serialNumberString;
		MutableCString pathString;

		InternalUsbDeviceInfo () = default;
		InternalUsbDeviceInfo (uint16 vendorId, uint16 productId, CStringPtr serialNumber, CStringPtr path);

		bool operator == (const InternalUsbDeviceInfo& other) const;
	};

	void enumerate (Vector<InternalUsbDeviceInfo>& deviceInfos, const UsbIdPair& ids);
};

} // namespace Usb
} // namespace CCL

#endif // _hidintegration_h
