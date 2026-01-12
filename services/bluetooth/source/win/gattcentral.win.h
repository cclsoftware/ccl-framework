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
// Filename    : gattcentral.win.h
// Description : Bluetooth LE Gatt Central Windows
//
//************************************************************************************************

#ifndef _gattcentral_win_h
#define _gattcentral_win_h

#include "core/public/devices/coregattcentral.h"

#include <winrt/Windows.Devices.Bluetooth.GenericAttributeProfile.h>
#include <winrt/Windows.Devices.Bluetooth.Advertisement.h>
#include <winrt/Windows.Devices.Bluetooth.h>
#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.Storage.Streams.h>

#include "core/public/coreobserver.h"

#include "ccl/base/object.h"

#include "ccl/public/text/cstring.h"
#include "ccl/public/text/cclstring.h"
#include "ccl/public/collections/vector.h"
#include "ccl/public/plugins/icoreplugin.h"

#include "meta/generated/cpp/bluetooth-constants-generated.h"

//************************************************************************************************
// Shorter names for WinRT types
//************************************************************************************************

template <class T>
using WinRTIAsyncOperation = winrt::Windows::Foundation::IAsyncOperation<T>;
using WinRTAsyncStatus = winrt::Windows::Foundation::AsyncStatus;
using WinRTIInspectable = winrt::Windows::Foundation::IInspectable;

using WinRTBluetoothAddressType = winrt::Windows::Devices::Bluetooth::BluetoothAddressType;
using WinRTBluetoothConnectionStatus = winrt::Windows::Devices::Bluetooth::BluetoothConnectionStatus;
using WinRTBluetoothLEAdvertisementReceivedEventArgs = winrt::Windows::Devices::Bluetooth::Advertisement::BluetoothLEAdvertisementReceivedEventArgs;
using WinRTBluetoothLEAdvertisementWatcher = winrt::Windows::Devices::Bluetooth::Advertisement::BluetoothLEAdvertisementWatcher;
using WinRTBluetoothLEAdvertisementWatcherStatus = winrt::Windows::Devices::Bluetooth::Advertisement::BluetoothLEAdvertisementWatcherStatus;
using WinRTBluetoothLEAdvertisementWatcherStoppedEventArgs = winrt::Windows::Devices::Bluetooth::Advertisement::BluetoothLEAdvertisementWatcherStoppedEventArgs;
using WinRTBluetoothLEDevice = winrt::Windows::Devices::Bluetooth::BluetoothLEDevice;

using WinRTGattCharacteristic = winrt::Windows::Devices::Bluetooth::GenericAttributeProfile::GattCharacteristic;
using WinRTGattCharacteristicsResult = winrt::Windows::Devices::Bluetooth::GenericAttributeProfile::GattCharacteristicsResult;
using WinRTGattClientCharacteristicConfigurationDescriptorValue = winrt::Windows::Devices::Bluetooth::GenericAttributeProfile::GattClientCharacteristicConfigurationDescriptorValue;
using WinRTGattCommunicationStatus = winrt::Windows::Devices::Bluetooth::GenericAttributeProfile::GattCommunicationStatus;
using WinRTGattDescriptor = winrt::Windows::Devices::Bluetooth::GenericAttributeProfile::GattDescriptor;
using WinRTGattDescriptorsResult = winrt::Windows::Devices::Bluetooth::GenericAttributeProfile::GattDescriptorsResult;
using WinRTGattDeviceService = winrt::Windows::Devices::Bluetooth::GenericAttributeProfile::GattDeviceService;
using WinRTGattDeviceServicesResult = winrt::Windows::Devices::Bluetooth::GenericAttributeProfile::GattDeviceServicesResult;
using WinRTGattReadResult = winrt::Windows::Devices::Bluetooth::GenericAttributeProfile::GattReadResult;
using WinRTGattValueChangedEventArgs = winrt::Windows::Devices::Bluetooth::GenericAttributeProfile::GattValueChangedEventArgs;
using WinRTGattWriteResult = winrt::Windows::Devices::Bluetooth::GenericAttributeProfile::GattWriteResult;

