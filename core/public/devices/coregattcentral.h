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
// Filename    : core/public/devices/coregattcentral.h
// Description : Bluetooth GATT Central Interfaces
//
//************************************************************************************************

#ifndef _coregattcentral_h
#define _coregattcentral_h

#include "core/public/devices/coregattshared.h"

namespace Core {
namespace Bluetooth {

/**
	The IGattCentral interface enables communicating with Bluetooth Low Energy peripherals.

	A GATT (General Attribute Profile) server or peripheral is a device that contains a database
	of attributes that can be read and written.

	A GATT client or central is a device that connects to peripherals and reads and writes data to
	and from GATT servers.

	The IGattCentral structure consists of multiple layers:
	- IGattCentral: The device manager for GATT devices
	- IGattCentralDevice: A device provides any number of services
	- IGattCentralService: A service contains any number of characteristics
	- IGattCentralCharacteristic: An attribute that can be written to and/or read from
	- IGattCentralDescriptor: An attribute that contains meta-data for a characteristic

	Since many function calls are asynchronous, each of these interfaces has a matching observer
	interface. If and only if a call to IExample::doSomethingAsync() returns no error, it	responds
	with the callback IIExampleObserver::onDoSomethingCompleted(). Multiple simultaneous calls to
	asynchronous functions on the same object are not allowed (kError_InvalidState).

	All functions must be called from the main thread. The callbacks are also called from the main
	thread.
*/

//************************************************************************************************
// IDFilter
//************************************************************************************************

struct IDFilter
{
	UIDBytes* ids = nullptr;
	int numIds = 0;

	bool contains (UIDRef uid) const
	{
		for(int i = 0; i < numIds; i++)
			if(ids[i].equals (uid))
				return true;

		return false;
	}
};

//************************************************************************************************
// TIDFilter
//************************************************************************************************

template <int kNumIds>
struct TIDFilter: IDFilter
{
	UIDBytes idData[kNumIds];

	TIDFilter (const UIDBytes (&_idData)[kNumIds])
	{
		this->ids = idData;
		this->numIds = kNumIds;
		for(int i = 0; i < kNumIds; i++)
			idData[i] = _idData[i];
	}
};

template <>
struct TIDFilter<0>: IDFilter
{};

//************************************************************************************************
// IGattCentralAttributeObserver
//************************************************************************************************

struct IGattCentralAttributeObserver
{
	/** Callback for readAsync(). */
	virtual void onReadCompleted (const uint8 valueBuffer[], int valueSize, ErrorCode errorCode) = 0;

	/** Callback for writeAsync(). */
	virtual void onWriteCompleted (ErrorCode errorCode) = 0;
};

//************************************************************************************************
// IGattCentralAttribute
//************************************************************************************************

struct IGattCentralAttribute
{
	/** Request the current value of the attribute. */
	virtual ErrorCode readAsync () = 0;

	/** Write a new value to the attribute. */
	virtual ErrorCode writeAsync (const uint8 valueBuffer[], int valueSize) = 0;
};

//************************************************************************************************
// IGattCentralDescriptorObserver
//************************************************************************************************

struct IGattCentralDescriptorObserver: IGattCentralAttributeObserver
{
};

//************************************************************************************************
// IGattCentralDescriptor
//************************************************************************************************

struct IGattCentralDescriptor: IGattCentralAttribute
{
	/** Start receiving callbacks. */
	virtual void addObserver (IGattCentralDescriptorObserver* observer) = 0;

	/** Stop receiving callbacks. */
	virtual void removeObserver (IGattCentralDescriptorObserver* observer) = 0;
};

//************************************************************************************************
// IGattCentralCharacteristicObserver
//************************************************************************************************

struct IGattCentralCharacteristicObserver: IGattCentralAttributeObserver
{
	/** Callback for getDescriptorsAsync(). */
	virtual void onGetDescriptorsCompleted (IGattCentralDescriptor* descriptors[], int numDescriptors, ErrorCode errorCode) = 0;

	/** Callback for subscribeAsync(). */
	virtual void onSubscribeCompleted (ErrorCode errorCode) = 0;

	/** Callback for unsubscribeAsync(). */
	virtual void onUnsubscribeCompleted (ErrorCode errorCode) = 0;

	/** If this characteristic has been subscribed to:
		This function is called whever the value of this characteristic changes
		by other actors. If this GATT central changes the value with
		writeAsync(), it responds with onWriteCompleted() instead. */
	virtual void onNotificationReceived (const uint8 valueBuffer[], int valueSize) = 0;
};

//************************************************************************************************
// IGattCentralCharacteristic
//************************************************************************************************

struct IGattCentralCharacteristic: IGattCentralAttribute
{
	/** Get the id of this characteristic. */
	virtual UIDBytes getUid () const = 0;

