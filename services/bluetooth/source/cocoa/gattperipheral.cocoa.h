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
// Filename    : gattperipheral.cocoa.h
// Description : Bluetooth LE Gatt Peripheral (Core Bluetooth)
//
//************************************************************************************************

#ifndef _gattperipheral_cocoa_h
#define _gattperipheral_cocoa_h

#include "core/public/devices/coregattperipheral.h"
#include "core/public/coreobserver.h"

#include "ccl/base/object.h"

#include "ccl/public/collections/vector.h"
#include "ccl/public/plugins/icoreplugin.h"

#include "ccl/platform/cocoa/macutils.h"

@class CBCharacteristic;

namespace CCL {
namespace Bluetooth {

using namespace Core::Bluetooth;

class CocoaGattPeripheralService;
class CocoaGattPeripheralCharacteristic;

//************************************************************************************************
// CocoaGattPeripheral
//************************************************************************************************

class CocoaGattPeripheral: public CorePropertyHandler<IGattPeripheral, Object, IObject>
{
public:
	CocoaGattPeripheral ();

	// IGattPeripheral
	void startup () override;
	Core::ErrorCode createServiceAsync (Core::UIDRef uuid) override;
	void shutdown () override;
	IGattPeripheralService* getService (int index) const override;
	int getNumServices () const override;

	DEFINE_OBSERVER_OVERRIDE (IGattPeripheralObserver);

private:
	int nextCreateServiceCallId;
	Vector<CocoaGattPeripheralService*> services;
	int users;
};

//************************************************************************************************
// CocoaGattPeripheralService
//************************************************************************************************

class CocoaGattPeripheralService: public IGattPeripheralService
{
public:
	CocoaGattPeripheralService ();

	// IGattPeripheralService
	Core::ErrorCode createCharacteristicAsync (const CharacteristicInfo& characteristicInfo) override;
	uint16 getStartHandle () const override;
	uint16 getStopHandle () const override;
	void addInclude (IGattPeripheralService* service) override;
	tbool startAdvertising () override;
	tbool stopAdvertising () override;
	void close () override;

	DEFINE_OBSERVER_OVERRIDE (IGattPeripheralServiceObserver);

protected:
	int nextCreateCharacteristicId;
	Vector<CocoaGattPeripheralCharacteristic*> characteristics;
};

//************************************************************************************************
// CocoaGattPeripheralCharacteristic
//************************************************************************************************

class CocoaGattPeripheralCharacteristic: public IGattPeripheralCharacteristic
{
public:
	CocoaGattPeripheralCharacteristic ();
	CocoaGattPeripheralCharacteristic (CBCharacteristic* characteristic);
	~CocoaGattPeripheralCharacteristic ();

	// IGattPeripheralCharacteristic
	void notify (const uint8 valueBuffer[], int valueSize) override;
	Core::ErrorCode createDescriptorAsync (Core::UIDRef uuid, const uint8 valueBuffer[], int valueSize) override;

	DEFINE_OBSERVER_OVERRIDE (IGattPeripheralCharacteristicObserver);

protected:
	NSObj<CBCharacteristic> characteristic;
	int nextCreateDescriptorId;
	int nextNotifyId;
};

//************************************************************************************************
// CocoaGattPeripheralDescriptor
//************************************************************************************************

class CocoaGattPeripheralDescriptor: public IGattPeripheralDescriptor
{
public:
	CocoaGattPeripheralDescriptor ();
	~CocoaGattPeripheralDescriptor ();

	DEFINE_OBSERVER_OVERRIDE (IGattPeripheralDescriptorObserver)
};

} // namespace Bluetooth
} // namespace Core

#endif //_gattperipheral_cocoa_h
