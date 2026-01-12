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
// Filename    : ccl/extras/devices/deviceenumerator.cpp
// Description : Device enumeration base classes
//
//************************************************************************************************

#include "ccl/extras/devices/deviceenumerator.h"

using namespace CCL;

//************************************************************************************************
// DeviceDescription
//************************************************************************************************

DEFINE_CLASS_HIDDEN (DeviceDescription, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

DeviceDescription::DeviceDescription (StringRef idString, StringRef friendlyName, int flags)
: idString (idString),
  friendlyName (friendlyName),
  flags (flags)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringRef CCL_API DeviceDescription::getDeviceName () const
{
	return getFriendlyName ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringRef CCL_API DeviceDescription::getDeviceID () const
{
	return getIDString ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API DeviceDescription::getDeviceFlags () const
{
	return flags;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API DeviceDescription::getDeviceAttributes (IAttributeList& a) const
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool DeviceDescription::equals (const Object& obj) const
{
	const DeviceDescription* other = ccl_cast<DeviceDescription> (&obj);
	if(other)
		return idString == other->idString;
	else
		return SuperClass::equals (obj);
}

//************************************************************************************************
// DeviceEnumerator
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (DeviceEnumerator, Object)
