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
// Filename    : ccl/public/devices/ideviceenumerator.h
// Description : Device Enumerator Interface
//
//************************************************************************************************

#ifndef _ccl_ideviceenumerator_h
#define _ccl_ideviceenumerator_h

#include "ccl/public/base/iunknown.h"

namespace CCL {
	
interface IAttributeList;
interface IUnknownList;

//////////////////////////////////////////////////////////////////////////////////////////////////

#define PLUG_CATEGORY_DEVICEENUMERATOR CCLSTR ("DeviceEnumerator")

//************************************************************************************************
// IDeviceDescription
/** Device description interface. */
//************************************************************************************************

interface IDeviceDescription: IUnknown
{
	/** Get device name for display. */
	virtual StringRef CCL_API getDeviceName () const = 0;
	
	/** Get platform-specific device identifier. */
	virtual StringRef CCL_API getDeviceID () const = 0;
	
	/** Get device flags. */
	virtual int CCL_API getDeviceFlags () const = 0;

	/** Get additional device attributes. */
	virtual void CCL_API getDeviceAttributes (IAttributeList& a) const = 0;

	//////////////////////////////////////////////////////////////////////////////////////////////
	// Device flags and attributes
	//////////////////////////////////////////////////////////////////////////////////////////////

	static constexpr int kMediaRenderCapability = 1<<0;		///< device can render media
	static constexpr int kMediaCaptureCapability = 1<<1;	///< device can capture media

	DECLARE_STRINGID_MEMBER (kDeviceContainerID)			///< device container ID (Windows)

	DECLARE_IID (IDeviceDescription)
};

DEFINE_IID (IDeviceDescription, 0x8d4a13f8, 0xde67, 0x40d4, 0xbd, 0x4, 0x34, 0x49, 0x97, 0x1c, 0x78, 0xd)
DEFINE_STRINGID_MEMBER (IDeviceDescription, kDeviceContainerID, "DeviceContainerID")

//************************************************************************************************
// IDeviceEnumerator
/** Device enumeration interface. */
//************************************************************************************************

interface IDeviceEnumerator: IUnknown
{	
	/** Enumerate devices, returns IDeviceDescription instances. */
	virtual tresult CCL_API enumerateDevices (IUnknownList& resultList, int flags) = 0;

	DECLARE_IID (IDeviceEnumerator)
};

DEFINE_IID (IDeviceEnumerator, 0xe8c6319f, 0x4721, 0x4fe6, 0x92, 0xbe, 0x26, 0x8d, 0x85, 0x48, 0x31, 0x52)

} // namespace CCL

#endif // _ccl_ideviceenumerator_h