namespace CCL {
namespace Bluetooth {

using namespace Core::Bluetooth;

class WindowsGattCentral;

//************************************************************************************************
// WindowsGattCentralDescriptor
//************************************************************************************************

class WindowsGattCentralDescriptor: public Object,
									public IGattCentralDescriptor
{
public:
	DECLARE_CLASS (WindowsGattCentralDescriptor, Object)

	WindowsGattCentralDescriptor (WinRTGattDescriptor winrtDescriptorArg = nullptr);
	~WindowsGattCentralDescriptor ();

	// Object
	void CCL_API notify (ISubject* subject, MessageRef msg) override;

	// IGattCentralDescription
	Core::ErrorCode readAsync () override;
	Core::ErrorCode writeAsync (const uint8 valueBuffer[], int valueSize) override;

	DEFINE_OBSERVER (IGattCentralDescriptorObserver)

private:
	WinRTGattDescriptor winrtDescriptor;

	WinRTIAsyncOperation<WinRTGattReadResult> readOperation;
	WinRTIAsyncOperation<WinRTGattCommunicationStatus> writeOperation;

	void onReadCompleted (WinRTIAsyncOperation<WinRTGattReadResult> op, const WinRTAsyncStatus status);
	void onWriteCompleted (WinRTIAsyncOperation<WinRTGattCommunicationStatus> op, const WinRTAsyncStatus status);
	void handleReadCompleted (const WinRTGattReadResult& result);
	void handleWriteCompleted (const WinRTGattCommunicationStatus& status);
};

//************************************************************************************************
// WindowsGattCentralCharacteristic
//************************************************************************************************

class WindowsGattCentralCharacteristic: public Object,
										public IGattCentralCharacteristic
{
public:
	DECLARE_CLASS_ABSTRACT (WindowsGattCentralCharacteristic, Object)

	WindowsGattCentralCharacteristic (WinRTGattCharacteristic winrtCharacteristicArg = nullptr);
	~WindowsGattCentralCharacteristic ();

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

	DEFINE_OBSERVER (IGattCentralCharacteristicObserver)

private:
	WinRTGattCharacteristic winrtCharacteristic;

	Vector<WindowsGattCentralDescriptor*> descriptors;
	Vector<UIDBytes> getDescriptorsFilter;
	WinRTGattClientCharacteristicConfigurationDescriptorValue cccd;

	WinRTGattCharacteristic::ValueChanged_revoker valueChangedRevoker;
	WinRTIAsyncOperation<WinRTGattWriteResult> changeCCCDOperation;
	WinRTIAsyncOperation<WinRTGattDescriptorsResult> getDescriptorsOperation;
	WinRTIAsyncOperation<WinRTGattReadResult> readOperation;
	WinRTIAsyncOperation<WinRTGattCommunicationStatus> writeOperation;

	void onChangeCCCDCompleted (WinRTIAsyncOperation<WinRTGattWriteResult> op, const WinRTAsyncStatus status);
	void onGetDescriptorsCompleted (WinRTIAsyncOperation<WinRTGattDescriptorsResult> op, const WinRTAsyncStatus status);
	void onReadCompleted (WinRTIAsyncOperation<WinRTGattReadResult> op, const WinRTAsyncStatus status);
	void onWriteCompleted (WinRTIAsyncOperation<WinRTGattCommunicationStatus> op, const WinRTAsyncStatus status);
	void onValueChanged (const WinRTGattCharacteristic& characteristic, const WinRTGattValueChangedEventArgs& args);

	void handleChangeCCCDCompleted (const WinRTGattWriteResult& result);
	void handleGetDescriptorsCompleted (const WinRTGattDescriptorsResult& result);
	void handleReadCompleted (const WinRTGattReadResult& result);
	void handleWriteCompleted (const WinRTGattCommunicationStatus& status);
	void handleValueChanged (const WinRTGattCharacteristic& characteristic, const WinRTGattValueChangedEventArgs& args);

	Core::ErrorCode changeCCCD (WinRTGattClientCharacteristicConfigurationDescriptorValue cccd);
	void deleteDescriptors ();
};

//************************************************************************************************
// WindowsGattCentralService
//************************************************************************************************

class WindowsGattCentralService: public Object,
								 public IGattCentralService
{
public:
	DECLARE_CLASS_ABSTRACT (WindowsGattCentralService, Object)

	WindowsGattCentralService (WinRTGattDeviceService winrtServiceArg = nullptr);
	~WindowsGattCentralService ();

	void cancelConnectionAttempt ();

	// Object
	void CCL_API notify (ISubject* subject, MessageRef msg) override;

	// IGattCentralService
	const UIDBytes& getServiceId () const override;
	int getNumIncludedServices () const override;
	IGattCentralService* getIncludedService (int index) const override;
	Core::ErrorCode getCharacteristicsAsync (const IDFilter& characteristicFilter) override;

	DEFINE_OBSERVER (IGattCentralServiceObserver)

private:
	WinRTGattDeviceService winrtService;

	UIDBytes serviceId;
	Vector<WindowsGattCentralCharacteristic*> characteristics;
	Vector<UIDBytes> getCharacteristicsFilter;

	WinRTIAsyncOperation<WinRTGattCharacteristicsResult> getCharacteristicsOperation;

	void onGetCharacteristicsCompleted (WinRTIAsyncOperation<WinRTGattCharacteristicsResult> op, const WinRTAsyncStatus status);
	void handleGetCharacteristicsCompleted (const WinRTGattCharacteristicsResult& result);

	void deleteCharacteristics ();
};

//************************************************************************************************
// WindowsGattCentralDevice
//************************************************************************************************

class WindowsGattCentralDevice: public Object,
								public IGattCentralDevice
{
public:
	DECLARE_CLASS_ABSTRACT (WindowsGattCentralDevice, Object)

	WindowsGattCentralDevice (WindowsGattCentral* central, uint64 bluetoothAddress,
							  WinRTBluetoothAddressType bluetoothAddressType, CStringRef manufacturerData);
	~WindowsGattCentralDevice () override;

	PROPERTY_VARIABLE (ConnectionState, connectionState, ConnectionState)

	void close ();

	uint64 getBluetoothAddress () const;
	Core::ErrorCode requestServices ();
	void cancelConnectionAttempt ();

	// Object
	void CCL_API notify (ISubject* subject, MessageRef msg) override;

	// IGattCentralDevice
	CStringPtr getIdentifier () const override;
	CStringPtr getName () const override;
	CStringPtr getManufacturerData () const override;
	tbool isConnected () const override;
	Core::ErrorCode setConnectionMode (ConnectionMode connectionMode) override;
	Core::ErrorCode getServicesAsync () override;

	DEFINE_OBSERVER (IGattCentralDeviceObserver)

private:
	WindowsGattCentral* central;

	WinRTBluetoothLEDevice winrtDevice;

	uint64 bluetoothAddress;
	WinRTBluetoothAddressType bluetoothAddressType;
	MutableCString identifier;
	MutableCString name;
	MutableCString manufacturerData;

	Vector<WindowsGattCentralService*> services;

	WinRTIAsyncOperation<WinRTBluetoothLEDevice> fromBluetoothAddressOperation;
	WinRTIAsyncOperation<WinRTGattDeviceServicesResult> getGattServicesOperation;
	WinRTBluetoothLEDevice::ConnectionStatusChanged_revoker connectionStatusChangedRevoker;

	void onFromBluetoothAddressCompleted (WinRTIAsyncOperation<WinRTBluetoothLEDevice> op, const WinRTAsyncStatus status);
	void onGetGattServicesCompleted (WinRTIAsyncOperation<WinRTGattDeviceServicesResult> op, const WinRTAsyncStatus status);
	void onConnectionStatusChanged (const WinRTBluetoothLEDevice& winrtDevice, const WinRTIInspectable& args);
	void handleFromBluetoothAddressCompleted (const WinRTBluetoothLEDevice& winrtDevice);
	void handleGetGattServicesCompleted (const WinRTGattDeviceServicesResult& result);

	void deleteServices ();
};

//************************************************************************************************
// WindowsGattCentral
//************************************************************************************************

class WindowsGattCentral: public CorePropertyHandler<IGattCentral, Object, IObject>
{
public:
	DECLARE_CLASS_ABSTRACT (WindowsGattCentral, Object)

