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
// Filename    : gattperipheral.android.cpp
// Description : Android Bluetooth LE Gatt Peripheral
//
//************************************************************************************************

#include "gattperipheral.android.h"

using namespace Core;
using namespace Core::Errors;

using namespace CCL;
using namespace CCL::Bluetooth;

//************************************************************************************************
// AndroidGattPeripheral
//************************************************************************************************

void AndroidGattPeripheral::startup ()
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

ErrorCode AndroidGattPeripheral::createServiceAsync (UIDRef uuid)
{
	return kError_NotImplemented;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AndroidGattPeripheral::shutdown ()
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

IGattPeripheralService* AndroidGattPeripheral::getService (int index) const
{
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int AndroidGattPeripheral::getNumServices () const
{
	return 0;
}
