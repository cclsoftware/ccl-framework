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
// Filename    : gattshared.win.cpp
// Description : Bluetooth LE Gatt Central/Peripheral Shared
//
//************************************************************************************************

#include "gattshared.win.h"

#include "ccl/public/base/primitives.h"

namespace CCL {
namespace Bluetooth {

//////////////////////////////////////////////////////////////////////////////////////////////////

UIDBytes fromWinrtGuid (winrt::guid winrtUid)
{
	return INLINE_UID (winrtUid.Data1, winrtUid.Data2, winrtUid.Data3, 
					   winrtUid.Data4[0], winrtUid.Data4[1], winrtUid.Data4[2], winrtUid.Data4[3], 
					   winrtUid.Data4[4], winrtUid.Data4[5], winrtUid.Data4[6], winrtUid.Data4[7]);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

winrt::guid toWinrtGuid (UIDRef uid)
{
	std::array<uint8, 8> byteArray;
	for(int n = 0; n < 8; n++)
		byteArray[n] = uid.data4[n];

	return winrt::guid (uid.data1, uid.data2, uid.data3, byteArray);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Core::ErrorCode toErrorCode (const WinRTGattCommunicationStatus& status)
{
	switch(status)
	{
	case WinRTGattCommunicationStatus::Success :
		return Core::Errors::kError_NoError;
	case WinRTGattCommunicationStatus::Unreachable :
		return Core::Bluetooth::kError_BluetoothBusy;
	default :
		return Core::Errors::kError_Failed;
	}
}

} // namespace Bluetooth
} // namespace CCL
