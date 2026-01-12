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
// Filename    : androidbluetoothstatics.cpp
// Description : Android Bluetooth Statics
//
//************************************************************************************************

#include "../bluetoothstatics.h"

#include "gattcentral.android.h"
#include "gattperipheral.android.h"

namespace CCL {
namespace Bluetooth {

//************************************************************************************************
// AndroidBluetoothStatics
//************************************************************************************************

class AndroidBluetoothStatics: public BluetoothStatics
{
public:
	DECLARE_CLASS (AndroidBluetoothStatics, BluetoothStatics)

	// BluetoothStatics
	IGattCentralFactory& CCL_API getGattCentralFactory () override;
	IGattPeripheralFactory& CCL_API getGattPeripheralFactory () override;
};

} // namespace Bluetooth
} // namespace CCL

using namespace CCL;
using namespace Bluetooth;

//************************************************************************************************
// AndroidBluetoothStatics
//************************************************************************************************

DEFINE_EXTERNAL_SINGLETON (BluetoothStatics, AndroidBluetoothStatics)
DEFINE_CLASS_HIDDEN (AndroidBluetoothStatics, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

IGattCentralFactory& CCL_API AndroidBluetoothStatics::getGattCentralFactory ()
{
	static GattCentralFactory<AndroidGattCentral> centralFactory;
	return centralFactory;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IGattPeripheralFactory& CCL_API AndroidBluetoothStatics::getGattPeripheralFactory ()
{
	static GattPeripheralFactory<AndroidGattPeripheral> peripheralFactory;
	return peripheralFactory;
}