	WindowsGattCentral ();
	~WindowsGattCentral ();

	void close (WindowsGattCentralDevice* device);

	void notifyDeviceFound (WindowsGattCentralDevice* device);
	void notifyConnectionStatusChanged (WindowsGattCentralDevice* device, WinRTBluetoothConnectionStatus status);
	void notifyGattServicesAvailable (WindowsGattCentralDevice* device);

	// Object
	void CCL_API notify (ISubject* subject, MessageRef msg) override;

	// IGattCentral
	GattCentralState getState () const override;
	Core::ErrorCode startScanning (const IDFilter& serviceFilter, const GattCentralScanOptions& scanOptions = {}) override;
	Core::ErrorCode stopScanning () override;
	Core::ErrorCode connectAsync (IGattCentralDevice* device, tbool autoReconnect) override;
	Core::ErrorCode disconnectAsync (IGattCentralDevice* device) override;

	DEFINE_OBSERVER (IGattCentralObserver)

private:
	GattCentralState centralState;
	Vector<WindowsGattCentralDevice*> devices;

	WinRTBluetoothLEAdvertisementWatcher advertisementWatcher;
	WinRTBluetoothLEAdvertisementWatcher::Received_revoker receivedRevoker;
	WinRTBluetoothLEAdvertisementWatcher::Stopped_revoker stoppedRevoker;

	void updateCentralState (GattCentralState newState);

	void onAdvertisementReceived (const WinRTBluetoothLEAdvertisementWatcher& watcher, const WinRTBluetoothLEAdvertisementReceivedEventArgs& args);
	void onAdvertisementStopped (const WinRTBluetoothLEAdvertisementWatcher& watcher, const WinRTBluetoothLEAdvertisementWatcherStoppedEventArgs& args);
	void handleAdvertisementReceived (const WinRTBluetoothLEAdvertisementReceivedEventArgs& args);

	WindowsGattCentralDevice* findDeviceByAddress (uint64 bluetoothAddress) const;
	void cleanupDevices ();
};

} // namespace Bluetooth
} // namespace CCL

#endif // _gattcentral_win_h
