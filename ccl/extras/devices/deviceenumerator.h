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
// Filename    : ccl/extras/devices/deviceenumerator.h
// Description : Device enumeration base classes
//
//************************************************************************************************

#ifndef _ccl_deviceenumerator_h
#define _ccl_deviceenumerator_h

#include "ccl/base/object.h"

#include "ccl/public/text/cclstring.h"
#include "ccl/public/devices/ideviceenumerator.h"

namespace CCL {

//************************************************************************************************
// DeviceDescription
//************************************************************************************************

class DeviceDescription: public Object,
						 public IDeviceDescription
{
public:
	DECLARE_CLASS (DeviceDescription, Object)

	DeviceDescription (StringRef idString = nullptr, StringRef friendlyName = nullptr, int flags = 0);

	PROPERTY_STRING (idString, IDString)
	PROPERTY_STRING (friendlyName, FriendlyName)

	// IDeviceDescription
	StringRef CCL_API getDeviceName () const override;
	StringRef CCL_API getDeviceID () const override;
	int CCL_API getDeviceFlags () const override;
	void CCL_API getDeviceAttributes (IAttributeList& a) const override;

	// Object
	bool equals (const Object& obj) const override;

	CLASS_INTERFACE (IDeviceDescription, Object)

protected:
	int flags;
};

//************************************************************************************************
// DeviceEnumerator
//************************************************************************************************

class DeviceEnumerator: public Object,
						public IDeviceEnumerator
{
public:
	DECLARE_CLASS_ABSTRACT (DeviceEnumerator, Object)

	CLASS_INTERFACE (IDeviceEnumerator, Object)
};

} // namespac CCL

#endif // _ccl_deviceenumerator_h
