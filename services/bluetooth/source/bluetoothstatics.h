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
// Filename    : bluetoothstatics.h
// Description : Bluetooth Statics
//
//************************************************************************************************

#ifndef _bluetoothstatics_h
#define _bluetoothstatics_h

#include "ccl/base/singleton.h"
#include "ccl/public/plugins/pluginst.h"
#include "ccl/public/plugins/icoreplugin.h"
#include "ccl/public/devices/ibluetoothstatics.h"

namespace CCL {
namespace Bluetooth {

//************************************************************************************************
// GattCentralFactory
//************************************************************************************************

template <typename CentralClass>
class GattCentralFactory: public CorePropertyHandler<IGattCentralFactory, Object, IObject>
{
public:
	// IGattCentralFactory
	IGattCentral* createGattCentral () override { return NEW CentralClass; }
};

//************************************************************************************************
// GattPeripheralFactory
//************************************************************************************************

template <typename PeripheralClass>
class GattPeripheralFactory: public CorePropertyHandler<IGattPeripheralFactory, Object, IObject>
{
public:
	// IGattPeripheralFactory
	IGattPeripheral* createGattPeripheral () override { return NEW PeripheralClass; }
};

//************************************************************************************************
// BluetoothStatics
//************************************************************************************************

class BluetoothStatics: public Object,
						public IBluetoothStatics,
						public PluginInstance,
						public ExternalSingleton<BluetoothStatics>
{
public:
	DECLARE_CLASS_ABSTRACT (BluetoothStatics, Object)

	CLASS_INTERFACE2 (IBluetoothStatics, IPluginInstance, Object)
};

} // namespace Bluetooth
} // namespace CCL

#endif // _bluetoothstatics_h
