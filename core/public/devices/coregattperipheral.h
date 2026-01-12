//************************************************************************************************
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
// Filename    : core/public/devices/coregattperipheral.h
// Description : Bluetooth GATT Peripheral Interfaces
//
//************************************************************************************************

#ifndef _coregattperipheral_h
#define _coregattperipheral_h

#include "core/public/devices/coregattshared.h"

namespace Core {
namespace Bluetooth {

/**
	ATTENTION: Interfaces for GATT Peripheral are experimental and not fully implemented
	on all platforms. Do not base production code on it.
*/

//************************************************************************************************
// CharacteristicInfo
//************************************************************************************************

struct CharacteristicInfo
{
	UIDRef uuid = kNullUID;
	CharacteristicProperties properties = 0;
	CStringPtr description = "";
};

//************************************************************************************************
// IGattPeripheralAttributeObserver
//************************************************************************************************

struct IGattPeripheralAttributeObserver
{
	/** Called when a Gatt Central reads from the Attribute, the Observer writes the Attribute
		value to be read into valueBuffer, and the descriptor size into valueSize, capacity is
		kAttributeCapacity (512 bytes). */
	virtual void onRead (uint8* valueBuffer, int& valueSize) = 0;

	/** Called when a Gatt Central writes to the Descriptor, the Observer accepts the descriptor data
		just written from valueBuffer and the number of bytes from valueSize. The number of bytes
		accepted is written back into valueSize. */
	virtual void onWrite (const uint8 valueBuffer[], int& valueSize) = 0;
};

//************************************************************************************************
// IGattPeripheralAttribute
//************************************************************************************************

struct IGattPeripheralAttribute
{};

//************************************************************************************************
// IGattPeripheralDescriptorObserver
//************************************************************************************************

struct IGattPeripheralDescriptorObserver: IGattPeripheralAttributeObserver
{};

//************************************************************************************************
// IGattPeripheralDescriptor
//************************************************************************************************

struct IGattPeripheralDescriptor: IGattPeripheralAttribute
{
	virtual void addObserver (IGattPeripheralDescriptorObserver* observer) = 0;
	
	virtual void removeObserver (IGattPeripheralDescriptorObserver* observer) = 0;
};

//************************************************************************************************
// IGattPeripheralCharacteristicObserver
//************************************************************************************************

struct IGattPeripheralCharacteristicObserver: IGattPeripheralAttributeObserver
{
	/** Signals notification of Centrals of the new characteristic value is complete.*/
	virtual void onNotify () = 0;

	/** Called after a descriptor has been created*/
	virtual void onDescriptorCreated (IGattPeripheralDescriptor* descriptor) = 0;
};

//************************************************************************************************
// IGattPeripheralCharacteristic
//************************************************************************************************

struct IGattPeripheralCharacteristic: IGattPeripheralAttribute
{
	/** Called to notify/indicate Centrals of new Value*/
	virtual void notify (const uint8 valueBuffer[], int valueSize) = 0;
	
	virtual ErrorCode createDescriptorAsync (UIDRef uuid, const uint8 valueBuffer[], int valueSize) = 0;
	
	virtual void addObserver (IGattPeripheralCharacteristicObserver* observer) = 0;
	
	virtual void removeObserver (IGattPeripheralCharacteristicObserver* observer) = 0;
};

//************************************************************************************************
// IGattPeripheralServiceObserver
//************************************************************************************************

struct IGattPeripheralServiceObserver
{
	virtual void onCharacteristicCreated (IGattPeripheralCharacteristic* characteristic) = 0;
};

//************************************************************************************************
// IGattPeripheralService
//************************************************************************************************

struct IGattPeripheralService
{
	virtual uint16 getStartHandle () const = 0;
	
	virtual uint16 getStopHandle () const = 0;

	/** Triggers creation of Characteristic, behavior unreliable when called after startAdvertising, callId output to be passed with Handler*/
	virtual ErrorCode createCharacteristicAsync (const CharacteristicInfo& characteristicInfo) = 0;
		
	virtual void addInclude (IGattPeripheralService* service) = 0;
	
	virtual tbool startAdvertising () = 0;
	
	virtual tbool stopAdvertising () = 0;
	
	virtual void addObserver (IGattPeripheralServiceObserver* observer) = 0;
	
	virtual void removeObserver (IGattPeripheralServiceObserver* observer) = 0;
	
	virtual void close () = 0;
};

//************************************************************************************************
// GattPeripheralStatus
//************************************************************************************************

DEFINE_ENUM (GattPeripheralStatus)
{
	kReady = 0,
	kLEUnsupported = 1,
	kPeripheralUnsupported = 2
};

//************************************************************************************************
// IGattPeripheralObserver
//************************************************************************************************

struct IGattPeripheralObserver
{
	/** Called when startup sequence is completed */
	virtual void onPeripheralChanged (GattPeripheralStatus status) = 0;
	
	/** Called when service is created, implementation is responsible for checking callId matches return value of createService */
	virtual void onServiceCreated (IGattPeripheralService* service, ErrorCode result) = 0;
};

//************************************************************************************************
// IGattPeripheral
/** Top-level interface instantiates BT Adapter and provices access to GATT Peripheral. */
//************************************************************************************************

struct IGattPeripheral: IPropertyHandler
{
	/** Triggers startup sequence if not already started. */
	virtual void startup () = 0;
	
	/** Triggers shutdown sequence if not already started. */
	virtual void shutdown () = 0;

	virtual int getNumServices () const = 0;

	virtual IGattPeripheralService* getService (int index) const = 0;

	/** Triggers creation of Service, instances stored in member vector. returns callId to be passed with Handler*/
	virtual ErrorCode createServiceAsync (UIDRef uuid) = 0;
		
	virtual void addObserver (IGattPeripheralObserver* observer) = 0;
	
	virtual void removeObserver (IGattPeripheralObserver* observer) = 0;

	static const InterfaceID kIID = FOUR_CHAR_ID ('B', 'T', 'G', 'P');
};

//************************************************************************************************
// IGattPeripheralFactory
//************************************************************************************************

struct IGattPeripheralFactory: IPropertyHandler
{
	virtual IGattPeripheral* createGattPeripheral () = 0;

	static const InterfaceID kIID = FOUR_CHAR_ID ('G', 'A', 'P', 'F');
};

} // namespace Bluetooth
} // namespace Core

#endif // _coregattperipheral_h
