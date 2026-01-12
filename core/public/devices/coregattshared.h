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
// Filename    : core/public/devices/coregattshared.h
// Description : Bluetooth LE GATT Shared
//
//************************************************************************************************

#ifndef _coregattshared_h
#define _coregattshared_h

#include "core/public/coreuid.h"
#include "core/public/coreplugin.h"

namespace Core {
namespace Bluetooth {

constexpr int kAttributeCapacity = 512;

//************************************************************************************************
// Error Codes
//************************************************************************************************

enum BluetoothErrorCodes
{
	kError_BluetoothBusy = 2000
};

//************************************************************************************************
// CharacteristicProperties
//************************************************************************************************

DEFINE_ENUM (CharacteristicProperties)
{
	kNone = 0,
	kBroadcast = 1,
	kRead = 1 << 1,
	kWriteWithoutResponse = 1 << 2,
	kWrite = 1 << 3,
	kNotify = 1 << 4,
	kIndicate = 1 << 5,
	kAuthenticatedSignedWrites = 1 << 6,
	kExtendedProperties = 1 << 7,
	kReliableWrites = 1 << 8,
	kWritableAuxiliaries = 1 << 9
};

//************************************************************************************************
// Standard GATT UUIDS
//************************************************************************************************

static const UIDBytes kUserDescription = INLINE_UID (0x00002901, 0x0000, 0x1000, 0x80, 0x00, 0x00, 0x80, 0x5f, 0x9b, 0x34, 0xfb);
static const UIDBytes kServiceChanged = INLINE_UID (0x00002a05, 0x0000, 0x1000, 0x80, 0x00, 0x00, 0x80, 0x5f, 0x9b, 0x34, 0xfb);

} // namespace Bluetooth
} // namespace Core

#endif // _coregattshared_h
