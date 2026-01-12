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
// Filename    : usbhidstatics.h
// Description : USB HID Statics
//
//************************************************************************************************

#ifndef _usbhidstatics_h
#define _usbhidstatics_h

#include "ccl/base/singleton.h"
#include "ccl/public/plugins/pluginst.h"
#include "ccl/public/devices/iusbhidstatics.h"

namespace CCL {
namespace Usb {

//************************************************************************************************
// UsbHidStatics
//************************************************************************************************

class UsbHidStatics: public Object,
					 public IUsbHidStatics,
					 public PluginInstance,
					 public ExternalSingleton<UsbHidStatics>
{
public:
	DECLARE_CLASS_ABSTRACT (UsbHidStatics, Object)

	CLASS_INTERFACE2 (IUsbHidStatics, IPluginInstance, Object)
};

} // namespace Usb
} // namespace CCL

#endif // _usbhidstatics_h