	/** Get the properties.
		These properties indicate which operations this characteristic supports. */
	virtual CharacteristicProperties getProperties () const = 0;

	/** Retrieve all descriptors of this characteristic.
		If the descriptorFilter is not empty, it will only return the descriptors
		whose ids are in the descriptorFilter. */
	virtual ErrorCode getDescriptorsAsync (const IDFilter& descriptorFilter) = 0;

	/** Subscribe to characteristic value updates.
		It calls onNotificationReceived() whenever the value changes. */
	virtual ErrorCode subscribeAsync () = 0;

	/** Unsubscribe from characteristic value updates. */
	virtual ErrorCode unsubscribeAsync () = 0;

	/** Start receiving callbacks. */
	virtual void addObserver (IGattCentralCharacteristicObserver* observer) = 0;

	/** Stop receiving callbacks. */
	virtual void removeObserver (IGattCentralCharacteristicObserver* observer) = 0;
};

//************************************************************************************************
// IGattCentralServiceObserver
//************************************************************************************************

struct IGattCentralServiceObserver
{
	/** Callback for getCharacteristicsAsync(). */
	virtual void onGetCharacteristicsCompleted (IGattCentralCharacteristic* characteristics[], int numCharacteristics, ErrorCode errorCode) = 0;
};

//************************************************************************************************
// IGattCentralService
//************************************************************************************************

struct IGattCentralService
{
	/** Get the id of this service. */
	virtual const UIDBytes& getServiceId () const = 0;

	/** A service might include secondary dependant services.
		This feature is rarely used. */
	virtual int getNumIncludedServices () const = 0;

	/** A service might include secondary dependant services.
		This feature is rarely used. */
	virtual IGattCentralService* getIncludedService (int index) const = 0;

	/** Retrieve all characteristics of this service.
		If the characteristicFilter is not empty, it will only return the
		characteristics whose ids are in the characteristicFilter. */
	virtual ErrorCode getCharacteristicsAsync (const IDFilter& characteristicFilter) = 0;

	/** Start receiving callbacks. */
	virtual void addObserver (IGattCentralServiceObserver* observer) = 0;

	/** Stop receiving callbacks. */
	virtual void removeObserver (IGattCentralServiceObserver* observer) = 0;
};

//************************************************************************************************
// IGattCentralDeviceObserver
//************************************************************************************************

struct IGattCentralDeviceObserver
{
	/** Callback for getServicesAsync(). */
	virtual void onGetServicesCompleted (IGattCentralService* services[], int numServices, ErrorCode errorCode) = 0;
};

//************************************************************************************************
// IGattCentralDevice
//************************************************************************************************

struct IGattCentralDevice
{
	DEFINE_ENUM (ConnectionMode)
	{
		kBalanced = 0, ///< balanced (default)
		kPowerSaving,  ///< optimized for low energy consumption
		kThroughput	   ///< optimized for high performance
	};

	/** Get a unique string for this device. */
	virtual CStringPtr getIdentifier () const = 0;

	/** Get the user-facing name that the device included in its
		advertisement. */
	virtual CStringPtr getName () const = 0;

	/** Get the manufacturer-specific string that the device included in its
		advertisement. */
	virtual CStringPtr getManufacturerData () const = 0;

	/** Check if this device is (still) connected.
		Connections are handled by the IGattCentral interface, not the device.
		This is set to true after onConnectCompleted() returns with no error.
		This is set to false after onDisconnectCompleted() returns. */
	virtual tbool isConnected () const = 0;

	/** Increase performance at the cost of energy efficiency or vice versa. */
	virtual ErrorCode setConnectionMode (ConnectionMode connectionMode) = 0;

	/** Retrieve all services of this device.
		This function may only be called if the device is connected. */
	virtual ErrorCode getServicesAsync () = 0;

	/** Start receiving callbacks. */
	virtual void addObserver (IGattCentralDeviceObserver* observer) = 0;

	/** Stop receiving callbacks. */
	virtual void removeObserver (IGattCentralDeviceObserver* observer) = 0;
};

//************************************************************************************************
// GattCentralState
//************************************************************************************************

DEFINE_ENUM (GattCentralState)
{
	kInitializing = 0, ///< adapter state is not known yet (application needs to wait for state change)
	kStateUnknown,	   ///< adapter state cannot be determined (application may continue but must be prepared for operations to fail)
	kNotSupported,	   ///< adapter not present or Bluetooth LE not supported (application should not attempt to use Bluetooth)
	kPermissionDenied, ///< permission to access adapter is denied by operating system (application may ask user to grant permission)
	kPoweredOff,	   ///< adapter has been turned off by user (application may ask user to enable Bluetooth)
	kPoweredOn		   ///< adapter is enabled and ready to use
};

//************************************************************************************************
// GattCentralScanOptions
//************************************************************************************************

struct GattCentralScanOptions
{
	DEFINE_ENUM (ScanMode)
	{
		kBalanced = 0, ///< balanced (default)
		kPowerSaving,  ///< optimized for low energy consumption
		kLowLatency	   ///< optimized for high performance
	};

