//************************************************************************************************
//
// Bluetooth Support
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
// Filename    : gattshared.linux.cpp
// Description : Bluetooth LE Gatt Central/Peripheral Shared
//
//************************************************************************************************

#include "gattshared.linux.h"

namespace CCL {
namespace Bluetooth {

void toBluezGuid (MutableCString& result, const UID& uid)
{
	uid.toCString (result, UIDBytes::kStandardNoBraces);
	result.toLowercase ();
}

void fromBluezGuid (UIDBytes& result, CStringPtr uid)
{
	result.fromCString (uid, UIDBytes::kStandardNoBraces);
}

} // namespace Bluetooth
} // namespace CCL
