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
// Filename    : linuxbluetoothstatics.cpp
// Description : Linux Bluetooth Statics
//
//************************************************************************************************

#include "../bluetoothstatics.h"

#include "gattcentral.linux.h"
#include "gattperipheral.linux.h"

namespace CCL {
namespace Bluetooth {

//************************************************************************************************
// LinuxBluetoothStatics
//************************************************************************************************

class LinuxBluetoothStatics: public BluetoothStatics
{
public:
	DECLARE_CLASS (LinuxBluetoothStatics, BluetoothStatics)

	// BluetoothStatics
	IGattCentralFactory& CCL_API getGattCentralFactory () override;
	IGattPeripheralFactory& CCL_API getGattPeripheralFactory () override;
};

} // namespace Bluetooth
} // namespace CCL

using namespace CCL;
using namespace Bluetooth;

//************************************************************************************************
// LinuxBluetoothStatics
//************************************************************************************************

DEFINE_EXTERNAL_SINGLETON (BluetoothStatics, LinuxBluetoothStatics)
DEFINE_CLASS_HIDDEN (LinuxBluetoothStatics, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

IGattCentralFactory& CCL_API LinuxBluetoothStatics::getGattCentralFactory ()
{
	static GattCentralFactory<LinuxGattCentral> centralFactory;
	return centralFactory;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IGattPeripheralFactory& CCL_API LinuxBluetoothStatics::getGattPeripheralFactory ()
{
	static GattPeripheralFactory<LinuxGattPeripheral> peripheralFactory;
	return peripheralFactory;
}
