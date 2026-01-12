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
// Filename    : gattcentral.win.cpp
// Description : Bluetooth LE Gatt Central Windows
//
//************************************************************************************************

#define DEBUG_LOG 1
#define CCL_ENABLE_EXCEPTION_HANDLING 1

#include "gattcentral.win.h"
#include "gattshared.win.h"

#include "ccl/base/message.h"

using namespace Core;
using namespace Core::Errors;

using namespace CCL;
using namespace CCL::Bluetooth;

using WinRTBuffer = winrt::Windows::Storage::Streams::Buffer;
using WinRTBluetoothConnectionStatus = winrt::Windows::Devices::Bluetooth::BluetoothConnectionStatus;
using WinRTBluetoothLEScanningMode = winrt::Windows::Devices::Bluetooth::Advertisement::BluetoothLEScanningMode;

//************************************************************************************************
// WindowsGattCentralDescriptor
//************************************************************************************************

DEFINE_CLASS_HIDDEN (WindowsGattCentralDescriptor, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

WindowsGattCentralDescriptor::WindowsGattCentralDescriptor (WinRTGattDescriptor winrtDescriptorArg)
: winrtDescriptor (winrtDescriptorArg)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

WindowsGattCentralDescriptor::~WindowsGattCentralDescriptor ()
{
	if(readOperation != nullptr)
	{
		readOperation.Cancel ();
		readOperation = nullptr;
	}
	if(writeOperation != nullptr)
	{
		writeOperation.Cancel ();
		writeOperation = nullptr;
	}

	cancelSignals ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WindowsGattCentralDescriptor::notify (ISubject* subject, MessageRef msg)
{
	if(msg == "readCompleted")
	{
		ASSERT (msg.getArgCount () == 1)
		if(auto argument = TypedMessageArgument<WinRTGattReadResult>::cast (msg[0]))
			handleReadCompleted (argument->getPayload ());
	}
	else if(msg == "writeCompleted")
	{
		ASSERT (msg.getArgCount () == 1)
		if(auto argument = TypedMessageArgument<WinRTGattCommunicationStatus>::cast (msg[0]))
			handleWriteCompleted (argument->getPayload ());
	}
	else
	{
		SuperClass::notify (subject, msg);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ErrorCode WindowsGattCentralDescriptor::readAsync ()
{
	if(readOperation && readOperation.Status () == WinRTAsyncStatus::Started)
		return kError_InvalidState;

	TRY
	{
		readOperation = winrtDescriptor.ReadValueAsync ();
		readOperation.Completed ({this, &WindowsGattCentralDescriptor::onReadCompleted});
	}
	EXCEPT
	{
		CCL_WARN ("Exception on reading value of Gatt descriptor", 0)
		return kError_Failed;
	}

	return kError_NoError;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ErrorCode WindowsGattCentralDescriptor::writeAsync (const uint8 valueBuffer[], int valueSize)
{
	if(writeOperation && writeOperation.Status () == WinRTAsyncStatus::Started)
		return kError_InvalidState;

	WinRTBuffer buffer (valueSize);
	memcpy (buffer.data (), valueBuffer, valueSize);
	buffer.Length (valueSize);

	TRY
	{
		writeOperation = winrtDescriptor.WriteValueAsync (buffer);
		writeOperation.Completed ({this, &WindowsGattCentralDescriptor::onWriteCompleted});
	}
	EXCEPT
	{
		CCL_WARN ("Exception on writing value of Gatt descriptor", 0)
		return kError_Failed;
	}

	return kError_NoError;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WindowsGattCentralDescriptor::onReadCompleted (WinRTIAsyncOperation<WinRTGattReadResult> op, const WinRTAsyncStatus status)
{
	if(status != WinRTAsyncStatus::Started)
		(NEW Message ("readCompleted", TypedMessageArgument<WinRTGattReadResult>::make (op.GetResults ())))->post (this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WindowsGattCentralDescriptor::onWriteCompleted (WinRTIAsyncOperation<WinRTGattCommunicationStatus> op, const WinRTAsyncStatus status)
{
	if(status != WinRTAsyncStatus::Started)
		(NEW Message ("writeCompleted", TypedMessageArgument<WinRTGattCommunicationStatus>::make (op.GetResults ())))->post (this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WindowsGattCentralDescriptor::handleReadCompleted (const WinRTGattReadResult& result)
{
	ASSERT (readOperation != nullptr)
	if(readOperation == nullptr)
		return;
	readOperation = nullptr;

	uint8* data = nullptr;
	uint32 length = 0;
	ErrorCode errorCode = toErrorCode (result.Status ());
	if(errorCode == kError_NoError)
	{
		data = result.Value ().data ();
		length = result.Value ().Length ();
	}
	observers.notify (&IGattCentralDescriptorObserver::onReadCompleted, data, length, errorCode);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WindowsGattCentralDescriptor::handleWriteCompleted (const WinRTGattCommunicationStatus& status)
{
	ASSERT (writeOperation != nullptr)
	if(writeOperation == nullptr)
		return;
	writeOperation = nullptr;

	observers.notify (&IGattCentralDescriptorObserver::onWriteCompleted, toErrorCode (status));
}

//************************************************************************************************
// WindowsGattCentralCharacteristic
//************************************************************************************************

DEFINE_CLASS_HIDDEN (WindowsGattCentralCharacteristic, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

WindowsGattCentralCharacteristic::WindowsGattCentralCharacteristic (WinRTGattCharacteristic winrtCharacteristicArg)
: winrtCharacteristic (winrtCharacteristicArg),
  cccd (WinRTGattClientCharacteristicConfigurationDescriptorValue::None)
{
	valueChangedRevoker = winrtCharacteristic.ValueChanged (winrt::auto_revoke, {this, &WindowsGattCentralCharacteristic::onValueChanged});
}

//////////////////////////////////////////////////////////////////////////////////////////////////

WindowsGattCentralCharacteristic::~WindowsGattCentralCharacteristic ()
{
	if(changeCCCDOperation)
		changeCCCDOperation.Cancel ();
	if(getDescriptorsOperation)
		getDescriptorsOperation.Cancel ();
	if(readOperation)
		readOperation.Cancel ();
	if(writeOperation)
		writeOperation.Cancel ();

	deleteDescriptors ();
	cancelSignals ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API WindowsGattCentralCharacteristic::notify (ISubject* subject, MessageRef msg)
{
	if(msg == "changeCCCDCompleted")
	{
		ASSERT (msg.getArgCount () == 1)
		if(auto result = TypedMessageArgument<WinRTGattWriteResult>::cast (msg[0]))
			handleChangeCCCDCompleted (result->getPayload ());
	}
	else if(msg == "getDescriptorsCompleted")
	{
		ASSERT (msg.getArgCount () == 1)
		if(auto result = TypedMessageArgument<WinRTGattDescriptorsResult>::cast (msg[0]))
			handleGetDescriptorsCompleted (result->getPayload ());
	}
	else if(msg == "readCompleted")
	{
		ASSERT (msg.getArgCount () == 1)
		if(auto result = TypedMessageArgument<WinRTGattReadResult>::cast (msg[0]))
			handleReadCompleted (result->getPayload ());
	}
	else if(msg == "valueChanged")
	{
		ASSERT (msg.getArgCount () == 2)
		auto characteristic = TypedMessageArgument<WinRTGattCharacteristic>::cast (msg[0]);
		auto argument = TypedMessageArgument<WinRTGattValueChangedEventArgs>::cast (msg[1]);
		if(characteristic && argument)
			handleValueChanged (characteristic->getPayload (), argument->getPayload ());
	}
	else if(msg == "writeCompleted")
	{
		ASSERT (msg.getArgCount () == 1)
		if(auto status = TypedMessageArgument<WinRTGattCommunicationStatus>::cast (msg[0]))
			handleWriteCompleted (status->getPayload ());
	}
	else
	{
		SuperClass::notify (subject, msg);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

UIDBytes WindowsGattCentralCharacteristic::getUid () const
{
	return fromWinrtGuid (winrtCharacteristic.Uuid ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CharacteristicProperties WindowsGattCentralCharacteristic::getProperties () const
{
	return static_cast<CharacteristicProperties> (winrtCharacteristic.CharacteristicProperties ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ErrorCode WindowsGattCentralCharacteristic::getDescriptorsAsync (const IDFilter& descriptorFilter)
{
	if(getDescriptorsOperation && getDescriptorsOperation.Status () == WinRTAsyncStatus::Started)
		return kError_InvalidState;

	getDescriptorsFilter.empty ();
	for(int i = 0; i < descriptorFilter.numIds; i++)
		getDescriptorsFilter.add (descriptorFilter.ids[i]);

	TRY
	{
		getDescriptorsOperation = winrtCharacteristic.GetDescriptorsAsync ();
		getDescriptorsOperation.Completed ({this, &WindowsGattCentralCharacteristic::onGetDescriptorsCompleted});
	}
	EXCEPT
	{
		CCL_WARN ("Exception on getting descriptors of Gatt characteristic", 0)
		return kError_Failed;
	}

	return kError_NoError;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ErrorCode WindowsGattCentralCharacteristic::readAsync ()
{
	if(readOperation && readOperation.Status () == WinRTAsyncStatus::Started)
		return kError_InvalidState;

	TRY
	{
		readOperation = winrtCharacteristic.ReadValueAsync ();
		readOperation.Completed ({this, &WindowsGattCentralCharacteristic::onReadCompleted});
	}
	EXCEPT
	{
		CCL_WARN ("Exception on reading value of Gatt characteristic", 0)
		return kError_Failed;
	}

	return kError_NoError;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ErrorCode WindowsGattCentralCharacteristic::subscribeAsync ()
{
	return changeCCCD (WinRTGattClientCharacteristicConfigurationDescriptorValue::Notify);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ErrorCode WindowsGattCentralCharacteristic::unsubscribeAsync ()
{
	return changeCCCD (WinRTGattClientCharacteristicConfigurationDescriptorValue::None);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ErrorCode WindowsGattCentralCharacteristic::writeAsync (const uint8 valueBuffer[], int valueSize)
{
	if(writeOperation && writeOperation.Status () == WinRTAsyncStatus::Started)
		return kError_InvalidState;

	WinRTBuffer buffer (valueSize);
	memcpy (buffer.data (), valueBuffer, valueSize);
	buffer.Length (valueSize);

	TRY
	{
		writeOperation = winrtCharacteristic.WriteValueAsync (buffer);
		writeOperation.Completed ({this, &WindowsGattCentralCharacteristic::onWriteCompleted});
	}
	EXCEPT
	{
		CCL_WARN ("Exception on writing value of Gatt characteristic", 0)
		return kError_Failed;
	}

	return kError_NoError;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WindowsGattCentralCharacteristic::onChangeCCCDCompleted (WinRTIAsyncOperation<WinRTGattWriteResult> op, const WinRTAsyncStatus status)
{
	if(status != WinRTAsyncStatus::Started)
		(NEW Message ("changeCCCDCompleted", TypedMessageArgument<WinRTGattWriteResult>::make (op.GetResults ())))->post (this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WindowsGattCentralCharacteristic::onGetDescriptorsCompleted (WinRTIAsyncOperation<WinRTGattDescriptorsResult> op, const WinRTAsyncStatus status)
{
	if(status != WinRTAsyncStatus::Started)
		(NEW Message ("getDescriptorsCompleted", TypedMessageArgument<WinRTGattDescriptorsResult>::make (op.GetResults ())))->post (this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WindowsGattCentralCharacteristic::onReadCompleted (WinRTIAsyncOperation<WinRTGattReadResult> op, const WinRTAsyncStatus status)
{
	if(status != WinRTAsyncStatus::Started)
		(NEW Message ("readCompleted", TypedMessageArgument<WinRTGattReadResult>::make (op.GetResults ())))->post (this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WindowsGattCentralCharacteristic::onWriteCompleted (WinRTIAsyncOperation<WinRTGattCommunicationStatus> op, const WinRTAsyncStatus status)
{
	if(status != WinRTAsyncStatus::Started)
		(NEW Message ("writeCompleted", TypedMessageArgument<WinRTGattCommunicationStatus>::make (op.GetResults ())))->post (this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WindowsGattCentralCharacteristic::onValueChanged (const WinRTGattCharacteristic& characteristic, const WinRTGattValueChangedEventArgs& args)
{
	(NEW Message ("valueChanged", TypedMessageArgument<WinRTGattCharacteristic>::make (characteristic), TypedMessageArgument<WinRTGattValueChangedEventArgs>::make (args)))->post (this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WindowsGattCentralCharacteristic::handleChangeCCCDCompleted (const WinRTGattWriteResult& result)
{
	switch(cccd)
	{
	case WinRTGattClientCharacteristicConfigurationDescriptorValue::Indicate :
		CCL_WARN ("[%s] Indicate is not supported", __func__)
		break;
	case WinRTGattClientCharacteristicConfigurationDescriptorValue::Notify :
		observers.notify (&IGattCentralCharacteristicObserver::onSubscribeCompleted, toErrorCode (result.Status ()));
		break;
	case WinRTGattClientCharacteristicConfigurationDescriptorValue::None :
		observers.notify (&IGattCentralCharacteristicObserver::onUnsubscribeCompleted, toErrorCode (result.Status ()));
		break;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WindowsGattCentralCharacteristic::handleGetDescriptorsCompleted (const WinRTGattDescriptorsResult& result)
{
	ASSERT (getDescriptorsOperation != nullptr)
	if(getDescriptorsOperation == nullptr)
		return;
	getDescriptorsOperation = nullptr;

	Vector<IGattCentralDescriptor*> iDescriptors;
	ErrorCode errorCode = toErrorCode (result.Status ());
	if(errorCode == kError_NoError)
	{
		auto winrtDescriptors = result.Descriptors ();
		int numDescriptors = winrtDescriptors.Size ();

		deleteDescriptors ();
		for(int i = 0; i < numDescriptors; i++)
			if(getDescriptorsFilter.isEmpty () || getDescriptorsFilter.contains (fromWinrtGuid (winrtDescriptors.GetAt (i).Uuid ())))
				descriptors.add (NEW WindowsGattCentralDescriptor (winrtDescriptors.GetAt (i)));

		for(WindowsGattCentralDescriptor* descriptor : descriptors)
			iDescriptors.add (descriptor);
	}

	getDescriptorsFilter.empty ();
	getDescriptorsOperation = nullptr;
	observers.notify (&IGattCentralCharacteristicObserver::onGetDescriptorsCompleted, iDescriptors.getItems (), iDescriptors.count (), errorCode);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WindowsGattCentralCharacteristic::handleReadCompleted (const WinRTGattReadResult& result)
{
	uint8* data = nullptr;
	uint32 length = 0;
	ErrorCode errorCode = toErrorCode (result.Status ());
	if(errorCode == kError_NoError)
	{
		data = result.Value ().data ();
		length = result.Value ().Length ();
	}

	readOperation = nullptr;
	observers.notify (&IGattCentralCharacteristicObserver::onReadCompleted, data, length, errorCode);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WindowsGattCentralCharacteristic::handleValueChanged (const WinRTGattCharacteristic& characteristic, const WinRTGattValueChangedEventArgs& args)
{
	observers.notify (
		&IGattCentralCharacteristicObserver::onNotificationReceived,
		args.CharacteristicValue ().data (),
		args.CharacteristicValue ().Length ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WindowsGattCentralCharacteristic::handleWriteCompleted (const WinRTGattCommunicationStatus& status)
{
	observers.notify (&IGattCentralCharacteristicObserver::onWriteCompleted, toErrorCode (status));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ErrorCode WindowsGattCentralCharacteristic::changeCCCD (WinRTGattClientCharacteristicConfigurationDescriptorValue cccd)
{
	if(changeCCCDOperation && changeCCCDOperation.Status () == WinRTAsyncStatus::Started)
		return kError_InvalidState;

	this->cccd = cccd;
	TRY
	{
		changeCCCDOperation = winrtCharacteristic.WriteClientCharacteristicConfigurationDescriptorWithResultAsync (cccd);
		changeCCCDOperation.Completed ({this, &WindowsGattCentralCharacteristic::onChangeCCCDCompleted});
	}
	EXCEPT
	{
		CCL_WARN ("Exception on writing CCCD of Gatt characteristic", 0)
		return kError_Failed;
	}

	return kError_NoError;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WindowsGattCentralCharacteristic::deleteDescriptors ()
{
	for(WindowsGattCentralDescriptor* descriptor : descriptors)
		delete descriptor;
	descriptors.removeAll ();
}

//************************************************************************************************
// WindowsGattCentralService
//************************************************************************************************

DEFINE_CLASS_HIDDEN (WindowsGattCentralService, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

WindowsGattCentralService::WindowsGattCentralService (WinRTGattDeviceService winrtServiceArg)
: winrtService (winrtServiceArg),
  serviceId (kNullUID)
{
	if(winrtService)
		serviceId = fromWinrtGuid (winrtService.Uuid ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

WindowsGattCentralService::~WindowsGattCentralService ()
{
	if(getCharacteristicsOperation)
		getCharacteristicsOperation.Cancel ();
	cancelSignals ();

	deleteCharacteristics ();

	TRY
	{
		winrtService.Close ();
	}
	EXCEPT
	{
		CCL_WARN ("Exception on closing BT LE service", 0)
	}
	winrtService = nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WindowsGattCentralService::cancelConnectionAttempt ()
{
	if(getCharacteristicsOperation && getCharacteristicsOperation.Status () == WinRTAsyncStatus::Started)
		getCharacteristicsOperation.Cancel ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API WindowsGattCentralService::notify (ISubject* subject, MessageRef msg)
{
	CCL_PRINTF ("[%s] %s\n", __func__, msg.getID ().str ());
	if(msg == "getCharacteristicsCompleted")
	{
		ASSERT (msg.getArgCount () == 1)
		if(auto argument = TypedMessageArgument<WinRTGattCharacteristicsResult>::cast (msg[0]))
			handleGetCharacteristicsCompleted (argument->getPayload ());
	}
	else
	{
		SuperClass::notify (subject, msg);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ErrorCode WindowsGattCentralService::getCharacteristicsAsync (const IDFilter& characteristicFilter)
{
	if(getCharacteristicsOperation && getCharacteristicsOperation.Status () == WinRTAsyncStatus::Started)
		return kError_InvalidState;

	getCharacteristicsFilter.empty ();
	for(int i = 0; i < characteristicFilter.numIds; i++)
		getCharacteristicsFilter.add (characteristicFilter.ids[i]);

	TRY
	{
		getCharacteristicsOperation = winrtService.GetCharacteristicsAsync ();
		getCharacteristicsOperation.Completed ({this, &WindowsGattCentralService::onGetCharacteristicsCompleted});
	}
	EXCEPT
	{
		CCL_WARN ("Exception on getting characteristics of Gatt service", 0)
		return kError_Failed;
	}
	return kError_NoError;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WindowsGattCentralService::onGetCharacteristicsCompleted (WinRTIAsyncOperation<WinRTGattCharacteristicsResult> op, const WinRTAsyncStatus status)
{
	if(status == WinRTAsyncStatus::Completed)
		(NEW Message ("getCharacteristicsCompleted", TypedMessageArgument<WinRTGattCharacteristicsResult>::make (op.GetResults ())))->post (this);
	else
		getCharacteristicsOperation = nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WindowsGattCentralService::handleGetCharacteristicsCompleted (const WinRTGattCharacteristicsResult& result)
{
	Vector<IGattCentralCharacteristic*> iCharacteristics;
	ErrorCode errorCode = toErrorCode (result.Status ());

	if(errorCode == kError_NoError)
	{
		auto winrtCharacteristics = result.Characteristics ();
		int numCharacteristics = winrtCharacteristics.Size ();

		deleteCharacteristics ();
		for(int i = 0; i < numCharacteristics; i++)
			if(getCharacteristicsFilter.isEmpty () || getCharacteristicsFilter.contains (fromWinrtGuid (winrtCharacteristics.GetAt (i).Uuid ())))
				characteristics.add (NEW WindowsGattCentralCharacteristic (winrtCharacteristics.GetAt (i)));

		for(WindowsGattCentralCharacteristic* characteristic : characteristics)
			iCharacteristics.add (characteristic);
	}

	getCharacteristicsFilter.empty ();
	getCharacteristicsOperation = nullptr;
	observers.notify (&IGattCentralServiceObserver::onGetCharacteristicsCompleted, iCharacteristics.getItems (), iCharacteristics.count (), errorCode);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const UIDBytes& WindowsGattCentralService::getServiceId () const
{
	return serviceId;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int WindowsGattCentralService::getNumIncludedServices () const
{
	return 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IGattCentralService* WindowsGattCentralService::getIncludedService (int index) const
{
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WindowsGattCentralService::deleteCharacteristics ()
{
	for(WindowsGattCentralCharacteristic* characteristic : characteristics)
		delete characteristic;
	characteristics.removeAll ();
}

//************************************************************************************************
// WindowsGattCentralDevice
//************************************************************************************************

DEFINE_CLASS_HIDDEN (WindowsGattCentralDevice, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

WindowsGattCentralDevice::WindowsGattCentralDevice (WindowsGattCentral* central, uint64 bluetoothAddress, WinRTBluetoothAddressType bluetoothAddressType, CStringRef manufacturerData)
: central (central),
  winrtDevice (nullptr),
  bluetoothAddress (bluetoothAddress),
  bluetoothAddressType (bluetoothAddressType),
  identifier (nullptr),
  name (nullptr),
  manufacturerData (manufacturerData),
  connectionState (kConnectionStateUnavailable)
{
	fromBluetoothAddressOperation = WinRTBluetoothLEDevice::FromBluetoothAddressAsync (bluetoothAddress, bluetoothAddressType);
	fromBluetoothAddressOperation.Completed ({this, &WindowsGattCentralDevice::onFromBluetoothAddressCompleted});
}

//////////////////////////////////////////////////////////////////////////////////////////////////

WindowsGattCentralDevice::~WindowsGattCentralDevice ()
{
	close ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

uint64 WindowsGattCentralDevice::getBluetoothAddress () const
{
	return bluetoothAddress;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ErrorCode WindowsGattCentralDevice::requestServices ()
{
	if(!services.isEmpty ())
	{
		central->notifyGattServicesAvailable (this);
		return kError_NoError;
	}

	if(getGattServicesOperation && getGattServicesOperation.Status () == WinRTAsyncStatus::Started)
		return kError_InvalidState;

	TRY
	{
		getGattServicesOperation = winrtDevice.GetGattServicesAsync (winrt::Windows::Devices::Bluetooth::BluetoothCacheMode::Uncached);
		getGattServicesOperation.Completed ({this, &WindowsGattCentralDevice::onGetGattServicesCompleted});
	}
	EXCEPT
	{
		CCL_WARN ("Exception on getting Gatt services", 0)
		return kError_Failed;
	}

	return kError_NoError;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WindowsGattCentralDevice::cancelConnectionAttempt ()
{
	CCL_PRINTF ("[%s]\n", __func__);
	if(getGattServicesOperation && getGattServicesOperation.Status () == WinRTAsyncStatus::Started)
		getGattServicesOperation.Cancel ();

	for(WindowsGattCentralService* service : services)
		service->cancelConnectionAttempt ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API WindowsGattCentralDevice::notify (ISubject* subject, MessageRef msg)
{
	CCL_PRINTF ("[%s] %s\n", __func__, msg.getID ().str ());
	if(msg == "getGattServicesCompleted")
	{
		ASSERT (msg.getArgCount () == 1)
		if(auto argument = TypedMessageArgument<WinRTGattDeviceServicesResult>::cast (msg[0]))
			handleGetGattServicesCompleted (argument->getPayload ());
	}
	else if(msg == "fromBluetoothAddressCompleted")
	{
		ASSERT (msg.getArgCount () == 1)
		if(auto argument = TypedMessageArgument<WinRTBluetoothLEDevice>::cast (msg[0]))
			handleFromBluetoothAddressCompleted (argument->getPayload ());
	}
	else if(msg == "connectionStatusChanged")
	{
		central->notifyConnectionStatusChanged (this, winrtDevice.ConnectionStatus ());
	}
	else
	{
		SuperClass::notify (subject, msg);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CStringPtr WindowsGattCentralDevice::getIdentifier () const
{
	return identifier.str ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CStringPtr WindowsGattCentralDevice::getName () const
{
	return name.str ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CStringPtr WindowsGattCentralDevice::getManufacturerData () const
{
	return manufacturerData.str ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool WindowsGattCentralDevice::isConnected () const
{
	return connectionState == kConnectionStateConnected;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ErrorCode WindowsGattCentralDevice::setConnectionMode (ConnectionMode connectionMode)
{
	// Only works with winsdk version > 10.0.22000.0
	using BluetoothLEPreferredConnectionParameters = winrt::Windows::Devices::Bluetooth::BluetoothLEPreferredConnectionParameters;

	TRY
	{
		switch(connectionMode)
		{
		case kBalanced :
			winrtDevice.RequestPreferredConnectionParameters (BluetoothLEPreferredConnectionParameters::Balanced ());
			break;

		case kPowerSaving :
			winrtDevice.RequestPreferredConnectionParameters (BluetoothLEPreferredConnectionParameters::PowerOptimized ());
			break;

		case kThroughput :
			winrtDevice.RequestPreferredConnectionParameters (BluetoothLEPreferredConnectionParameters::ThroughputOptimized ());
			break;
		}
	}
	EXCEPT
	{
		CCL_WARN ("Exception on changing preferred BT LE connection parameters", 0)
		return kError_Failed;
	}
	return ErrorCodes::kError_NoError;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ErrorCode WindowsGattCentralDevice::getServicesAsync ()
{
	CCL_PRINTF ("%s\n", __func__);
	if(winrtDevice == nullptr || connectionState != kConnectionStateConnected)
		return kError_InvalidState;

	Vector<IGattCentralService*> iServices;
	for(WindowsGattCentralService* service : services)
		iServices.add (service);
	observers.notify (&IGattCentralDeviceObserver::onGetServicesCompleted, iServices.getItems (), iServices.count (), kError_NoError);
	return kError_NoError;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WindowsGattCentralDevice::close ()
{
	cancelSignals ();

	if(fromBluetoothAddressOperation != nullptr)
	{
		fromBluetoothAddressOperation.Cancel ();
		fromBluetoothAddressOperation = nullptr;
	}

	if(getGattServicesOperation != nullptr)
	{
		getGattServicesOperation.Cancel ();
		getGattServicesOperation = nullptr;
	}

	deleteServices ();

	if(winrtDevice != nullptr)
	{
		TRY
		{
			winrtDevice.Close ();
		}
		EXCEPT
		{
			CCL_WARN ("Exception on closing BT LE device", 0)
		}
		winrtDevice = nullptr;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WindowsGattCentralDevice::onFromBluetoothAddressCompleted (WinRTIAsyncOperation<WinRTBluetoothLEDevice> op, const WinRTAsyncStatus status)
{
	if(status == WinRTAsyncStatus::Completed && op.GetResults () != nullptr)
		(NEW Message ("fromBluetoothAddressCompleted", TypedMessageArgument<WinRTBluetoothLEDevice>::make (op.GetResults ())))->post (this);
	else
		fromBluetoothAddressOperation = nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WindowsGattCentralDevice::onGetGattServicesCompleted (WinRTIAsyncOperation<WinRTGattDeviceServicesResult> op, const WinRTAsyncStatus status)
{
	CCL_PRINTF ("[%s] status = %i\n", __func__, status);
	if(status == WinRTAsyncStatus::Completed && op.GetResults () != nullptr)
		(NEW Message ("getGattServicesCompleted", TypedMessageArgument<WinRTGattDeviceServicesResult>::make (op.GetResults ())))->post (this);
	else
		getGattServicesOperation = nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WindowsGattCentralDevice::onConnectionStatusChanged (const WinRTBluetoothLEDevice& winrtDevice, const WinRTIInspectable& args)
{
	(NEW Message ("connectionStatusChanged"))->post (this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WindowsGattCentralDevice::handleFromBluetoothAddressCompleted (const WinRTBluetoothLEDevice& winrtDevice)
{
	ASSERT (this->winrtDevice == nullptr)
	ASSERT (identifier == nullptr)
	ASSERT (name == nullptr)

	if(winrtDevice != nullptr)
	{
		setConnectionState (kConnectionStateDisconnected);

		this->winrtDevice = winrtDevice;
		identifier = String (this->winrtDevice.DeviceId ().c_str ());
		name = String (this->winrtDevice.Name ().c_str ());

		central->notifyDeviceFound (this);

		connectionStatusChangedRevoker = this->winrtDevice.ConnectionStatusChanged (
			winrt::auto_revoke, {this, &WindowsGattCentralDevice::onConnectionStatusChanged});
	}

	fromBluetoothAddressOperation = nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WindowsGattCentralDevice::handleGetGattServicesCompleted (const WinRTGattDeviceServicesResult& result)
{
	ASSERT (services.isEmpty ())
	ErrorCode errorCode = toErrorCode (result.Status ());

	if(errorCode == kError_NoError)
	{
		auto winrtServices = result.Services ();
		int numServices = winrtServices.Size ();

		services.resize (numServices);
		for(int i = 0; i < numServices; i++)
			services.add (NEW WindowsGattCentralService (winrtServices.GetAt (i)));
	}

	getGattServicesOperation = nullptr;

	central->notifyGattServicesAvailable (this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WindowsGattCentralDevice::deleteServices ()
{
	for(WindowsGattCentralService* service : services)
		delete service;
	services.removeAll ();
}

//************************************************************************************************
// WindowsGattCentral
//************************************************************************************************

DEFINE_CLASS_HIDDEN (WindowsGattCentral, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

WindowsGattCentral::WindowsGattCentral ()
: centralState (kPoweredOn)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

WindowsGattCentral::~WindowsGattCentral ()
{
	stopScanning ();
	cancelSignals ();
	cleanupDevices ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WindowsGattCentral::close (WindowsGattCentralDevice* device)
{
	if(device != nullptr)
	{
		int index = devices.index (device);
		if(index != -1)
		{
			observers.notify (&IGattCentralObserver::onDeviceRemoved, device);
			device->close ();
			delete device;
			devices.removeAt (index);
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WindowsGattCentral::notifyDeviceFound (WindowsGattCentralDevice* device)
{
	ASSERT (device != nullptr)
	ASSERT (device->getIdentifier () != nullptr)
	ASSERT (device->getName () != nullptr)
	ASSERT (devices.contains (device))

	observers.notify (&IGattCentralObserver::onDeviceAdded, device);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WindowsGattCentral::notifyConnectionStatusChanged (WindowsGattCentralDevice* device, WinRTBluetoothConnectionStatus status)
{
	CCL_PRINTF ("[%s] %s (state: %i): %s\n", __func__, device->getName (), device->getConnectionState (),
				(status == WinRTBluetoothConnectionStatus::Connected) ? "Connected" : "Disconnected");

	if(device->getConnectionState () == kConnectionStateConnecting)
	{
		// ignore, the connection has been initiated by requesting the services
		// when the services callback returns, the connection state changes to connected
	}
	else if(status == WinRTBluetoothConnectionStatus::Connected)
	{
		if(device->getConnectionState () == kConnectionStateDisconnected)
		{
			// the connection has been established automatically
			device->setConnectionState (kConnectionStateConnected);
			observers.notify (&IGattCentralObserver::onConnectionRestored, device);
		}
		else
		{
			ASSERT (false); // invalid state
		}
	}
	else if(device->getConnectionState () == kConnectionStateConnected)
	{
		// the connection has been terminated automatically
		device->setConnectionState (kConnectionStateDisconnected);
		close (device);
	}
	else
	{
		ASSERT (false); // invalid state
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WindowsGattCentral::notifyGattServicesAvailable (WindowsGattCentralDevice* device)
{
	ASSERT (device->getConnectionState () == kConnectionStateConnecting)
	device->setConnectionState (kConnectionStateConnected);

	observers.notify (&IGattCentralObserver::onConnectCompleted, device, kError_NoError);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API WindowsGattCentral::notify (ISubject* subject, MessageRef msg)
{
	if(msg == "advertisementReceived")
	{
		ASSERT (msg.getArgCount () == 1)
		if(auto argument = TypedMessageArgument<WinRTBluetoothLEAdvertisementReceivedEventArgs>::cast (msg[0]))
			handleAdvertisementReceived (argument->getPayload ());
	}
	else if(msg == "scanningStarted")
	{
		observers.notify (&IGattCentralObserver::onScanningStarted);
	}
	else if(msg == "scanningStopped")
	{
		observers.notify (&IGattCentralObserver::onScanningStopped);
	}
	else
	{
		SuperClass::notify (subject, msg);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

GattCentralState WindowsGattCentral::getState () const
{
	// TODO: How to detect Bluetooth adapter state on Windows?
	//       Could try to get a Windows::Devices::Bluetooth::BluetoothAdapter and query its capabilities,
	//       but the BluetoothAdapter::GetDefaultAsync() method is asynchronous. Also, it's unclear/undocumented
	//       how it behaves when there is no Bluetooth adapter or access to it is denied.
	return centralState;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WindowsGattCentral::updateCentralState (GattCentralState newState)
{
	if(centralState != newState)
	{
		centralState = newState;
		observers.notify (&IGattCentralObserver::onStateChanged, centralState);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ErrorCode WindowsGattCentral::startScanning (const IDFilter& serviceFilter, const GattCentralScanOptions& scanOptions)
{
	// remove unconnected devices
	for(int i = devices.count () - 1; i >= 0; i--)
		if(!devices[i]->isConnected ())
			close (devices[i]);

	// TODO: use scanOptions

	// set service filter
	advertisementWatcher.AdvertisementFilter ().Advertisement ().ServiceUuids ().Clear ();
	for(int i = 0; i < serviceFilter.numIds; i++)
		advertisementWatcher.AdvertisementFilter ().Advertisement ().ServiceUuids ().Append (toWinrtGuid (serviceFilter.ids[i]));

	// active scanning is necessary to find devices with limited discoverability
	advertisementWatcher.ScanningMode (WinRTBluetoothLEScanningMode::Active);

	// register callbacks
	receivedRevoker = advertisementWatcher.Received (winrt::auto_revoke, {this, &WindowsGattCentral::onAdvertisementReceived});
	stoppedRevoker = advertisementWatcher.Stopped (winrt::auto_revoke, {this, &WindowsGattCentral::onAdvertisementStopped});

	TRY
	{
		advertisementWatcher.Start ();
	}
	EXCEPT
	{
		// TODO: Finish error handling...
		//winrt::hresult hr = winrt::to_hresult ();
		//if(hr == HRESULT_FROM_WIN32(ERROR_DEVICE_NOT_AVAILABLE))
		updateCentralState (kPermissionDenied);

		CCL_WARN ("Exception on starting BT LE Advertisement Watcher", 0)
		return kError_Failed;
	}

	(NEW Message ("scanningStarted"))->post (this);
	return kError_NoError;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ErrorCode WindowsGattCentral::stopScanning ()
{
	if(advertisementWatcher.Status () != WinRTBluetoothLEAdvertisementWatcherStatus::Started)
		return kError_InvalidState;

	TRY
	{
		advertisementWatcher.Stop ();
		// observers will be notified in onAdvertisementStopped()
	}
	EXCEPT
	{
		CCL_WARN ("Exception on stopping BT LE Advertisement Watcher", 0)
		return kError_Failed;
	}
	return kError_NoError;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WindowsGattCentral::onAdvertisementReceived (const WinRTBluetoothLEAdvertisementWatcher& watcher, const WinRTBluetoothLEAdvertisementReceivedEventArgs& args)
{
	// only handle advertisements if the advertisement watcher is running
	if(advertisementWatcher.Status () == WinRTBluetoothLEAdvertisementWatcherStatus::Started)
		(NEW Message ("advertisementReceived", TypedMessageArgument<WinRTBluetoothLEAdvertisementReceivedEventArgs>::make (args)))->post (this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WindowsGattCentral::onAdvertisementStopped (const WinRTBluetoothLEAdvertisementWatcher& watcher, const WinRTBluetoothLEAdvertisementWatcherStoppedEventArgs& args)
{
	if(advertisementWatcher.Status () == WinRTBluetoothLEAdvertisementWatcherStatus::Aborted)
	{
		// TODO: jira:UT-309
		// the advertisement watcher aborts immediately if there's no BLE adapter
		// we need to signal this to the application
	}

	receivedRevoker.revoke ();
	stoppedRevoker.revoke ();

	(NEW Message ("scanningStopped"))->post (this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WindowsGattCentral::handleAdvertisementReceived (const WinRTBluetoothLEAdvertisementReceivedEventArgs& args)
{
	uint64 bluetoothAddress = args.BluetoothAddress ();
	WinRTBluetoothAddressType bluetoothAddressType = args.BluetoothAddressType ();

	// ignore this ad if we already know this device
	if(findDeviceByAddress (bluetoothAddress) != nullptr)
		return;

	MutableCString manufacturerData;
	const auto& winrtManufacturerData = args.Advertisement ().ManufacturerData ();
	for(int i = 0; i < winrtManufacturerData.Size (); i++)
	{
		const auto& data = winrtManufacturerData.GetAt (0).Data ();
		manufacturerData.append (reinterpret_cast<char*> (data.data ()), data.Length ());
	}

	CCL_PRINTF ("[%s] %s\n", __func__, manufacturerData.str ());

	devices.add (NEW WindowsGattCentralDevice (this, bluetoothAddress, bluetoothAddressType, manufacturerData));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ErrorCode WindowsGattCentral::connectAsync (IGattCentralDevice* device, tbool autoReconnect)
{
	// TODO: autoReconnect:
	// GattSession.MaintainConnection

	if(device == nullptr)
		return kError_InvalidArgument;

	WindowsGattCentralDevice* windowsDevice = static_cast<WindowsGattCentralDevice*> (device);
	ASSERT (windowsDevice != nullptr && devices.contains (windowsDevice))

	ASSERT (windowsDevice->getConnectionState () == kConnectionStateDisconnected)
	if(windowsDevice->getConnectionState () != kConnectionStateDisconnected)
		return kError_InvalidState;

	windowsDevice->setConnectionState (kConnectionStateConnecting);
	return windowsDevice->requestServices ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ErrorCode WindowsGattCentral::disconnectAsync (IGattCentralDevice* device)
{
	if(device == nullptr)
		return kError_ItemNotFound;

	WindowsGattCentralDevice* windowsDevice = static_cast<WindowsGattCentralDevice*> (device);
	// ASSERT (windowsDevice != nullptr && devices.contains (windowsDevice))

	ASSERT (windowsDevice->getConnectionState () == kConnectionStateConnected ||
			windowsDevice->getConnectionState () == kConnectionStateConnecting)
	if(windowsDevice->getConnectionState () == kConnectionStateConnecting)
		windowsDevice->cancelConnectionAttempt ();

	windowsDevice->setConnectionState (kConnectionStateDisconnected);
	observers.notify (&IGattCentralObserver::onDisconnectCompleted, device, kError_NoError);
	return kError_NoError;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

WindowsGattCentralDevice* WindowsGattCentral::findDeviceByAddress (uint64 bluetoothAddress) const
{
	for(WindowsGattCentralDevice* device : devices)
		if(device->getBluetoothAddress () == bluetoothAddress)
			return device;
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WindowsGattCentral::cleanupDevices ()
{
	while(!devices.isEmpty ())
		close (devices.last ());
}
