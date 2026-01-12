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
// Filename    : gattshared.cocoa.mm
// Description : Bluetooth LE Gatt (Core Bluetooth)
//
//************************************************************************************************

#include "gattshared.cocoa.h"

#include "ccl/public/base/ccldefpush.h"
#include <CoreBluetooth/CoreBluetooth.h>
#include "ccl/public/base/ccldefpop.h"

using namespace Core;

Core::UIDBytes fromCBUUID (CBUUID* cbuuid)
{
	ASSERT (cbuuid.data.length == 16)
	const uint8* bytes = (const uint8*)cbuuid.data.bytes;
	
	uint32 data1 = bytes[0];
	for(int i = 1; i < 4; i++)
	{
		data1 <<= 8;
		data1 |= bytes[i];
	}
	uint16 data2 = bytes[4];
	data2 <<= 8;
	data2 |= bytes[5];
	uint16 data3 = bytes[6];
	data3 <<= 8;
	data3 |= bytes[7];
	uint8 data4[8];
	for(int i = 0; i < 8; i++)
		data4[i] = bytes[8 + i];

	return INLINE_UID (data1, data2, data3, data4[0], data4[1], data4[2], data4[3], data4[4], data4[5], data4[6], data4[7]);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void toCBUUID (CBUUID*& cbuuid, Core::UIDRef uid)
{
	uint8 buffer[16];
	for(int i = 0; i < 4; i++)
		buffer[3 - i] = (uid.data1 >> (i * 8)) & 0xFF;
	for(int i = 0; i < 2; i++)
		buffer[5 - i] = (uid.data2 >> (i * 8)) & 0xFF;
	for(int i = 0; i < 2; i++)
		buffer[7 - i] = (uid.data3 >> (i * 8)) & 0xFF;

	for(int i = 0; i < 8; i++)
		buffer[8 + i] = uid.data4[i];

	cbuuid = [CBUUID UUIDWithData:[NSData dataWithBytes:buffer length:16]];
}
