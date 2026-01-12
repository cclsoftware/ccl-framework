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
// Filename    : gattperipheral.win.h
// Description : Bluetooth LE Gatt Peripheral
//
//************************************************************************************************

#ifndef _gattperipheral_win_h
#define _gattperipheral_win_h

#include "core/public/devices/coregattperipheral.h"

#include <winrt/Windows.Devices.Bluetooth.GenericAttributeProfile.h>
#include <winrt/Windows.Devices.Bluetooth.h>

#include "core/public/coreobserver.h"

#include "ccl/base/object.h"

#include "ccl/public/collections/vector.h"
#include "ccl/public/plugins/icoreplugin.h"

namespace CCL {
namespace Bluetooth {

using namespace Core::Bluetooth;

class WindowsGattPeripheralService;
class WindowsGattPeripheralCharacteristic;

//************************************************************************************************
// WindowsGattPeripheral
//************************************************************************************************

class WindowsGattPeripheral: public CorePropertyHandler<IGattPeripheral, Object, IObject>
{
public:
	WindowsGattPeripheral ();

	// IGattPeripheral
	void startup () override;
	Core::ErrorCode createServiceAsync (Core::UIDRef uuid) override;
	void shutdown () override;
	IGattPeripheralService* getService (int index) const override;
	int getNumServices () const override;

	DEFINE_OBSERVER (IGattPeripheralObserver);

private:
	Vector<WindowsGattPeripheralService*> services;
	int users;
};

//************************************************************************************************
// WindowsGattPeripheralService
//************************************************************************************************

class WindowsGattPeripheralService: public IGattPeripheralService
{
public:
	// IGattPeripheralService
	Core::ErrorCode createCharacteristicAsync (const CharacteristicInfo& characteristicInfo) override;
	uint16 getStartHandle () const override;
	uint16 getStopHandle () const override;
	void addInclude (IGattPeripheralService* service) override;
	tbool startAdvertising () override;
	tbool stopAdvertising () override;
	void close () override;

	DEFINE_OBSERVER (IGattPeripheralServiceObserver);

protected:
	friend WindowsGattPeripheral;
	winrt::Windows::Devices::Bluetooth::GenericAttributeProfile::GattServiceProvider serviceProvider {nullptr};
	Vector<WindowsGattPeripheralCharacteristic*> characteristics;
};

//************************************************************************************************
// WindowsGattPeripheralCharacteristic
//************************************************************************************************

class WindowsGattPeripheralCharacteristic: public IGattPeripheralCharacteristic
{
public:
	WindowsGattPeripheralCharacteristic ();
	WindowsGattPeripheralCharacteristic (winrt::Windows::Devices::Bluetooth::GenericAttributeProfile::GattLocalCharacteristic& characteristic);
	~WindowsGattPeripheralCharacteristic ();

	// IGattPeripheralCharacteristic
	void notify (const uint8 valueBuffer[], int valueSize) override;
	Core::ErrorCode createDescriptorAsync (Core::UIDRef uuid, const uint8 valueBuffer[], int valueSize) override;

	DEFINE_OBSERVER (IGattPeripheralCharacteristicObserver);

protected:
	friend WindowsGattPeripheralService;
	winrt::Windows::Devices::Bluetooth::GenericAttributeProfile::GattLocalCharacteristic characteristic;
	winrt::event_token readToken;
	winrt::event_token writeToken;
};

//************************************************************************************************
// WindowsGattPeripheralDescriptor
//************************************************************************************************

class WindowsGattPeripheralDescriptor: public IGattPeripheralDescriptor
{
public:
	WindowsGattPeripheralDescriptor ();
	~WindowsGattPeripheralDescriptor ();

	DEFINE_OBSERVER (IGattPeripheralDescriptorObserver)

protected:
	friend WindowsGattPeripheralCharacteristic;
	winrt::Windows::Devices::Bluetooth::GenericAttributeProfile::GattLocalDescriptor descriptor {nullptr};
	winrt::event_token readToken;
	winrt::event_token writeToken;
};

} // namespace Bluetooth
} // namespace Core

#endif //_gattperipheral_win_h
