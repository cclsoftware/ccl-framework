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
// Filename    : gattcentral.cocoa.h
// Description : Bluetooth LE Gatt Central (Core Bluetooth)
//
//************************************************************************************************

#ifndef _gattcentral_cocoa_h
#define _gattcentral_cocoa_h

#include "core/public/devices/coregattcentral.h"
#include "core/public/coreobserver.h"

#include "ccl/base/object.h"

#include "ccl/public/text/cstring.h"
#include "ccl/public/text/cclstring.h"
#include "ccl/public/collections/vector.h"
#include "ccl/public/plugins/icoreplugin.h"

#include "ccl/platform/cocoa/macutils.h"

@class CBDescriptor;
@class CBCharacteristic;
@class CBService;
@class CBPeripheral;
@class CBCentralManager;

@class CentralManagerDelegate;
@class PeripheralDelegate;

namespace CCL {
namespace Bluetooth {

using namespace Core::Bluetooth;

//************************************************************************************************
// CocoaGattCentralDescriptor
//************************************************************************************************

class CocoaGattCentralDescriptor: public Object,
								  public IGattCentralDescriptor
{
public:
	CocoaGattCentralDescriptor (CBDescriptor* descriptor = nullptr);

	CBDescriptor* getDescriptor () const { return descriptor; }

	void onUpdateValue (Core::ErrorCode result);
	void onWriteValue (Core::ErrorCode result);

	// IGattCentralDescription
	Core::ErrorCode writeAsync (const uint8 valueBuffer[], int valueSize) override;
	Core::ErrorCode readAsync () override;

	DEFINE_OBSERVER_OVERRIDE (IGattCentralDescriptorObserver)

private:
	NSObj<CBDescriptor> descriptor;
};

//************************************************************************************************
// CocoaGattCentralCharacteristic
//************************************************************************************************

class CocoaGattCentralCharacteristic: public Object,
									  public IGattCentralCharacteristic
{
public:
	CocoaGattCentralCharacteristic (CBCharacteristic* characteristic = nullptr);

	CBCharacteristic* getCharacteristic () const { return characteristic; }

	void onUpdateNotificationState (Core::ErrorCode result);
	void onUpdateValue (Core::ErrorCode result);
	void onWriteValue (Core::ErrorCode result);
	void onDiscoverDescriptors (Core::ErrorCode result);

	// IGattCentralCharacteristic
	UIDBytes getUid () const override;
	CharacteristicProperties getProperties () const override;
	Core::ErrorCode getDescriptorsAsync (const IDFilter& descriptorFilter) override;
	Core::ErrorCode subscribeAsync () override;
	Core::ErrorCode unsubscribeAsync () override;
	Core::ErrorCode writeAsync (const uint8 valueBuffer[], int valueSize) override;
	Core::ErrorCode readAsync () override;

	DEFINE_OBSERVER_OVERRIDE (IGattCentralCharacteristicObserver)

private:
	NSObj<CBCharacteristic> characteristic;
	Vector<UIDBytes> descriptorFilter;
	Vector<AutoPtr<CocoaGattCentralDescriptor>> descriptors;

    bool readPending;
};

//************************************************************************************************
// CocoaGattCentralService
//************************************************************************************************

class CocoaGattCentralService: public Object,
							   public IGattCentralService
{
public:
	CocoaGattCentralService (CBService* service = nullptr);

	CBService* getService () const { return service; }
	CocoaGattCentralCharacteristic* getCharacteristic (CBCharacteristic* characteristic) const;
	
	void onDiscoverCharacteristics (Core::ErrorCode result);
	
	// IGattCentralService
	const UIDBytes& getServiceId () const override;
	int getNumIncludedServices () const override;
	IGattCentralService* getIncludedService (int index) const override;
	Core::ErrorCode getCharacteristicsAsync (const IDFilter& charactericFilter) override;

	DEFINE_OBSERVER_OVERRIDE (IGattCentralServiceObserver)

private:
	NSObj<CBService> service;
	UIDBytes serviceID;
	Vector<AutoPtr<CocoaGattCentralCharacteristic>> characteristics;
	Vector<AutoPtr<CocoaGattCentralService>> includedServices;
};

//************************************************************************************************
// CocoaGattCentralDevice
//************************************************************************************************

class CocoaGattCentralDevice: public Object,
							  public IGattCentralDevice
{
public:
	CocoaGattCentralDevice (CBPeripheral* device = nullptr, NSDictionary<NSString*,id>* advertisementData = nullptr);
	~CocoaGattCentralDevice ();

	PROPERTY_BOOL (shouldReconnect, ShouldReconnect)
	PROPERTY_BOOL (connectPending, ConnectPending)
	PROPERTY_BOOL (disconnectPending, DisconnectPending)

	CBPeripheral* getPeripheral () const { return peripheral; }
	CocoaGattCentralService* getService (CBService* service) const;

	void onDiscoverServices (Core::ErrorCode result);
	void onDiscoverCharacteristics (CBService* service, Core::ErrorCode result);
	
	void setDeviceInfo (NSDictionary<NSString*,id>* advertisementData);

	// IGattCentralDevice
	CStringPtr getIdentifier () const override;
	CStringPtr getName () const override;
	CStringPtr getManufacturerData () const override;
	tbool isConnected () const override;
	Core::ErrorCode setConnectionMode (ConnectionMode connectionMode) override;
	Core::ErrorCode getServicesAsync () override;

	DEFINE_OBSERVER_OVERRIDE (IGattCentralDeviceObserver)

private:
	NSObj<CBPeripheral> peripheral;
	NSObj<PeripheralDelegate> delegate;
	MutableCString identifier;
	MutableCString name;
	MutableCString manufacturerData;
	Vector<AutoPtr<CocoaGattCentralService>> services;

	void deleteServices ();
};

//************************************************************************************************
// CocoaGattCentral
//************************************************************************************************

class CocoaGattCentral: public CorePropertyHandler<IGattCentral, Object, IObject>
{
public:
	~CocoaGattCentral ();
	
	void onUpdateState (NSInteger state);
	void onDiscoverPeripheral (CBPeripheral* peripheral, NSDictionary<NSString*,id>* advertisementData);
	void onConnectPeripheral (CBPeripheral* peripheral, Core::ErrorCode result);
	void onDisconnectPeripheral (CBPeripheral* peripheral, Core::ErrorCode result);

	// IGattCentral
	GattCentralState getState () const override;
	Core::ErrorCode startScanning (const IDFilter& serviceFilter, const GattCentralScanOptions& scanOptions = {}) override;
	Core::ErrorCode stopScanning () override;
	Core::ErrorCode connectAsync (IGattCentralDevice* device, tbool autoReconnect) override;
	Core::ErrorCode disconnectAsync (IGattCentralDevice* device) override;
	
	DEFINE_OBSERVER_OVERRIDE (IGattCentralObserver)
	
private:
	NSObj<CBCentralManager> centralManager;
	NSObj<CentralManagerDelegate> delegate;
	Vector<AutoPtr<CocoaGattCentralDevice>> discoveredPeripherals;
	
	void initialize ();
	bool isInitialized () const;
	bool isOff () const;
	CocoaGattCentralDevice* getNativeDevice (IGattCentralDevice* iDevice) const;
	CBPeripheral* getPeripheral (IGattCentralDevice* device) const;
	CocoaGattCentralDevice* getNativeDevice (CBPeripheral* peripheral) const;
};

} // namespace Bluetooth
} // namespace CCL

#endif // _gattcentral_cocoa_h
