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
// Filename    : gattcentral.android.h
// Description : Android Bluetooth LE Gatt Central
//
//************************************************************************************************

#ifndef _gattcentral_android_h
#define _gattcentral_android_h

#include "core/public/devices/coregattcentral.h"
#include "core/public/coreobserver.h"

#include "ccl/base/asyncoperation.h"

#include "ccl/public/text/cstring.h"
#include "ccl/public/text/cclstring.h"
#include "ccl/public/collections/vector.h"
#include "ccl/public/plugins/icoreplugin.h"

#include "ccl/platform/android/cclandroidjni.h"

#include "meta/generated/cpp/bluetooth-constants-generated.h"

namespace CCL {
interface IBuffer;

namespace Bluetooth {

using namespace Core::Bluetooth;

class AndroidGattCentral;
class AndroidGattCentralDevice;

//************************************************************************************************
// AndroidGattCentralDescriptor
//************************************************************************************************

class AndroidGattCentralDescriptor: public Object,
									public IGattCentralDescriptor
{
public:
	DECLARE_CLASS_ABSTRACT (AndroidGattCentralDescriptor, Object)

	AndroidGattCentralDescriptor (AndroidGattCentralDevice* device, jobject descriptor);
	~AndroidGattCentralDescriptor ();

	jobject getJavaObject () const;

	UIDBytes getUid () const;

	// IGattCentralDescription
	Core::ErrorCode readAsync () override;
	Core::ErrorCode writeAsync (const uint8 valueBuffer[], int valueSize) override;

	DEFINE_OBSERVER_OVERRIDE (IGattCentralDescriptorObserver)

private:
	SharedPtr<AndroidGattCentralDevice> gattCentralDevice;
	Core::Java::JniObject bluetoothGattDescriptor;

	AutoPtr<IAsyncOperation> pendingOperation;
};

//************************************************************************************************
// AndroidGattCentralCharacteristic
//************************************************************************************************

class AndroidGattCentralCharacteristic: public Object,
										public IGattCentralCharacteristic
{
public:
	DECLARE_CLASS_ABSTRACT (AndroidGattCentralCharacteristic, Object)

	AndroidGattCentralCharacteristic (AndroidGattCentralDevice* device, jobject characteristic);
	~AndroidGattCentralCharacteristic ();

	jobject getJavaObject () const;

	// Object
	void CCL_API notify (ISubject* subject, MessageRef msg) override;

	// IGattCentralCharacteristic
	UIDBytes getUid () const override;
	CharacteristicProperties getProperties () const override;
	Core::ErrorCode getDescriptorsAsync (const IDFilter& descriptorFilter) override;
	Core::ErrorCode readAsync () override;
	Core::ErrorCode writeAsync (const uint8 valueBuffer[], int valueSize) override;
	Core::ErrorCode subscribeAsync () override;
	Core::ErrorCode unsubscribeAsync () override;

	DEFINE_OBSERVER_OVERRIDE (IGattCentralCharacteristicObserver)

private:
	SharedPtr<AndroidGattCentralDevice> gattCentralDevice;
	Core::Java::JniObject bluetoothGattCharacteristic;

	Vector<AutoPtr<AndroidGattCentralDescriptor>> descriptors;

	AutoPtr<IAsyncOperation> pendingOperation;
};

//************************************************************************************************
// AndroidGattCentralService
//************************************************************************************************

class AndroidGattCentralService: public Object,
								 public IGattCentralService
{
public:
	DECLARE_CLASS_ABSTRACT (AndroidGattCentralService, Object)

	AndroidGattCentralService (AndroidGattCentralDevice* device, jobject service);

	// IGattCentralService
	const UIDBytes& getServiceId () const override;
	int getNumIncludedServices () const override;
	IGattCentralService* getIncludedService (int index) const override;
	Core::ErrorCode getCharacteristicsAsync (const IDFilter& characteristicFilter) override;

	DEFINE_OBSERVER_OVERRIDE (IGattCentralServiceObserver)

private:
	SharedPtr<AndroidGattCentralDevice> gattCentralDevice;
	Core::Java::JniObject bluetoothGattService;

	UIDBytes serviceId;

	Vector<AutoPtr<AndroidGattCentralCharacteristic>> characteristics;
	Vector<AutoPtr<AndroidGattCentralService>> includedServices;
};

//************************************************************************************************
// DeviceDetails
//************************************************************************************************

class DeviceDetails
{
public:
	DeviceDetails (jobject device, jobject record);

	void update (jobject device);

