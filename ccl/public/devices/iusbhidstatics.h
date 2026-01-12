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
// Filename    : ccl/public/devices/iusbhidstatics.h
// Description : USB HID Statics Interface
//
//************************************************************************************************

#ifndef _ccl_iusbhidstatics_h
#define _ccl_iusbhidstatics_h

#include "ccl/public/base/iunknown.h"

#include "core/public/devices/coreusbhid.h"

namespace CCL {

// import Core Library definitions
namespace Usb {
using namespace Core::Usb; }

//////////////////////////////////////////////////////////////////////////////////////////////////

namespace ClassID
{
	DEFINE_CID (UsbHidStatics, 0x65849bb9, 0x630, 0x403f, 0xb0, 0x65, 0xf, 0x88, 0x5c, 0xd, 0x70, 0xc6)
}

//************************************************************************************************
// IUsbHidStatics
//************************************************************************************************

interface IUsbHidStatics: IUnknown
{
	/** Get USB HID Manager instance. */
	virtual Usb::IUsbHidManager& CCL_API getUsbHidManager () = 0;

	DECLARE_IID (IUsbHidStatics)
};

DEFINE_IID (IUsbHidStatics, 0x577aaf7, 0xd6d9, 0x407b, 0x96, 0xf6, 0x41, 0x78, 0xea, 0x18, 0xe4, 0xaf)

} // namespace CCL

#endif // _ccl_iusbhidstatics_h
