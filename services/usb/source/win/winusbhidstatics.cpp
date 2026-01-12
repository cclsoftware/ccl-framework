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
// Filename    : winusbhidstatics.cpp
// Description : Windows USB HID Statics
//
//************************************************************************************************

#include "winusbhidmanager.h"

#include "../usbhidstatics.h"

namespace CCL {
namespace Usb {

//************************************************************************************************
// WindowsUsbHidStatics
//************************************************************************************************

class WindowsUsbHidStatics: public UsbHidStatics
{
public:
	DECLARE_CLASS (WindowsUsbHidStatics, UsbHidStatics)

	// UsbHidStatics
	IUsbHidManager& CCL_API getUsbHidManager ();
};

} // namespace Usb
} // namespace CCL

using namespace CCL;
using namespace Usb;

//************************************************************************************************
// WindowsUsbHidStatics
//************************************************************************************************

DEFINE_EXTERNAL_SINGLETON (UsbHidStatics, WindowsUsbHidStatics)
DEFINE_CLASS_HIDDEN (WindowsUsbHidStatics, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

IUsbHidManager& CCL_API WindowsUsbHidStatics::getUsbHidManager ()
{
	static WindowsUsbHidManager deviceManager;
	return deviceManager;
}
