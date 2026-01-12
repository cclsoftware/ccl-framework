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
// Filename    : gattperipheral.linux.cpp
// Description : Bluetooth LE Gatt Peripheral
//
//************************************************************************************************

#include "gattperipheral.linux.h"
#include "gattshared.linux.h"

using namespace Core;
using namespace Core::Errors;

using namespace CCL;
using namespace CCL::Bluetooth;

//************************************************************************************************
// LinuxGattPeripheralDescriptor
//************************************************************************************************

LinuxGattPeripheralDescriptor::LinuxGattPeripheralDescriptor ()
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

LinuxGattPeripheralDescriptor::~LinuxGattPeripheralDescriptor ()
{}

//************************************************************************************************
// LinuxGattPeripheralCharacteristic
//************************************************************************************************

LinuxGattPeripheralCharacteristic::LinuxGattPeripheralCharacteristic ()
: nextCreateDescriptorId (0),
  nextNotifyId (0)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

LinuxGattPeripheralCharacteristic::~LinuxGattPeripheralCharacteristic ()
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void LinuxGattPeripheralCharacteristic::notify (const uint8 valueBuffer[], int valueSize)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

ErrorCode LinuxGattPeripheralCharacteristic::createDescriptorAsync (Core::UIDRef uuid, const uint8 valueBuffer[], int valueSize)
{
	return kError_NoError;
}

//************************************************************************************************
// LinuxGattPeripheralService
//************************************************************************************************

LinuxGattPeripheralService::LinuxGattPeripheralService ()
: nextCreateCharacteristicId (0)
{
	characteristics.objectCleanup ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

LinuxGattPeripheralService::~LinuxGattPeripheralService ()
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

ErrorCode LinuxGattPeripheralService::createCharacteristicAsync (const CharacteristicInfo& characteristicInfo)
{
	return kError_NoError;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

uint16 LinuxGattPeripheralService::getStartHandle () const
{
	return 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

uint16 LinuxGattPeripheralService::getStopHandle () const
{
	return 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void LinuxGattPeripheralService::addInclude (IGattPeripheralService* service)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool LinuxGattPeripheralService::startAdvertising ()
{
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool LinuxGattPeripheralService::stopAdvertising ()
{
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void LinuxGattPeripheralService::close ()
{
	characteristics.removeAll ();
	stopAdvertising ();
}

//************************************************************************************************
// LinuxGattPeripheral
//************************************************************************************************

LinuxGattPeripheral::LinuxGattPeripheral ()
: users (0)
{
	services.objectCleanup ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

LinuxGattPeripheral::~LinuxGattPeripheral ()
{
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void LinuxGattPeripheral::startup ()
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

ErrorCode LinuxGattPeripheral::createServiceAsync (Core::UIDRef uuid)
{
	return kError_NoError;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void LinuxGattPeripheral::shutdown ()
{
	services.removeAll();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IGattPeripheralService* LinuxGattPeripheral::getService (int index) const
{
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int LinuxGattPeripheral::getNumServices () const
{
	return services.count ();
}