	ScanMode kScanMode = kBalanced;

	int kAdvertisementTimeout = 5000;
};

//************************************************************************************************
// IGattCentralObserver
//************************************************************************************************

struct IGattCentralObserver
{
	/** This function is called unsolicitedly whenever the global state of the
		IGattCentral interface changes. Scan and connect methods may only be
		called when the state is kPoweredOn. */
	virtual void onStateChanged (GattCentralState state) = 0;

	/** While scanning for devices, an advertisement for a previously
		undiscovered device has been received. This GattCentral instance has not
		yet communicated with the device or attempted to connect to it.
		This callback creates a new device instance. The lifetime of this
		instance ends with either onDeviceRemoved() or close() */
	virtual void onDeviceAdded (IGattCentralDevice* device) = 0;

	/** While scanning for devices, no advertisements for a previously
		discovered device have been received for at least kAdvertisementTimeout
		milliseconds and no connection to this device has been attempted or
		established. The lifetime of the device ends after this function
		returns. */
	virtual void onDeviceRemoved (IGattCentralDevice* device) = 0;

	/** While scanning for device, a previously discovered (but not connected)
		device changed its name or manufacturer data. */
	virtual void onDeviceUpdated (IGattCentralDevice* device) = 0;

	/** When connecting to a device using connectAsync(), a connection attempt
		has either failed or been successful. This method is never called
		unsolicitedly. If a device connected automatically (for example because
		of the autoReconnect flag), it responds with onConnectionEstablished()
		instead. */
	virtual void onConnectCompleted (IGattCentralDevice* device, ErrorCode errorCode) = 0;

	/** When disconnecting from a device using disconnectAsync(), an existing
		connection has been terminated. This method is never called
		unsolicitedly. */
	virtual void onDisconnectCompleted (IGattCentralDevice* device, ErrorCode errorCode) = 0;

	/** A connection to a device has been automatically reestablished after
		a short while without a previous call to connectAsync(). If the
		connection can't be restored automatically, it will be removed entirely
		with onDeviceRemoved() instead. */
	virtual void onConnectionRestored (IGattCentralDevice* device) = 0;

	/** The GattCentral has started listening for advertisements.
		This callback may be called significantly later than the startScanning()
		function that triggered it, or not at all in case of an error. */
	virtual void onScanningStarted () = 0;

	/** The GattCentral has stopped listening for advertisements. */
	virtual void onScanningStopped () = 0;
};

//************************************************************************************************
// IGattCentral
/** Top-level interface to handle creation of device connections. */
//************************************************************************************************

struct IGattCentral: IPropertyHandler
{
	/** Returns the current state of the bluetooth adapter. Scan and connect
		methods may only be called when the state is kPoweredOn. */
	virtual GattCentralState getState () const = 0;

	/** Listen for advertisements from Bluetooth LE devices.
		If the serviceFilter is not empty, the device needs to provide all
		services in the serviceFilter. When receiving an advertisement from a
		previously unknown device, onDeviceAdded() is called. */
	virtual ErrorCode startScanning (const IDFilter& serviceFilter, const GattCentralScanOptions& scanOptions = {}) = 0;

	/** Stop listening for advertisements. */
	virtual ErrorCode stopScanning () = 0;

	/** Connects to a discovered device and establishing the communication with
		the device. */
	virtual ErrorCode connectAsync (IGattCentralDevice* device, tbool autoReconnect) = 0;

	/** Disconnects a connected device, but keeps it around for reconnecting
		to it in the future without having to scan again. */
	virtual ErrorCode disconnectAsync (IGattCentralDevice* device) = 0;

	/** Start receiving callbacks. */
	virtual void addObserver (IGattCentralObserver* observer) = 0;

	/** Stop receiving callbacks. */
	virtual void removeObserver (IGattCentralObserver* observer) = 0;

	static const InterfaceID kIID = FOUR_CHAR_ID ('B', 'T', 'G', 'C');
};

//************************************************************************************************
// IGattCentralFactory
//************************************************************************************************

struct IGattCentralFactory: IPropertyHandler
{
	/** Create an instance of the IGattCentral interface. Multiple instances
		may be used at the same time. */
	virtual IGattCentral* createGattCentral () = 0;

	static const InterfaceID kIID = FOUR_CHAR_ID ('G', 'A', 'C', 'F');
};

} // namespace Bluetooth
} // namespace Core

#endif // _coregattcentral_h
