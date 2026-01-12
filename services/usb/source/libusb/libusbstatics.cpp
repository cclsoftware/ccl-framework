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
// Filename    : libusbstatics.cpp
// Description : Libusb Statics
//
//************************************************************************************************

#include "libusbmanager.h"

#include "../usbhidstatics.h"

namespace CCL {
namespace Usb {

//************************************************************************************************
// LibUsbStatics
//************************************************************************************************

class LibUsbStatics: public UsbHidStatics
{
public:
	DECLARE_CLASS (LibUsbStatics, UsbHidStatics)

	// UsbHidStatics
	IUsbHidManager& CCL_API getUsbHidManager () override;
};

} // namespace Usb
} // namespace CCL

using namespace CCL;
using namespace Usb;

//************************************************************************************************
// LibUsbStatics
//************************************************************************************************

DEFINE_EXTERNAL_SINGLETON (UsbHidStatics, LibUsbStatics)
DEFINE_CLASS_HIDDEN (LibUsbStatics, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

IUsbHidManager& CCL_API LibUsbStatics::getUsbHidManager ()
{
	static LibUsbDeviceManager deviceManager;
	return deviceManager;
}