	CStringRef getIdentifier () const { return identifier; }
	CStringRef getName () const { return name; }
	CStringRef getManufacturerData () const { return manufacturerData; }

private:
	MutableCString identifier;
	MutableCString name;
	MutableCString manufacturerData;
};

//************************************************************************************************
// AndroidGattCentralDevice
//************************************************************************************************

class AndroidGattCentralDevice: public Object,
								public IGattCentralDevice
{
public:
	DECLARE_CLASS_ABSTRACT (AndroidGattCentralDevice, Object)

	AndroidGattCentralDevice (AndroidGattCentral* central, const DeviceDetails& deviceDetails);
	~AndroidGattCentralDevice ();

	bool isValid () const;
	Core::ErrorCode connectAsync ();
	Core::ErrorCode disconnectAsync ();

	IAsyncOperation* readDescriptor (AndroidGattCentralDescriptor* descriptor);
	IAsyncOperation* writeDescriptor (AndroidGattCentralDescriptor* descriptor, const uint8 valueBuffer[], int valueSize);

	IAsyncOperation* readCharacteristic (AndroidGattCentralCharacteristic* characteristic);
	IAsyncOperation* writeCharacteristic (AndroidGattCentralCharacteristic* characteristic, const uint8 valueBuffer[], int valueSize);

	IAsyncOperation* setCharacteristicNotification (AndroidGattCentralCharacteristic* characteristic, bool enable);

	void close ();

	// IGattCentralDevice
	Core::CStringPtr getIdentifier () const override;
	Core::CStringPtr getName () const override;
	Core::CStringPtr getManufacturerData () const override;
	tbool isConnected () const override;
	Core::ErrorCode setConnectionMode (ConnectionMode connectionMode) override;
	Core::ErrorCode getServicesAsync () override;

	// Notification handlers
	void onConnectionStateChange (int status, int state);
	void onServicesDiscovered (int status, const Core::Java::JniObjectArray& services);

	void onAttributeRead (int status, const Core::Java::JniByteArray& value);
	void onAttributeWrite (int status);

	void onSubscribeCompleted (int status);

	void onCharacteristicChanged (jobject jCharacteristic, const Core::Java::JniByteArray& value);

	DEFINE_OBSERVER_OVERRIDE (IGattCentralDeviceObserver)

private:
	AndroidGattCentral* central;

	Core::Java::JniObject gattCentralDevice;
	DeviceDetails deviceDetails;
	int connectionState;

	AutoPtr<AsyncOperation> pendingConnect;
	AutoPtr<AsyncOperation> pendingDisconnect;
	AutoPtr<AsyncOperation> pendingOperation;

	Vector<AutoPtr<AndroidGattCentralService>> services;
	Vector<UIDBytes> servicesFilter;

	Vector<SharedPtr<AndroidGattCentralCharacteristic>> subscribedCharacteristics;
};

//************************************************************************************************
// AndroidGattCentral
//************************************************************************************************

class AndroidGattCentral: public CorePropertyHandler<IGattCentral, Object, IObject>
{
public:
	DECLARE_CLASS (AndroidGattCentral, Object)

	AndroidGattCentral ();
	~AndroidGattCentral ();

	jobject getJavaObject () const;

	void close (AndroidGattCentralDevice* device);
	
	// Object
	void CCL_API notify (ISubject* subject, MessageRef msg) override;

	// IGattCentral
	GattCentralState getState () const override;
	Core::ErrorCode startScanning (const IDFilter& serviceFilter, const GattCentralScanOptions& scanOptions = {}) override;
	Core::ErrorCode stopScanning () override;
	Core::ErrorCode connectAsync (IGattCentralDevice* device, tbool autoReconnect) override;
	Core::ErrorCode disconnectAsync (IGattCentralDevice* device) override;

	jobject getDevice (Core::CStringPtr identifier) const;

	// Notification handlers
	void onDeviceFound (jobject device, jobject record);
	void onDeviceLost (jobject device, jobject record);
	void onPermissionsUpdated (PermissionsState permissionsState);
	void onScanningStarted ();
	void onScanningStopped ();

	void onDeviceConnected (Core::ErrorCode result);
	void onDeviceDisconnected (Core::ErrorCode result);
	void onDeviceLost (AndroidGattCentralDevice* device);

	DEFINE_OBSERVER_OVERRIDE (Core::Bluetooth::IGattCentralObserver)

private:
	Core::Java::JniObject gattCentral;
	PermissionsState permissionsState;

	AutoPtr<AsyncOperation> pendingConnect;
	AutoPtr<AsyncOperation> pendingDisconnect;

	Vector<AutoPtr<AndroidGattCentralDevice>> devices;
	Vector<AndroidGattCentralDevice*> connectedDevices;

	void cleanupDevices ();
};

} // namespace Bluetooth
} // namespace CCL

#endif // _gattcentral_android_h
