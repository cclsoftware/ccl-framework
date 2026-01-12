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
// Filename    : gattperipheral.cocoa.mm
// Description : Bluetooth LE Gatt Peripheral (Core Bluetooth)
//
//************************************************************************************************

#include "gattperipheral.cocoa.h"
#include "gattshared.cocoa.h"

#include "core/public/coreuid.h"
#include "ccl/public/base/debug.h"

#include "ccl/public/base/ccldefpush.h"
#include <CoreBluetooth/CoreBluetooth.h>
#include "ccl/public/base/ccldefpop.h"

using namespace Core;
using namespace Core::Errors;

using namespace CCL;
using namespace CCL::Bluetooth;

//************************************************************************************************
// CocoaGattPeripheralDescriptor
//************************************************************************************************

CocoaGattPeripheralDescriptor::CocoaGattPeripheralDescriptor ()
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

CocoaGattPeripheralDescriptor::~CocoaGattPeripheralDescriptor ()
{}

//************************************************************************************************
// CocoaGattPeripheralCharacteristic
//************************************************************************************************

CocoaGattPeripheralCharacteristic::CocoaGattPeripheralCharacteristic ()
: characteristic (nil),
  nextCreateDescriptorId (0)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

CocoaGattPeripheralCharacteristic::CocoaGattPeripheralCharacteristic (CBCharacteristic* _characteristic)
: characteristic (_characteristic),
  nextCreateDescriptorId (0)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

CocoaGattPeripheralCharacteristic::~CocoaGattPeripheralCharacteristic ()
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CocoaGattPeripheralCharacteristic::notify (const uint8 valueBuffer[], int valueSize)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

ErrorCode CocoaGattPeripheralCharacteristic::createDescriptorAsync (Core::UIDRef uuid, const uint8 valueBuffer[], int valueSize)
{
	return kError_NoError;
}

//************************************************************************************************
// CocoaGattPeripheralService
//************************************************************************************************

CocoaGattPeripheralService::CocoaGattPeripheralService ()
: nextCreateCharacteristicId (0)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

ErrorCode CocoaGattPeripheralService::createCharacteristicAsync (const CharacteristicInfo& characteristicInfo)
{
	return kError_NoError;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

uint16 CocoaGattPeripheralService::getStartHandle () const
{
	return 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

uint16 CocoaGattPeripheralService::getStopHandle () const
{
	return 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CocoaGattPeripheralService::addInclude (IGattPeripheralService* service)
{
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CocoaGattPeripheralService::startAdvertising ()
{
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CocoaGattPeripheralService::stopAdvertising ()
{
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CocoaGattPeripheralService::close ()
{
	for(auto& characteristic : characteristics)
	{
		delete characteristic;
	}
	stopAdvertising ();
}

//************************************************************************************************
// CocoaGattPeripheral
//************************************************************************************************

CocoaGattPeripheral::CocoaGattPeripheral ()
: nextCreateServiceCallId (0),
  users (0)
{
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CocoaGattPeripheral::startup ()
{
	if(users++ == 0)
	{
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ErrorCode CocoaGattPeripheral::createServiceAsync (Core::UIDRef uuid)
{
	return kError_NoError;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CocoaGattPeripheral::shutdown ()
{
	if(--users == 0)
	{
		for(auto& service : services)
		{
			service->close ();
			delete service;
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IGattPeripheralService* CocoaGattPeripheral::getService (int index) const
{
	if(services.isValidIndex (index))
	{
		return services[index];
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CocoaGattPeripheral::getNumServices () const
{
	return services.count ();
}
