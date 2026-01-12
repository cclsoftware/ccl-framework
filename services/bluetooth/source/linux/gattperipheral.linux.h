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
// Filename    : gattperipheral.linux.h
// Description : Bluetooth LE Gatt Peripheral
//
//************************************************************************************************

#ifndef _gattperipheral_linux_h
#define _gattperipheral_linux_h

#include "core/public/devices/coregattperipheral.h"

#include "core/public/coreobserver.h"

#include "ccl/base/object.h"
#include "ccl/base/collections/objectarray.h"

#include "ccl/public/plugins/icoreplugin.h"

namespace CCL {
namespace Bluetooth {

using namespace Core::Bluetooth;

class LinuxGattPeripheralService;
class LinuxGattPeripheralCharacteristic;

//************************************************************************************************
// LinuxGattPeripheral
//************************************************************************************************

class LinuxGattPeripheral: public CorePropertyHandler<IGattPeripheral, Object, IObject>
{
public:
	LinuxGattPeripheral ();
	~LinuxGattPeripheral ();

	// IGattPeripheral
	void startup () override;
	Core::ErrorCode createServiceAsync (Core::UIDRef uuid) override;
	void shutdown () override;
	IGattPeripheralService* getService (int index) const override;
	int getNumServices () const override;

	DEFINE_OBSERVER (IGattPeripheralObserver)

private:
	int nextCreateServiceCallId;
	ObjectArray services;
	int users;
};

//************************************************************************************************
// LinuxGattPeripheralService
//************************************************************************************************

class LinuxGattPeripheralService: public Object,
								  public IGattPeripheralService
{
public:
	LinuxGattPeripheralService ();
	~LinuxGattPeripheralService ();

	// IGattPeripheralService
	Core::ErrorCode createCharacteristicAsync (const CharacteristicInfo& characteristicInfo) override;
	uint16 getStartHandle () const override;
	uint16 getStopHandle () const override;
	void addInclude (IGattPeripheralService* service) override;
	tbool startAdvertising () override;
	tbool stopAdvertising () override;
	void close () override;

	DEFINE_OBSERVER (IGattPeripheralServiceObserver)

protected:
	int nextCreateCharacteristicId;
	ObjectArray characteristics;
};

//************************************************************************************************
// LinuxGattPeripheralCharacteristic
//************************************************************************************************

class LinuxGattPeripheralCharacteristic: public Object,
										 public IGattPeripheralCharacteristic
{
public:
	LinuxGattPeripheralCharacteristic ();
	~LinuxGattPeripheralCharacteristic ();

	// IGattPeripheralCharacteristic
	void notify (const uint8 valueBuffer[], int valueSize) override;
	Core::ErrorCode createDescriptorAsync (Core::UIDRef uuid, const uint8 valueBuffer[], int valueSize) override;

	DEFINE_OBSERVER (IGattPeripheralCharacteristicObserver)

protected:
	int nextCreateDescriptorId;
	int nextNotifyId;
};

//************************************************************************************************
// LinuxGattPeripheralDescriptor
//************************************************************************************************

class LinuxGattPeripheralDescriptor: public IGattPeripheralDescriptor
{
public:
	LinuxGattPeripheralDescriptor ();
	~LinuxGattPeripheralDescriptor ();

	DEFINE_OBSERVER (IGattPeripheralDescriptorObserver)
};

} // namespace Bluetooth
} // namespace Core

#endif //_gattperipheral_linux_h
