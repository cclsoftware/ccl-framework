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
// Filename    : windowsbluetoothstatics.cpp
// Description : Windows Bluetooth Statics
//
//************************************************************************************************

#include "../bluetoothstatics.h"

#include "gattcentral.win.h"
#include "gattperipheral.win.h"

#pragma comment (lib, "windowsapp")

namespace CCL {
namespace Bluetooth {

//************************************************************************************************
// WindowsBluetoothStatics
//************************************************************************************************

class WindowsBluetoothStatics: public BluetoothStatics
{
public:
	DECLARE_CLASS (WindowsBluetoothStatics, BluetoothStatics)

	// BluetoothStatics
	IGattCentralFactory& CCL_API getGattCentralFactory () override;
	IGattPeripheralFactory& CCL_API getGattPeripheralFactory () override;
};

} // namespace Bluetooth
} // namespace CCL

using namespace CCL;
using namespace Bluetooth;

//************************************************************************************************
// WindowsBluetoothStatics
//************************************************************************************************

DEFINE_EXTERNAL_SINGLETON (BluetoothStatics, WindowsBluetoothStatics)
DEFINE_CLASS_HIDDEN (WindowsBluetoothStatics, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

IGattCentralFactory& CCL_API WindowsBluetoothStatics::getGattCentralFactory ()
{
	static GattCentralFactory<WindowsGattCentral> centralFactory;
	return centralFactory;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IGattPeripheralFactory& CCL_API WindowsBluetoothStatics::getGattPeripheralFactory ()
{
	static GattPeripheralFactory<WindowsGattPeripheral> peripheralFactory;
	return peripheralFactory;
}
