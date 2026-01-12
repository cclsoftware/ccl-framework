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
// Filename    : gattshared.win.h
// Description : Bluetooth LE Gatt Central/Peripheral Shared
//
//************************************************************************************************

#ifndef _gattshared_win_h
#define _gattshared_win_h

#include <winrt/base.h>
#include <winrt/Windows.Devices.Bluetooth.GenericAttributeProfile.h>

#include "core/public/devices/coregattshared.h"
#include "ccl/public/base/uid.h"

namespace CCL {
namespace Bluetooth {

using WinRTGattCommunicationStatus = winrt::Windows::Devices::Bluetooth::GenericAttributeProfile::GattCommunicationStatus;

//////////////////////////////////////////////////////////////////////////////////////////////////
// UID Conversion
//////////////////////////////////////////////////////////////////////////////////////////////////

UIDBytes fromWinrtGuid (winrt::guid winrtUid);
winrt::guid toWinrtGuid (UIDRef uid);

//////////////////////////////////////////////////////////////////////////////////////////////////
// Error Conversion
//////////////////////////////////////////////////////////////////////////////////////////////////

Core::ErrorCode toErrorCode (const WinRTGattCommunicationStatus& status);

} // namespace Bluetooth
} // namespace CCL

#endif // _gattshared_win_h
