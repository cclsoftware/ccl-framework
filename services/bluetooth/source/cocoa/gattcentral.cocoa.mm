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
// Filename    : gattcentral.cocoa.mm
// Description : Bluetooth LE Gatt Central (Core Bluetooth)
//
//************************************************************************************************

#include "gattcentral.cocoa.h"
#include "gattshared.cocoa.h"

#include "core/public/coreuid.h"

#include "ccl/public/base/ccldefpush.h"
#include <CoreBluetooth/CoreBluetooth.h>

using namespace Core;
using namespace Core::Errors;

using namespace CCL;
using namespace CCL::Bluetooth;

static constexpr GattCentralState stateFromManagerState (CBManagerState managerState)
{
	GattCentralState state = kStateUnknown;
	switch(managerState)
	{
	case CBManagerStateUnknown : state = kInitializing; break;
	case CBManagerStateResetting : state = kInitializing; break;
	case CBManagerStateUnsupported : state = kNotSupported; break;
	case CBManagerStateUnauthorized : state = kPermissionDenied; break;
	case CBManagerStatePoweredOff : state = kPoweredOff; break;
	case CBManagerStatePoweredOn : state = kPoweredOn; break;
	}
	
	return state;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

static void getFilterArray (NSMutableArray* filter, const IDFilter& idFilter)
{
	if(idFilter.numIds > 0)
		for(int i = 0; i < idFilter.numIds; i++)
			if(idFilter.ids[i].isValid ())
			{
				CBUUID* serviceID = nil;
				toCBUUID (serviceID, idFilter.ids[i]);
				[filter addObject:serviceID];
			}
}

//************************************************************************************************
// CentralManagerDelegate
//************************************************************************************************

@interface CentralManagerDelegate : NSObject<CBCentralManagerDelegate>
{
	CocoaGattCentral* central;
}

- (instancetype)initWithCentral:(CocoaGattCentral*)central;

@end

//************************************************************************************************
// PeripheralDelegate
//************************************************************************************************

@interface PeripheralDelegate : NSObject<CBPeripheralDelegate>
{
	CocoaGattCentralDevice* device;
}

- (instancetype)initWithDevice:(CocoaGattCentralDevice*)device;

@end

//************************************************************************************************
// CentralManagerDelegate
//************************************************************************************************

@implementation CentralManagerDelegate

- (instancetype)initWithCentral:(CocoaGattCentral*)_central
{
	ASSERT (_central)
	if(self = [super init])
		central = _central;
	
	return self;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (void)centralManagerDidUpdateState:(CBCentralManager*)manager
{
	central->onUpdateState (manager.state);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (void)centralManager:(CBCentralManager*)manager didDiscoverPeripheral:(CBPeripheral*)peripheral advertisementData:(NSDictionary<NSString*,id>*)advertisementData RSSI:(NSNumber*)RSSI
{
	central->onDiscoverPeripheral (peripheral, advertisementData);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (void)centralManager:(CBCentralManager*)manager didConnectPeripheral:(CBPeripheral*)peripheral
{
	central->onConnectPeripheral (peripheral, kError_NoError);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (void)centralManager:(CBCentralManager*)manager didDisconnectPeripheral:(CBPeripheral*)peripheral error:(NSError*)error
{
	central->onDisconnectPeripheral (peripheral, error == nil ? kError_NoError : kError_Failed);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (void)centralManager:(CBCentralManager*)manager didFailToConnectPeripheral:(CBPeripheral*)peripheral error:(NSError*)error
{
	central->onConnectPeripheral (peripheral, kError_Failed);
}

@end

//************************************************************************************************
// PeripheralDelegate
//************************************************************************************************

@implementation PeripheralDelegate

- (instancetype)initWithDevice:(CocoaGattCentralDevice*)_device
{
	ASSERT (_device)
	if(self = [super init])
		device = _device;
	
	return self;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (void)peripheral:(CBPeripheral*)peripheral didDiscoverServices:(NSError*)error
{
	device->onDiscoverServices (error == nil ? kError_NoError : kError_Failed);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (void)peripheral:(CBPeripheral*)peripheral didDiscoverCharacteristicsForService:(CBService*)cbService error:(NSError*)error
{
	if(CocoaGattCentralService* service = device->getService (cbService))
		service->onDiscoverCharacteristics (error == nil ? kError_NoError : kError_Failed);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (void)peripheral:(CBPeripheral*)peripheral didUpdateNotificationStateForCharacteristic:(CBCharacteristic*)cbCharacteristic error:(NSError*)error
{
	if(CocoaGattCentralService* service = device->getService ([cbCharacteristic service]))
		if(CocoaGattCentralCharacteristic* characteristic = service->getCharacteristic (cbCharacteristic))
			characteristic->onUpdateNotificationState (error == nil ? kError_NoError : kError_Failed);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (void)peripheral:(CBPeripheral*)peripheral didUpdateValueForCharacteristic:(CBCharacteristic*)cbCharacteristic error:(NSError*)error
{
	if(CocoaGattCentralService* service = device->getService ([cbCharacteristic service]))
		if(CocoaGattCentralCharacteristic* characteristic = service->getCharacteristic (cbCharacteristic))
			characteristic->onUpdateValue (error == nil ? kError_NoError : kError_Failed);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (void)peripheral:(CBPeripheral *)peripheral didWriteValueForCharacteristic:(CBCharacteristic *)cbCharacteristic error:(NSError *)error
{
	if(CocoaGattCentralService* service = device->getService ([cbCharacteristic service]))
		if(CocoaGattCentralCharacteristic* characteristic = service->getCharacteristic (cbCharacteristic))
			characteristic->onWriteValue (error == nil ? kError_NoError : kError_Failed);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (void)peripheral:(CBPeripheral*)peripheral didDiscoverDescriptorsForCharacteristic:(CBCharacteristic*)cbCharacteristic error:(NSError*)error
{
	if(CocoaGattCentralService* service = device->getService ([cbCharacteristic service]))
		if(CocoaGattCentralCharacteristic* characteristic = service->getCharacteristic (cbCharacteristic))
			characteristic->onDiscoverDescriptors (error == nil ? kError_NoError : kError_Failed);
}

@end

//************************************************************************************************
// CocoaGattCentralDescriptor
//************************************************************************************************

CocoaGattCentralDescriptor::CocoaGattCentralDescriptor (CBDescriptor* _descriptor)
: descriptor (_descriptor)
{
	ASSERT (_descriptor)
	[_descriptor retain]; // take ownership with NSObj AutoPtr
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ErrorCode CocoaGattCentralDescriptor::writeAsync (const uint8 valueBuffer[], int valueSize)
{
	NSObj<NSData> data = [[NSData alloc] initWithBytes:valueBuffer length:valueSize];
	[[[[descriptor characteristic] service] peripheral] writeValue:data forDescriptor:descriptor];

	return kError_NoError;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ErrorCode CocoaGattCentralDescriptor::readAsync ()
{
	[[[[descriptor characteristic] service] peripheral] readValueForDescriptor:descriptor];
	
	return kError_NoError;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CocoaGattCentralDescriptor::onUpdateValue (Core::ErrorCode result)
{
	const unsigned char* bytes = nullptr;
	int length = 0;
	long long integerValue = 0;
	
	if([[descriptor value] isKindOfClass:[NSData class]])
	{
		NSData* data = static_cast<NSData*>([descriptor value]);
		bytes = static_cast<const unsigned char*> ([data bytes]);
		length = (int)[data length];
	}
	else if([[descriptor value] isKindOfClass:[NSString class]])
	{
		NSString* string = static_cast<NSString*>([descriptor value]);
		bytes = reinterpret_cast<const unsigned char*> ([string UTF8String]);
		length = (int)[string lengthOfBytesUsingEncoding:NSUTF8StringEncoding];
		
	}
	else if([[descriptor value] isKindOfClass:[NSNumber class]])
	{
		NSNumber* number = static_cast<NSNumber*>([descriptor value]);
		integerValue = [number longLongValue];
		bytes = reinterpret_cast<const unsigned char*> (&integerValue);
		length = sizeof(integerValue);
	}
	else
	{
		ASSERT (0)
		return;
	}
	
	observers.notify (&IGattCentralDescriptorObserver::onReadCompleted, bytes, length, result);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CocoaGattCentralDescriptor::onWriteValue (Core::ErrorCode result)
{
	observers.notify (&IGattCentralDescriptorObserver::onWriteCompleted, result);
}

//************************************************************************************************
// CocoaGattCentralCharacteristic
//************************************************************************************************

CocoaGattCentralCharacteristic::CocoaGattCentralCharacteristic (CBCharacteristic* _characteristic)
: characteristic (_characteristic),
  readPending (false)
{
	ASSERT (_characteristic)
	[_characteristic retain]; // take ownership with NSObj AutoPtr
}

//////////////////////////////////////////////////////////////////////////////////////////////////

UIDBytes CocoaGattCentralCharacteristic::getUid () const
{
	return fromCBUUID ([characteristic UUID]);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CharacteristicProperties CocoaGattCentralCharacteristic::getProperties () const
{
	return static_cast<CharacteristicProperties> ([characteristic properties]);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ErrorCode CocoaGattCentralCharacteristic::getDescriptorsAsync (const IDFilter& filter)
{
	descriptorFilter.empty ();
	for(int i = 0; i < filter.numIds; i++)
		descriptorFilter.add (filter.ids[i]);

	[[[characteristic service] peripheral] discoverDescriptorsForCharacteristic:characteristic];
	
	return kError_NoError;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ErrorCode CocoaGattCentralCharacteristic::subscribeAsync ()
{
    if(!([characteristic properties] & CBCharacteristicPropertyNotify))
        return kError_Failed;

	[[[characteristic service] peripheral] setNotifyValue:YES forCharacteristic:characteristic];
	
	return kError_NoError;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ErrorCode CocoaGattCentralCharacteristic::unsubscribeAsync ()
{
	[[[characteristic service] peripheral] setNotifyValue:NO forCharacteristic:characteristic];
	
	return kError_NoError;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ErrorCode CocoaGattCentralCharacteristic::writeAsync (const uint8 valueBuffer[], int valueSize)
{
    if(!([characteristic properties] & CBCharacteristicPropertyWrite))
        return kError_Failed;

	NSObj<NSData> data = [[NSData alloc] initWithBytes:valueBuffer length:valueSize];
	[[[characteristic service] peripheral] writeValue:data forCharacteristic:characteristic type:CBCharacteristicWriteWithResponse];

	return kError_NoError;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ErrorCode CocoaGattCentralCharacteristic::readAsync ()
{
    if(!([characteristic properties] & CBCharacteristicPropertyRead))
        return kError_Failed;

	[[[characteristic service] peripheral] readValueForCharacteristic:characteristic];
	readPending = true;
	
	return kError_NoError;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CocoaGattCentralCharacteristic::onUpdateNotificationState (Core::ErrorCode result)
{
	observers.notify (&IGattCentralCharacteristicObserver::onSubscribeCompleted, result);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CocoaGattCentralCharacteristic::onUpdateValue (Core::ErrorCode result)
{
	if(!readPending && [characteristic isNotifying])
		observers.notify (&IGattCentralCharacteristicObserver::onNotificationReceived, static_cast<const unsigned char*> ([characteristic value].bytes), (int)[characteristic value].length);
	else
		observers.notify (&IGattCentralCharacteristicObserver::onReadCompleted, static_cast<const unsigned char*> ([characteristic value].bytes), (int)[characteristic value].length, result);

	readPending = false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CocoaGattCentralCharacteristic::onWriteValue (Core::ErrorCode result)
{
	observers.notify (&IGattCentralCharacteristicObserver::onWriteCompleted, result);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CocoaGattCentralCharacteristic::onDiscoverDescriptors (Core::ErrorCode result)
{
	Vector<IGattCentralDescriptor*> iDescriptors;
	for(CBDescriptor* d in [characteristic descriptors])
		if(descriptorFilter.isEmpty () || descriptorFilter.contains (fromCBUUID (d.UUID)))
			descriptors.add (NEW CocoaGattCentralDescriptor (d));

	for(CocoaGattCentralDescriptor* descriptor : descriptors)
		iDescriptors.add (descriptor);
	
	observers.notify (&IGattCentralCharacteristicObserver::onGetDescriptorsCompleted, iDescriptors.getItems (), iDescriptors.count (), result);
}

//************************************************************************************************
// CocoaGattCentralService
//************************************************************************************************

CocoaGattCentralService::CocoaGattCentralService (CBService* _service)
: service (_service)
{
	ASSERT (_service)
	[_service retain]; // take ownership with NSObj AutoPtr
	
	serviceID = fromCBUUID (_service.UUID);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ErrorCode CocoaGattCentralService::getCharacteristicsAsync (const IDFilter& characteristicFilter)
{
	NSObj<NSMutableArray> filter = nil;
	if(characteristicFilter.numIds > 0)
	{
		filter = [[NSMutableArray alloc] init];
		getFilterArray (filter, characteristicFilter);
	}
	
	[[service peripheral] discoverCharacteristics:filter forService:service];
	
	return kError_NoError;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CocoaGattCentralService::onDiscoverCharacteristics (Core::ErrorCode result)
{
	Vector<IGattCentralCharacteristic*> iCharacteristics;

	for(CBCharacteristic* c in [service characteristics])
		characteristics.add (NEW CocoaGattCentralCharacteristic (c));

	for(CocoaGattCentralCharacteristic* characteristic : characteristics)
		iCharacteristics.add (characteristic);

	observers.notify (&IGattCentralServiceObserver::onGetCharacteristicsCompleted, iCharacteristics.getItems (), iCharacteristics.count (), result);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IGattCentralService* CocoaGattCentralService::getIncludedService (int index) const
{
	// TODO change interface, this needs to be asynchronous (called from the device)
	if(includedServices.isValidIndex (index))
		return includedServices[index];
		
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const UIDBytes& CocoaGattCentralService::getServiceId () const
{
	return serviceID;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CocoaGattCentralService::getNumIncludedServices () const
{
	return includedServices.count ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CocoaGattCentralCharacteristic* CocoaGattCentralService::getCharacteristic (CBCharacteristic* cbCharacteristic) const
{
	return *characteristics.findIf ([&] (CocoaGattCentralCharacteristic* c)
	{
		return c->getCharacteristic () == cbCharacteristic;
	});
}

//************************************************************************************************
// CocoaGattCentralDevice
//************************************************************************************************

CocoaGattCentralDevice::CocoaGattCentralDevice (CBPeripheral* _peripheral, NSDictionary<NSString*,id>* advertisementData)
: peripheral (_peripheral),
  shouldReconnect (false),
  connectPending (false),
  disconnectPending (false)
{
	ASSERT (peripheral)
	[peripheral retain]; //take ownership with NSObj AutoPtr

	delegate = [[PeripheralDelegate alloc] initWithDevice:this];
	[peripheral setDelegate:delegate];
	setDeviceInfo (advertisementData);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CocoaGattCentralDevice::~CocoaGattCentralDevice ()
{
	deleteServices ();
	[peripheral setDelegate:nil];
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CStringPtr CocoaGattCentralDevice::getIdentifier () const
{
	return identifier.str ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CStringPtr CocoaGattCentralDevice::getName () const
{
	return name.str ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CStringPtr CocoaGattCentralDevice::getManufacturerData () const
{
	return manufacturerData.str ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CocoaGattCentralDevice::setDeviceInfo (NSDictionary<NSString*,id>* advertisementData)
{
	identifier = [[NSString stringWithString:[peripheral identifier].UUIDString] cStringUsingEncoding:NSUTF8StringEncoding];
	name = [[peripheral name] cStringUsingEncoding:NSUTF8StringEncoding];

	manufacturerData.empty ();
	if(advertisementData)
		if(NSData* data = advertisementData[CBAdvertisementDataManufacturerDataKey])
			if([data length] > 2)
				manufacturerData.append (reinterpret_cast<const char*>([data bytes]) + 2, int([data length]) - 2); // skip 16 bit manufacturer ID
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Core::ErrorCode CocoaGattCentralDevice::setConnectionMode (ConnectionMode connectionMode)
{
	// there are no modes defined in CoreBluetooth
	return Core::Errors::ErrorCodes::kError_NoError;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CocoaGattCentralDevice::isConnected () const
{
	return [peripheral state] == CBPeripheralStateConnected;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ErrorCode CocoaGattCentralDevice::getServicesAsync ()
{
	deleteServices ();
	
	NSObj<NSMutableArray> filter = nil;
	[peripheral discoverServices:filter];
	
	return kError_NoError;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CocoaGattCentralDevice::onDiscoverServices (Core::ErrorCode result)
{
	Vector<IGattCentralService*> iServices;
	
	if(result == kError_NoError)
	{
		deleteServices ();
		services.resize ((int)[[peripheral services] count]);
		for(CBService* service in [peripheral services])
			services.add (NEW CocoaGattCentralService (service));
		
		for(CocoaGattCentralService* service : services)
			iServices.add (service);
	}

	observers.notify (&IGattCentralDeviceObserver::onGetServicesCompleted, iServices.getItems (), iServices.count (), result);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CocoaGattCentralDevice::deleteServices ()
{
	services.removeAll ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CocoaGattCentralService* CocoaGattCentralDevice::getService (CBService* cbService) const
{
	return *services.findIf ([&] (CocoaGattCentralService* s)
	{
		return s->getService () == cbService;
	});
}
	
//************************************************************************************************
// CocoaGattCentral
//************************************************************************************************

CocoaGattCentral::~CocoaGattCentral ()
{
	if(centralManager)
	{
		[centralManager setDelegate:nil];
		for(CocoaGattCentralDevice* d : discoveredPeripherals)
			[centralManager cancelPeripheralConnection:d->getPeripheral ()];
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CocoaGattCentral::initialize ()
{
	if(isInitialized ())
		return;

	delegate = [[CentralManagerDelegate alloc] initWithCentral:this];

	NSDictionary* options = @
	{
		CBCentralManagerOptionShowPowerAlertKey : @YES
	};

	centralManager = [[CBCentralManager alloc] initWithDelegate:delegate queue:nil options:options];
	ASSERT (centralManager)
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool CocoaGattCentral::isInitialized () const
{
	return centralManager != nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CocoaGattCentral::onUpdateState (NSInteger state)
{
	observers.notify (&IGattCentralObserver::onStateChanged, getState ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CocoaGattCentral::onDiscoverPeripheral (CBPeripheral* peripheral, NSDictionary<NSString*,id>* advertisementData)
{
	if(peripheral == nullptr)
		return;

	CocoaGattCentralDevice* device = getNativeDevice (peripheral);
	if(device)
		device->setDeviceInfo (advertisementData);
	else
	{
		device = NEW CocoaGattCentralDevice (peripheral, advertisementData);
		discoveredPeripherals.add (device);
		observers.notify (&IGattCentralObserver::onDeviceAdded, device);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CocoaGattCentral::onConnectPeripheral (CBPeripheral* peripheral, ErrorCode result)
{
	CocoaGattCentralDevice* device = getNativeDevice (peripheral);
	if(device)
	{
		if(device->isConnectPending ())
		{
			observers.notify (&IGattCentralObserver::onConnectCompleted, device, result);
			device->setDisconnectPending (false);
		}
		else if(result == kError_NoError)
		{
			// this is the case when an accidentially disoconnected device reappears : do not keep the connection alive, just report the device like via advertisement
			device->setDisconnectPending (true);
			[centralManager cancelPeripheralConnection:peripheral];
			observers.notify (&IGattCentralObserver::onDeviceAdded, device);
		}
		device->setConnectPending (false);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CocoaGattCentral::onDisconnectPeripheral (CBPeripheral* peripheral, ErrorCode result)
{
	CocoaGattCentralDevice* device = getNativeDevice (peripheral);
	if(device)
	{
		if(device->isDisconnectPending ())
			observers.notify (&IGattCentralObserver::onDisconnectCompleted, device, result);
		else
		{
			if(device->isShouldReconnect ())
			{
				// try to reconnect, because on iOS a reappearing device does not generate advertisement events
				[centralManager connectPeripheral:peripheral options:nil];
				device->setConnectPending (true);
			}
			observers.notify (&IGattCentralObserver::onDeviceRemoved, device);
		}
		device->setConnectPending (false);
		device->setDisconnectPending (false);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

GattCentralState CocoaGattCentral::getState () const
{
	if(!isInitialized ())
		return kStateUnknown;

	return stateFromManagerState ([centralManager state]);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ErrorCode CocoaGattCentral::startScanning (const IDFilter& serviceFilter, const GattCentralScanOptions& scanOptions)
{
	if(!isInitialized ())
		initialize ();

	if(isOff ())
		return kError_Failed;

	NSObj<NSMutableArray> filter = nullptr;
	if(serviceFilter.numIds > 0)
	{
		filter = [[NSMutableArray alloc] init];
		getFilterArray (filter, serviceFilter);
	}
	[centralManager scanForPeripheralsWithServices:filter options:@{}];
	observers.notify (&IGattCentralObserver::onScanningStarted);

	return kError_NoError;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ErrorCode CocoaGattCentral::stopScanning ()
{
	if(!isInitialized () || isOff ())
		return kError_Failed;

	[centralManager stopScan];
	observers.notify (&IGattCentralObserver::onScanningStopped);

	return kError_NoError;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ErrorCode CocoaGattCentral::connectAsync (IGattCentralDevice* iDevice, tbool autoReconnect)
{
	if(!isInitialized () || isOff ())
		return kError_Failed;

	CocoaGattCentralDevice* device = getNativeDevice (iDevice);
	if(device == nullptr)
		return kError_Failed;

	CBPeripheral* peripheral = device->getPeripheral ();
	if(peripheral == nullptr)
		return kError_Failed;

	device->setShouldReconnect (autoReconnect);
	device->setConnectPending (true);
	device->setDisconnectPending (false);

	[centralManager connectPeripheral:peripheral options:nullptr];

	return kError_NoError;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ErrorCode CocoaGattCentral::disconnectAsync (IGattCentralDevice* iDevice)
{
	if(!isInitialized () || isOff ())
		return kError_Failed;

	CocoaGattCentralDevice* device = getNativeDevice (iDevice);
	if(device == nullptr)
		return kError_Failed;

	CBPeripheral* peripheral = device->getPeripheral ();
	if(peripheral == nullptr)
		return kError_Failed;

	device->setDisconnectPending (true);
	device->setConnectPending (false);

	[centralManager cancelPeripheralConnection:peripheral];

	return kError_NoError;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool CocoaGattCentral::isOff () const
{
	return [centralManager state] < CBManagerStatePoweredOn;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CocoaGattCentralDevice* CocoaGattCentral::getNativeDevice (IGattCentralDevice* iDevice) const
{
	if(iDevice == nullptr)
		return nullptr;

	auto device = discoveredPeripherals.findIf ([&] (CocoaGattCentralDevice* d)
	{
		return d == iDevice;
	});

	if(device)
		return *device;

	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CBPeripheral* CocoaGattCentral::getPeripheral (IGattCentralDevice* iDevice) const
{
	if(CocoaGattCentralDevice* device = getNativeDevice (iDevice))
		return device->getPeripheral ();

	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CocoaGattCentralDevice* CocoaGattCentral::getNativeDevice (CBPeripheral* peripheral) const
{
	auto device = discoveredPeripherals.findIf ([&] (CocoaGattCentralDevice* d)
	{
		return d->getPeripheral() == peripheral;
	});

	if(device)
		return *device;

	return nullptr;
}
