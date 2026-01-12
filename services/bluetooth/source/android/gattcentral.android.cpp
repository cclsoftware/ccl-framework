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
// Filename    : gattcentral.android.cpp
// Description : Android Bluetooth LE Gatt Central
//
//************************************************************************************************

#include "gattcentral.android.h"
#include "gattshared.android.h"

#include "ccl/base/message.h"

#include "ccl/public/base/ibuffer.h"

#include "core/platform/shared/jni/corejniarray.h"

namespace CCL {
namespace Bluetooth {

//////////////////////////////////////////////////////////////////////////////////////////////////
// Constants
//////////////////////////////////////////////////////////////////////////////////////////////////

// Adapter state constants from android.bluetooth.BluetoothAdapter
// https://developer.android.com/reference/android/bluetooth/BluetoothAdapter

constexpr int STATE_NOT_SUPPORTED = 0; // additional value indicating no adapter being present
constexpr int STATE_OFF = 10;
constexpr int STATE_TURNING_ON = 11;
constexpr int STATE_ON = 12;
constexpr int STATE_TURNING_OFF = 13;

// Profile state constants from android.bluetooth.BluetoothProfile
// https://developer.android.com/reference/android/bluetooth/BluetoothProfile

constexpr int STATE_DISCONNECTED = 0;
constexpr int STATE_CONNECTING = 1;
constexpr int STATE_CONNECTED = 2;
constexpr int STATE_DISCONNECTING = 3;

// Connection priority constants from android.bluetooth.BluetoothGatt
// https://developer.android.com/reference/android/bluetooth/BluetoothGatt

constexpr int CONNECTION_PRIORITY_BALANCED = 0;
constexpr int CONNECTION_PRIORITY_HIGH = 1;
constexpr int CONNECTION_PRIORITY_LOW_POWER = 2;

//************************************************************************************************
// android.bluetooth.BluetoothGattDescriptor
//************************************************************************************************

DECLARE_JNI_CLASS (BluetoothGattDescriptor, "android/bluetooth/BluetoothGattDescriptor")
	DECLARE_JNI_METHOD (jobject, getUuid)
END_DECLARE_JNI_CLASS (BluetoothGattDescriptor)

DEFINE_JNI_CLASS (BluetoothGattDescriptor)
	DEFINE_JNI_METHOD (getUuid, "()Ljava/util/UUID;")
END_DEFINE_JNI_CLASS

//************************************************************************************************
// android.bluetooth.BluetoothGattCharacteristic
//************************************************************************************************

DECLARE_JNI_CLASS (BluetoothGattCharacteristic, "android/bluetooth/BluetoothGattCharacteristic")
	DECLARE_JNI_METHOD (jobject, getUuid)
	DECLARE_JNI_METHOD (jobject, getDescriptors)
	DECLARE_JNI_METHOD (int, getProperties)
END_DECLARE_JNI_CLASS (BluetoothGattCharacteristic)

DEFINE_JNI_CLASS (BluetoothGattCharacteristic)
	DEFINE_JNI_METHOD (getUuid, "()Ljava/util/UUID;")
	DEFINE_JNI_METHOD (getDescriptors, "()Ljava/util/List;")
	DEFINE_JNI_METHOD (getProperties, "()I")
END_DEFINE_JNI_CLASS

//************************************************************************************************
// android.bluetooth.BluetoothGattService
//************************************************************************************************

DECLARE_JNI_CLASS (BluetoothGattService, "android/bluetooth/BluetoothGattService")
	DECLARE_JNI_METHOD (jobject, getUuid)
	DECLARE_JNI_METHOD (jobject, getCharacteristics)
	DECLARE_JNI_METHOD (jobject, getIncludedServices)
END_DECLARE_JNI_CLASS (BluetoothGattService)

DEFINE_JNI_CLASS (BluetoothGattService)
	DEFINE_JNI_METHOD (getUuid, "()Ljava/util/UUID;")
	DEFINE_JNI_METHOD (getCharacteristics, "()Ljava/util/List;")
	DEFINE_JNI_METHOD (getIncludedServices, "()Ljava/util/List;")
END_DEFINE_JNI_CLASS

//************************************************************************************************
// android.bluetooth.BluetoothDevice
//************************************************************************************************

DECLARE_JNI_CLASS (BluetoothDevice, "android/bluetooth/BluetoothDevice")
	DECLARE_JNI_METHOD (jstring, getAddress)
	DECLARE_JNI_METHOD (jstring, getName)
END_DECLARE_JNI_CLASS (BluetoothDevice)

DEFINE_JNI_CLASS (BluetoothDevice)
	DEFINE_JNI_METHOD (getAddress, "()Ljava/lang/String;")
	DEFINE_JNI_METHOD (getName, "()Ljava/lang/String;")
END_DEFINE_JNI_CLASS

//************************************************************************************************
// android.bluetooth.le.ScanRecord
//************************************************************************************************

DECLARE_JNI_CLASS (ScanRecord, "android/bluetooth/le/ScanRecord")
	DECLARE_JNI_METHOD (jobject, getManufacturerSpecificData)
END_DECLARE_JNI_CLASS (ScanRecord)

DEFINE_JNI_CLASS (ScanRecord)
	DEFINE_JNI_METHOD (getManufacturerSpecificData, "()Landroid/util/SparseArray;")
END_DEFINE_JNI_CLASS

//************************************************************************************************
// android.util.SparseArray
//************************************************************************************************

DECLARE_JNI_CLASS (SparseArray, "android/util/SparseArray")
	DECLARE_JNI_METHOD (int, size)
	DECLARE_JNI_METHOD (jobject, valueAt, int)
END_DECLARE_JNI_CLASS (SparseArray)

DEFINE_JNI_CLASS (SparseArray)
	DEFINE_JNI_METHOD (size, "()I")
	DEFINE_JNI_METHOD (valueAt, "(I)Ljava/lang/Object;")
END_DEFINE_JNI_CLASS

//************************************************************************************************
// dev.ccl.GattCentralDevice
//************************************************************************************************

DECLARE_JNI_CLASS (GattCentralDevice, "dev/ccl/services/bluetooth/GattCentralDevice")
	DECLARE_JNI_CONSTRUCTOR (construct, jobject, jobject, Android::JniIntPtr)
	DECLARE_JNI_METHOD (bool, connect)
	DECLARE_JNI_METHOD (bool, disconnect)
	DECLARE_JNI_METHOD (bool, requestConnectionPriority, int)
	DECLARE_JNI_METHOD (bool, discoverServices)
	DECLARE_JNI_METHOD (bool, readDescriptor, jobject)
	DECLARE_JNI_METHOD (bool, writeDescriptor, jobject, jbyteArray)
	DECLARE_JNI_METHOD (bool, readCharacteristic, jobject)
	DECLARE_JNI_METHOD (bool, writeCharacteristic, jobject, jbyteArray)
	DECLARE_JNI_METHOD (bool, setCharacteristicNotification, jobject, bool)
	DECLARE_JNI_METHOD (void, close)
END_DECLARE_JNI_CLASS (GattCentralDevice)

DEFINE_JNI_CLASS (GattCentralDevice)
	DEFINE_JNI_CONSTRUCTOR (construct, "(Ldev/ccl/services/bluetooth/GattCentral;Landroid/bluetooth/BluetoothDevice;J)V")
	DEFINE_JNI_METHOD (connect, "()Z")
	DEFINE_JNI_METHOD (disconnect, "()Z")
	DEFINE_JNI_METHOD (requestConnectionPriority, "(I)Z")
	DEFINE_JNI_METHOD (discoverServices, "()Z")
	DEFINE_JNI_METHOD (readDescriptor, "(Landroid/bluetooth/BluetoothGattDescriptor;)Z")
	DEFINE_JNI_METHOD (writeDescriptor, "(Landroid/bluetooth/BluetoothGattDescriptor;[B)Z")
	DEFINE_JNI_METHOD (readCharacteristic, "(Landroid/bluetooth/BluetoothGattCharacteristic;)Z")
	DEFINE_JNI_METHOD (writeCharacteristic, "(Landroid/bluetooth/BluetoothGattCharacteristic;[B)Z")
	DEFINE_JNI_METHOD (setCharacteristicNotification, "(Landroid/bluetooth/BluetoothGattCharacteristic;Z)Z")
	DEFINE_JNI_METHOD (close, "()V")
END_DEFINE_JNI_CLASS

//************************************************************************************************
// dev.ccl.GattCentral
//************************************************************************************************

DECLARE_JNI_CLASS (GattCentral, "dev/ccl/services/bluetooth/GattCentral")
	DECLARE_JNI_CONSTRUCTOR (construct, Android::JniIntPtr)
	DECLARE_JNI_METHOD (int, getState)
	DECLARE_JNI_METHOD (void, startScanning, jobjectArray, int)
	DECLARE_JNI_METHOD (void, stopScanning)
	DECLARE_JNI_METHOD (jobject, getDevice, jstring)
END_DECLARE_JNI_CLASS (GattCentral)

DEFINE_JNI_CLASS (GattCentral)
	DEFINE_JNI_CONSTRUCTOR (construct, "(J)V")
	DEFINE_JNI_METHOD (getState, "()I")
	DEFINE_JNI_METHOD (startScanning, "([Ljava/lang/String;I)V")
	DEFINE_JNI_METHOD (stopScanning, "()V")
	DEFINE_JNI_METHOD (getDevice, "(Ljava/lang/String;)Landroid/bluetooth/BluetoothDevice;")
END_DEFINE_JNI_CLASS

} // namespace Bluetooth
} // namespace CCL

using namespace Core;
using namespace Core::Errors;
using namespace Core::Java;

using namespace CCL;
using namespace CCL::Bluetooth;

//************************************************************************************************
// AndroidGattCentralDescriptor
//************************************************************************************************

DEFINE_CLASS_HIDDEN (AndroidGattCentralDescriptor, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

AndroidGattCentralDescriptor::AndroidGattCentralDescriptor (AndroidGattCentralDevice* device, jobject descriptor)
: gattCentralDevice (device),
  bluetoothGattDescriptor (JniAccessor (), descriptor)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

AndroidGattCentralDescriptor::~AndroidGattCentralDescriptor ()
{
	ASSERT (!pendingOperation)
}

//////////////////////////////////////////////////////////////////////////////////////////////////

jobject AndroidGattCentralDescriptor::getJavaObject () const
{
	return bluetoothGattDescriptor;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

UIDBytes AndroidGattCentralDescriptor::getUid () const
{
	JniAccessor jni;
	LocalRef jUuid (jni, BluetoothGattDescriptor.getUuid (bluetoothGattDescriptor));

	return uidFromJavaUuid (jUuid);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ErrorCode AndroidGattCentralDescriptor::readAsync ()
{
	ASSERT (!pendingOperation)
	if(pendingOperation)
		return kError_InvalidState;

	pendingOperation = gattCentralDevice->readDescriptor (this);

	Promise (return_shared<IAsyncOperation> (pendingOperation)).then ([&] (IAsyncOperation& op)
	{
		pendingOperation.release ();

		if(op.getState () == IAsyncInfo::kCompleted)
		{
			IBuffer* buffer = static_cast<IBuffer*> (op.getResult ().asUnknown ());
			observers.notify (&IGattCentralDescriptorObserver::onReadCompleted,
							  static_cast<uint8*> (buffer->getBufferAddress ()),
							  buffer->getBufferSize (), kError_NoError);
		}
		else
			observers.notify (&IGattCentralDescriptorObserver::onReadCompleted, nullptr, 0, kError_Failed);
	});

	return kError_NoError;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ErrorCode AndroidGattCentralDescriptor::writeAsync (const uint8 valueBuffer[], int valueSize)
{
	ASSERT (!pendingOperation)
	if(pendingOperation)
		return kError_InvalidState;

	pendingOperation = gattCentralDevice->writeDescriptor (this, valueBuffer, valueSize);

	Promise (return_shared<IAsyncOperation> (pendingOperation)).then ([&] (IAsyncOperation& op)
	{
		pendingOperation.release ();
		if(op.getState () == IAsyncInfo::kCompleted)
			observers.notify (&IGattCentralDescriptorObserver::onWriteCompleted, op.getResult ());
		else
			observers.notify (&IGattCentralDescriptorObserver::onWriteCompleted, op.getResult () ? op.getResult () : kError_Failed);
	});

	return kError_NoError;
}

//************************************************************************************************
// AndroidGattCentralCharacteristic
//************************************************************************************************

DEFINE_CLASS_HIDDEN (AndroidGattCentralCharacteristic, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

AndroidGattCentralCharacteristic::AndroidGattCentralCharacteristic (AndroidGattCentralDevice* device, jobject characteristic)
: gattCentralDevice (device),
  bluetoothGattCharacteristic (JniAccessor (), characteristic)
{
	JniAccessor jni;

	// get descriptors
	LocalRef jDescriptors (jni, BluetoothGattCharacteristic.getDescriptors (bluetoothGattCharacteristic));
	int numDescriptors = List.size (jDescriptors);
	for(int i = 0; i < numDescriptors; i++)
	{
		LocalRef jDescriptor (jni, List.get (jDescriptors, i));

		descriptors.add (NEW AndroidGattCentralDescriptor (gattCentralDevice, jDescriptor));
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

AndroidGattCentralCharacteristic::~AndroidGattCentralCharacteristic ()
{
	ASSERT (!pendingOperation)

	cancelSignals ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

jobject AndroidGattCentralCharacteristic::getJavaObject () const
{
	return bluetoothGattCharacteristic;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

UIDBytes AndroidGattCentralCharacteristic::getUid () const
{
	JniAccessor jni;
	LocalRef jUuid (jni, BluetoothGattCharacteristic.getUuid (bluetoothGattCharacteristic));

	return uidFromJavaUuid (jUuid);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CharacteristicProperties AndroidGattCentralCharacteristic::getProperties () const
{
	int properties = BluetoothGattCharacteristic.getProperties (bluetoothGattCharacteristic);

	return static_cast<CharacteristicProperties> (properties);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ErrorCode AndroidGattCentralCharacteristic::getDescriptorsAsync (const IDFilter& descriptorFilter)
{
	// create array of filtered descriptors
	Vector<IGattCentralDescriptor*>* iDescriptors = NEW Vector<IGattCentralDescriptor*>;
	for(AndroidGattCentralDescriptor* descriptor : descriptors)
	{
		UIDRef uid = descriptor->getUid ();
		if(descriptorFilter.numIds == 0 || descriptorFilter.contains (uid))
			iDescriptors->add (descriptor);
	}

	// create async operation notifying observers
	AutoPtr<AsyncOperation> operation = NEW AsyncOperation;

	Promise (return_shared<IAsyncOperation> (operation)).then ([&, iDescriptors] (IAsyncOperation& op)
	{
		observers.notify (&IGattCentralCharacteristicObserver::onGetDescriptorsCompleted, iDescriptors->getItems (), iDescriptors->count (), kError_NoError);
		delete iDescriptors;
	});

	operation->setStateDeferred (IAsyncInfo::kCompleted);

	return kError_NoError;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ErrorCode AndroidGattCentralCharacteristic::readAsync ()
{
	ASSERT (!pendingOperation)
	if(pendingOperation)
		return kError_InvalidState;

	pendingOperation = gattCentralDevice->readCharacteristic (this);

	Promise (return_shared<IAsyncOperation> (pendingOperation)).then ([&] (IAsyncOperation& op)
	{
		pendingOperation.release ();

		if(op.getState () == IAsyncInfo::kCompleted)
		{
			IBuffer* buffer = static_cast<IBuffer*> (op.getResult ().asUnknown ());
			observers.notify (&IGattCentralCharacteristicObserver::onReadCompleted,
							  static_cast<uint8*> (buffer->getBufferAddress ()),
							  buffer->getBufferSize (), kError_NoError);
		}
		else
			observers.notify (&IGattCentralCharacteristicObserver::onReadCompleted, nullptr, 0, kError_Failed);
	});

	return kError_NoError;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ErrorCode AndroidGattCentralCharacteristic::writeAsync (const uint8 valueBuffer[], int valueSize)
{
	ASSERT (!pendingOperation)
	if(pendingOperation)
		return kError_InvalidState;

	pendingOperation = gattCentralDevice->writeCharacteristic (this, valueBuffer, valueSize);

	Promise (return_shared<IAsyncOperation> (pendingOperation)).then ([&] (IAsyncOperation& op)
	{
		pendingOperation.release ();

		if(op.getState () == IAsyncInfo::kCompleted)
			observers.notify (&IGattCentralCharacteristicObserver::onWriteCompleted, op.getResult ());
		else
			observers.notify (&IGattCentralCharacteristicObserver::onWriteCompleted, op.getResult () ? op.getResult () : kError_Failed);
	});

	return kError_NoError;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ErrorCode AndroidGattCentralCharacteristic::subscribeAsync ()
{
	ASSERT (!pendingOperation)
	if(pendingOperation)
		return kError_InvalidState;

	pendingOperation = gattCentralDevice->setCharacteristicNotification (this, true);

	Promise (return_shared<IAsyncOperation> (pendingOperation)).then ([&] (IAsyncOperation& op)
	{
		pendingOperation.release ();

		if(op.getState () == IAsyncInfo::kCompleted)
			observers.notify (&IGattCentralCharacteristicObserver::onSubscribeCompleted, op.getResult ());
		else
			observers.notify (&IGattCentralCharacteristicObserver::onSubscribeCompleted, op.getResult () ? op.getResult () : kError_Failed);
	});

	return kError_NoError;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ErrorCode AndroidGattCentralCharacteristic::unsubscribeAsync ()
{
	pendingOperation = gattCentralDevice->setCharacteristicNotification (this, false);

	Promise (return_shared<IAsyncOperation> (pendingOperation)).then ([&] (IAsyncOperation& op)
	{
		pendingOperation.release ();

		if(op.getState () == IAsyncInfo::kCompleted)
			observers.notify (&IGattCentralCharacteristicObserver::onUnsubscribeCompleted, op.getResult ());
		else
			observers.notify (&IGattCentralCharacteristicObserver::onUnsubscribeCompleted, op.getResult () ? op.getResult () : kError_Failed);
	});

	return kError_NoError;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API AndroidGattCentralCharacteristic::notify (ISubject* subject, MessageRef msg)
{
	if(msg == "characteristicChanged")
	{
		ASSERT (msg.getArgCount () == 1)
		if(IBuffer* buffer = static_cast<IBuffer*> (msg[0].asUnknown ()))
			observers.notify (&IGattCentralCharacteristicObserver::onNotificationReceived, static_cast<uint8*> (buffer->getBufferAddress ()), buffer->getBufferSize ());
	}

	SuperClass::notify (subject, msg);
}

//************************************************************************************************
// AndroidGattCentralService
//************************************************************************************************

DEFINE_CLASS_HIDDEN (AndroidGattCentralService, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

AndroidGattCentralService::AndroidGattCentralService (AndroidGattCentralDevice* device, jobject service)
: gattCentralDevice (device),
  bluetoothGattService (JniAccessor (), service)
{
	JniAccessor jni;

	// get service info
	LocalRef jUuid (jni, BluetoothGattService.getUuid (bluetoothGattService));

	serviceId = uidFromJavaUuid (jUuid);

	// get characteristics
	LocalRef jCharacteristics (jni, BluetoothGattService.getCharacteristics (bluetoothGattService));
	int numCharacteristics = List.size (jCharacteristics);
	for(int i = 0; i < numCharacteristics; i++)
	{
		LocalRef jCharacteristic (jni, List.get (jCharacteristics, i));

		characteristics.add (NEW AndroidGattCentralCharacteristic (gattCentralDevice, jCharacteristic));
	}

	// get included services
	LocalRef jServices (jni, BluetoothGattService.getIncludedServices (bluetoothGattService));
	int numServices = List.size (jServices);
	for(int i = 0; i < numServices; i++)
	{
		LocalRef jService (jni, List.get (jServices, i));

		includedServices.add (NEW AndroidGattCentralService (gattCentralDevice, jService));
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const UIDBytes& AndroidGattCentralService::getServiceId () const
{
	return serviceId;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int AndroidGattCentralService::getNumIncludedServices () const
{
	return includedServices.count ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IGattCentralService* AndroidGattCentralService::getIncludedService (int index) const
{
	if(includedServices.isValidIndex (index))
		return includedServices[index];
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ErrorCode AndroidGattCentralService::getCharacteristicsAsync (const IDFilter& characteristicFilter)
{
	// create array of filtered characteristics
	Vector<IGattCentralCharacteristic*>* iCharacteristics = NEW Vector<IGattCentralCharacteristic*>;
	for(AndroidGattCentralCharacteristic* characteristic : characteristics)
	{
		UIDRef uid = characteristic->getUid ();
		if(characteristicFilter.numIds == 0 || characteristicFilter.contains (uid))
			iCharacteristics->add (characteristic);
	}

	// create async operation notifying observers
	AutoPtr<AsyncOperation> operation = NEW AsyncOperation;

	Promise (return_shared<IAsyncOperation> (operation)).then ([&, iCharacteristics] (IAsyncOperation& op)
	{
		observers.notify (&IGattCentralServiceObserver::onGetCharacteristicsCompleted, iCharacteristics->getItems (), iCharacteristics->count (), kError_NoError);
		delete iCharacteristics;
	});

	operation->setStateDeferred (IAsyncInfo::kCompleted);

	return kError_NoError;
}

//************************************************************************************************
// AndroidGattCentralDevice
//************************************************************************************************

DEFINE_CLASS_HIDDEN (AndroidGattCentralDevice, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

AndroidGattCentralDevice::AndroidGattCentralDevice (AndroidGattCentral* central, const DeviceDetails& deviceDetails)
: central (central),
  deviceDetails (deviceDetails)
{
	JniAccessor jni;
	LocalRef jDevice (jni, central->getDevice (deviceDetails.getIdentifier ()));
	if(jDevice != nullptr)
	{
		// create Java GattCentralDevice object
		gattCentralDevice.assign (jni, jni.newObject (GattCentralDevice, GattCentralDevice.construct, central->getJavaObject (), jDevice.getJObject (), JniIntPtr (this)));
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

AndroidGattCentralDevice::~AndroidGattCentralDevice ()
{
	ASSERT (!pendingConnect && !pendingDisconnect && !pendingOperation)
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool AndroidGattCentralDevice::isValid () const
{
	return gattCentralDevice.isValid ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ErrorCode AndroidGattCentralDevice::connectAsync ()
{
	ASSERT (!isConnected ())
	ASSERT (!pendingConnect)
	if(pendingConnect)
		return kError_InvalidState;

	pendingConnect = NEW AsyncOperation;

	if(!GattCentralDevice.connect (gattCentralDevice))
	{
		pendingConnect.release ();
		return kError_Failed;
	}

	Promise (return_shared<IAsyncOperation> (pendingConnect)).then ([&] (IAsyncOperation& op)
	{
		pendingConnect.release ();
		central->onDeviceConnected (op.getResult ().asInt ());
	});

	return kError_NoError;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ErrorCode AndroidGattCentralDevice::disconnectAsync ()
{
	ASSERT (isConnected ())
	ASSERT (!pendingDisconnect)
	if(pendingDisconnect)
		return kError_InvalidState;

	pendingDisconnect = NEW AsyncOperation;

	JniAccessor jni;
	if(!GattCentralDevice.disconnect (gattCentralDevice))
	{
		pendingDisconnect.release ();
		return kError_Failed;
	}

	Promise (return_shared<IAsyncOperation> (pendingDisconnect)).then ([&] (IAsyncOperation& op)
	{
		pendingDisconnect.release ();
		central->onDeviceDisconnected (op.getResult ().asInt ());
	});

	return kError_NoError;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Core::ErrorCode AndroidGattCentralDevice::setConnectionMode (ConnectionMode connectionMode)
{
	int connectionPriority;
	switch(connectionMode)
	{
	default :
	case kBalanced : connectionPriority = CONNECTION_PRIORITY_BALANCED; break;
	case kPowerSaving : connectionPriority = CONNECTION_PRIORITY_LOW_POWER; break;
	case kThroughput : connectionPriority = CONNECTION_PRIORITY_HIGH; break;
	}

	if(!GattCentralDevice.requestConnectionPriority (gattCentralDevice, connectionPriority))
		return kError_Failed;

	return kError_NoError;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool AndroidGattCentralDevice::isConnected () const
{
	return connectionState == STATE_CONNECTED;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ErrorCode AndroidGattCentralDevice::getServicesAsync ()
{
	ASSERT (!pendingOperation)
	if(pendingOperation)
		return kError_InvalidState;

	pendingOperation = NEW AsyncOperation;

	if(!GattCentralDevice.discoverServices (gattCentralDevice))
	{
		pendingOperation.release ();
		return kError_Failed;
	}

	Promise (return_shared<IAsyncOperation> (pendingOperation)).then ([&] (IAsyncOperation& op)
	{
		pendingOperation.release ();

		if(op.getState () == IAsyncInfo::kCompleted)
		{
			Vector<IGattCentralService*> iServices (services.count ());
			for(AndroidGattCentralService* service : services)
				iServices.add (service);

			observers.notify (&IGattCentralDeviceObserver::onGetServicesCompleted, iServices.getItems (), iServices.count (), kError_NoError);
		}
		else
		{
			observers.notify (&IGattCentralDeviceObserver::onGetServicesCompleted, nullptr, 0, op.getResult ());
		}
	});

	return kError_NoError;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AndroidGattCentralDevice::close ()
{
	services.removeAll ();

	GattCentralDevice.close (gattCentralDevice);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CStringPtr AndroidGattCentralDevice::getIdentifier () const
{
	return deviceDetails.getIdentifier ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CStringPtr AndroidGattCentralDevice::getName () const
{
	return deviceDetails.getName ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CStringPtr AndroidGattCentralDevice::getManufacturerData () const
{
	return deviceDetails.getManufacturerData ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IAsyncOperation* AndroidGattCentralDevice::readDescriptor (AndroidGattCentralDescriptor* descriptor)
{
	ASSERT (!pendingOperation)
	if(pendingOperation)
		return AsyncOperation::createFailed (true);

	pendingOperation = NEW AsyncOperation;

	if(!GattCentralDevice.readDescriptor (gattCentralDevice, descriptor->getJavaObject ()))
		pendingOperation->setStateDeferred (IAsyncInfo::kFailed);

	Promise promise = Promise (return_shared<IAsyncOperation> (pendingOperation)).then ([&] (IAsyncOperation& op)
	{
		pendingOperation.release ();
	});

	return return_shared<IAsyncOperation> (promise);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IAsyncOperation* AndroidGattCentralDevice::writeDescriptor (AndroidGattCentralDescriptor* descriptor, const uint8 valueBuffer[], int valueSize)
{
	ASSERT (!pendingOperation)
	if(pendingOperation)
		return AsyncOperation::createFailed (true);

	pendingOperation = NEW AsyncOperation;

	JniAccessor jni;
	JniByteArray value (jni, (jbyte*) valueBuffer, valueSize);
	if(!GattCentralDevice.writeDescriptor (gattCentralDevice, descriptor->getJavaObject (), value))
		pendingOperation->setStateDeferred (IAsyncInfo::kFailed);

	Promise promise = Promise (return_shared<IAsyncOperation> (pendingOperation)).then ([&] (IAsyncOperation& op)
	{
		pendingOperation.release ();
	});

	return return_shared<IAsyncOperation> (promise);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IAsyncOperation* AndroidGattCentralDevice::readCharacteristic (AndroidGattCentralCharacteristic* characteristic)
{
	ASSERT (!pendingOperation)
	if(pendingOperation)
		return AsyncOperation::createFailed (true);

	pendingOperation = NEW AsyncOperation;

	if(!GattCentralDevice.readCharacteristic (gattCentralDevice, characteristic->getJavaObject ()))
		pendingOperation->setStateDeferred (IAsyncInfo::kFailed);

	Promise promise = Promise (return_shared<IAsyncOperation> (pendingOperation)).then ([&] (IAsyncOperation& op)
	{
		pendingOperation.release ();
	});

	return return_shared<IAsyncOperation> (promise);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IAsyncOperation* AndroidGattCentralDevice::writeCharacteristic (AndroidGattCentralCharacteristic* characteristic, const uint8 valueBuffer[], int valueSize)
{
	ASSERT (!pendingOperation)
	if(pendingOperation)
		return AsyncOperation::createFailed (true);

	pendingOperation = NEW AsyncOperation;

	JniAccessor jni;
	JniByteArray value (jni, (jbyte*) valueBuffer, valueSize);
	if(!GattCentralDevice.writeCharacteristic (gattCentralDevice, characteristic->getJavaObject (), value))
		pendingOperation->setStateDeferred (IAsyncInfo::kFailed);

	Promise promise = Promise (return_shared<IAsyncOperation> (pendingOperation)).then ([&] (IAsyncOperation& op)
	{
		pendingOperation.release ();
	});

	return return_shared<IAsyncOperation> (promise);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IAsyncOperation* AndroidGattCentralDevice::setCharacteristicNotification (AndroidGattCentralCharacteristic* characteristic, bool enable)
{
	ASSERT (!pendingOperation)
	if(pendingOperation)
		return AsyncOperation::createFailed (true);

	if(enable == subscribedCharacteristics.contains (characteristic))
		return AsyncOperation::createCompleted (kError_NoError, true);

	pendingOperation = NEW AsyncOperation;

	if(!GattCentralDevice.setCharacteristicNotification (gattCentralDevice, characteristic->getJavaObject (), enable))
		pendingOperation->setStateDeferred (IAsyncInfo::kFailed);

	Promise promise = Promise (return_shared<IAsyncOperation> (pendingOperation)).then ([&, characteristic, enable] (IAsyncOperation& op)
	{
		pendingOperation.release ();

		if(enable)
			subscribedCharacteristics.add (characteristic);
		else
			subscribedCharacteristics.remove (characteristic);
	});

	return return_shared<IAsyncOperation> (promise);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AndroidGattCentralDevice::onConnectionStateChange (int status, int state)
{
	connectionState = state;
	ErrorCode errorCode = toErrorCode (status);

	if(pendingConnect)
	{
		pendingConnect->setResult (errorCode);
		if(errorCode == kError_NoError && state == STATE_CONNECTED)
			pendingConnect->setStateDeferred (IAsyncInfo::kCompleted);
		else if(errorCode != kError_NoError)
			pendingConnect->setStateDeferred (IAsyncInfo::kFailed);
	}
	else if(pendingDisconnect)
	{
		pendingDisconnect->setResult (errorCode);
		if(errorCode == kError_NoError && state == STATE_DISCONNECTED)
			pendingDisconnect->setStateDeferred (IAsyncInfo::kCompleted);
		else if(errorCode != kError_NoError)
			pendingDisconnect->setStateDeferred (IAsyncInfo::kFailed);
	}
	else if(state == STATE_DISCONNECTED)
	{
		if(pendingOperation)
			pendingOperation->setState (IAsyncInfo::kFailed);
		central->close (this);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AndroidGattCentralDevice::onServicesDiscovered (int status, const JniObjectArray& jServices)
{
	ErrorCode errorCode = toErrorCode (status);

	if(errorCode == kError_NoError)
	{
		services.removeAll ();

		if(jServices)
		{
			JniAccessor jni;
			int length = jServices.getLength ();
			for(int i = 0; i < length; i++)
			{
				LocalRef jService (jni, jServices[i]);
				LocalRef jUuid (jni, BluetoothGattService.getUuid (jService));

				UIDBytes uid = uidFromJavaUuid (jUuid);
				services.add (NEW AndroidGattCentralService (this, jService));
			}
		}

		pendingOperation->setStateDeferred (IAsyncInfo::kCompleted);
	}
	else
	{
		pendingOperation->setResult (errorCode);
		pendingOperation->setStateDeferred (IAsyncInfo::kFailed);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AndroidGattCentralDevice::onAttributeRead (int status, const JniByteArray& value)
{
	if(toErrorCode (status) == kError_NoError)
	{
		AutoPtr<IBuffer> buffer = createBufferFromJavaArray (value);

		pendingOperation->setResult (Variant (buffer, true));
		pendingOperation->setStateDeferred (IAsyncInfo::kCompleted);
	}
	else
		pendingOperation->setStateDeferred (IAsyncInfo::kFailed);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AndroidGattCentralDevice::onAttributeWrite (int status)
{
	ErrorCode errorCode = toErrorCode (status);

	pendingOperation->setResult (errorCode);
	pendingOperation->setStateDeferred (errorCode == kError_NoError ? IAsyncInfo::kCompleted : IAsyncInfo::kFailed);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AndroidGattCentralDevice::onSubscribeCompleted (int status)
{
	ErrorCode errorCode = toErrorCode (status);

	pendingOperation->setResult (errorCode);
	pendingOperation->setStateDeferred (errorCode == kError_NoError ? IAsyncInfo::kCompleted : IAsyncInfo::kFailed);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AndroidGattCentralDevice::onCharacteristicChanged (jobject jCharacteristic, const JniByteArray& value)
{
	JniAccessor jni;
	for(AndroidGattCentralCharacteristic* characteristic : subscribedCharacteristics)
	{
		if(!jni->IsSameObject (characteristic->getJavaObject (), jCharacteristic))
			continue;

		AutoPtr<IBuffer> buffer = createBufferFromJavaArray (value);
		(NEW Message ("characteristicChanged", Variant (buffer, true)))->post (characteristic);
		break;
	}
}

//************************************************************************************************
// GattCentralDevice Java native methods
//************************************************************************************************

DECLARE_JNI_CLASS_METHOD (dev_ccl_services_bluetooth, void, GattCentralDevice, onConnectionStateChangeNative, JniIntPtr nativePtr, int status, int state)
{
	AndroidGattCentralDevice* device = JniCast<AndroidGattCentralDevice>::fromIntPtr (nativePtr);
	if(!device)
		return;

	device->onConnectionStateChange (status, state);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DECLARE_JNI_CLASS_METHOD (dev_ccl_services_bluetooth, void, GattCentralDevice, onServicesDiscoveredNative, JniIntPtr nativePtr, int status, jobjectArray services)
{
	AndroidGattCentralDevice* device = JniCast<AndroidGattCentralDevice>::fromIntPtr (nativePtr);
	if(!device)
		return;

	device->onServicesDiscovered (status, JniObjectArray (env, services));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DECLARE_JNI_CLASS_METHOD (dev_ccl_services_bluetooth, void, GattCentralDevice, onAttributeReadNative, JniIntPtr nativePtr, int status, jbyteArray value)
{
	AndroidGattCentralDevice* device = JniCast<AndroidGattCentralDevice>::fromIntPtr (nativePtr);
	if(!device)
		return;

	device->onAttributeRead (status, JniByteArray (env, value));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DECLARE_JNI_CLASS_METHOD (dev_ccl_services_bluetooth, void, GattCentralDevice, onAttributeWriteNative, JniIntPtr nativePtr, int status)
{
	AndroidGattCentralDevice* device = JniCast<AndroidGattCentralDevice>::fromIntPtr (nativePtr);
	if(!device)
		return;

	device->onAttributeWrite (status);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DECLARE_JNI_CLASS_METHOD (dev_ccl_services_bluetooth, void, GattCentralDevice, onSubscribeCompletedNative, JniIntPtr nativePtr, int status)
{
	AndroidGattCentralDevice* device = JniCast<AndroidGattCentralDevice>::fromIntPtr (nativePtr);
	if(!device)
		return;

	device->onSubscribeCompleted (status);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DECLARE_JNI_CLASS_METHOD (dev_ccl_services_bluetooth, void, GattCentralDevice, onCharacteristicChangedNative, JniIntPtr nativePtr, jobject characteristic, jbyteArray value)
{
	AndroidGattCentralDevice* device = JniCast<AndroidGattCentralDevice>::fromIntPtr (nativePtr);
	if(!device)
		return;

	device->onCharacteristicChanged (characteristic, JniByteArray (env, value));
}

//************************************************************************************************
// DeviceDetails
//************************************************************************************************

DeviceDetails::DeviceDetails (jobject device, jobject record)
{
	JniAccessor jni;
	LocalStringRef jName (jni, BluetoothDevice.getName (device));
	LocalStringRef jAddress (jni, BluetoothDevice.getAddress (device));
	LocalRef jManufacturerData (jni, ScanRecord.getManufacturerSpecificData (record));

	if(jManufacturerData && SparseArray.size (jManufacturerData) > 0)
	{
		JniByteArray jValue (jni, (jbyteArray) SparseArray.valueAt (jManufacturerData, 0));
		char* value = NEW char[jValue.getLength () + 1];
		jValue.getData (value, jValue.getLength ());
		value[jValue.getLength ()] = 0;

		manufacturerData = MutableCString (value, Text::kUTF8);
		delete[] value;
	}

	name = MutableCString (fromJavaString (jName), Text::kUTF8);
	identifier = MutableCString (fromJavaString (jAddress));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DeviceDetails::update (jobject device)
{
	JniAccessor jni;
	LocalStringRef jName (jni, BluetoothDevice.getName (device));

	name = MutableCString (fromJavaString (jName), Text::kUTF8);
}

//************************************************************************************************
// AndroidGattCentral
//************************************************************************************************

DEFINE_CLASS_HIDDEN (AndroidGattCentral, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

AndroidGattCentral::AndroidGattCentral ()
: permissionsState (kPermissionsStateUnknown)
{
	// create Java GattCentral object
	JniAccessor jni;
	gattCentral.assign (jni, jni.newObject (GattCentral, GattCentral.construct, JniIntPtr (this)));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

AndroidGattCentral::~AndroidGattCentral ()
{
	ASSERT (!pendingConnect)
	ASSERT (!pendingDisconnect)

	cancelSignals ();
	cleanupDevices ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

jobject AndroidGattCentral::getJavaObject () const
{
	return gattCentral;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AndroidGattCentral::close (AndroidGattCentralDevice* device)
{
	if(device != nullptr)
	{
		connectedDevices.remove (device);
		int index = devices.index (device);
		if(index != -1)
		{
			observers.notify (&IGattCentralObserver::onDeviceRemoved, device);
			device->close ();
			devices.removeAt (index);
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API AndroidGattCentral::notify (ISubject* subject, MessageRef msg)
{
	if(msg == "stateChanged")
		observers.notify (&IGattCentralObserver::onStateChanged, getState ());
	else if(msg == "scanningStarted")
		observers.notify (&IGattCentralObserver::onScanningStarted);
	else if(msg == "scanningStopped")
		observers.notify (&IGattCentralObserver::onScanningStopped);
	else
		SuperClass::notify (subject, msg);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

GattCentralState AndroidGattCentral::getState () const
{
	int state = GattCentral.getState (gattCentral);

	if(state == STATE_NOT_SUPPORTED)
		return kNotSupported;

	if(permissionsState == kPermissionsStateDenied)
		return kPermissionDenied;

	if(permissionsState == kPermissionsStateGranted)
	{
		if(state == STATE_ON)
			return kPoweredOn;
		else if(state == STATE_OFF || state == STATE_TURNING_ON || state == STATE_TURNING_OFF)
			return kPoweredOff;
	}

	return kStateUnknown;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ErrorCode AndroidGattCentral::startScanning (const IDFilter& serviceFilter, const GattCentralScanOptions& scanOptions)
{
	// remove unconnected devices
	for(int i = devices.count () - 1; i >= 0; i--)
		if(!devices[i]->isConnected ())
			close (devices[i]);

	JniAccessor jni;
	JniStringArray jServiceIds (jni, serviceFilter.numIds);
	for(int i = 0; i < serviceFilter.numIds; i++)
	{
		char serviceId[39] = { 0 };
		serviceFilter.ids[i].toCString (serviceId, sizeof(serviceId));
		serviceId[37] = 0;
		jServiceIds.setElement (i, serviceId + 1);
	}

	// TODO: use scanOptions.kScanMode

	GattCentral.startScanning (gattCentral, jServiceIds, scanOptions.kAdvertisementTimeout);

	return kError_NoError;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ErrorCode AndroidGattCentral::stopScanning ()
{
	GattCentral.stopScanning (gattCentral);

	return kError_NoError;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ErrorCode AndroidGattCentral::connectAsync (IGattCentralDevice* iDevice, tbool autoReconnect)
{
	ASSERT (iDevice != nullptr)
	if(iDevice == nullptr)
		return kError_InvalidArgument;

	ASSERT (!pendingConnect)
	if(pendingConnect)
		return kError_InvalidState;

	for(AndroidGattCentralDevice* device : devices)
	{
		if(iDevice != device)
			continue;

		if(!device->isValid ())
			return kError_Failed;

		pendingConnect = NEW AsyncOperation;
		if(device->connectAsync () != kError_NoError)
		{
			pendingConnect.release ();
			return kError_Failed;
		}

		Promise (return_shared<IAsyncOperation> (pendingConnect)).then ([=] (IAsyncOperation& op)
		{
			pendingConnect.release ();
			if(op.getState () == IAsyncInfo::kCompleted)
			{
				connectedDevices.add (return_shared<AndroidGattCentralDevice> (device));
				observers.notify (&IGattCentralObserver::onConnectCompleted, device, kError_NoError);
			}
			else
			{
				observers.notify (&IGattCentralObserver::onConnectCompleted, device, op.getResult ().asInt ());
			}
		});
		break;
	}
	return kError_NoError;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ErrorCode AndroidGattCentral::disconnectAsync (IGattCentralDevice* iDevice)
{
	ASSERT (iDevice != nullptr)
	if(iDevice == nullptr)
		return kError_InvalidArgument;

	ASSERT (!pendingDisconnect)
	if(pendingDisconnect)
		return kError_InvalidState;

	for(AndroidGattCentralDevice* device : devices)
	{
		if(iDevice != device)
			continue;

		if(!device->isValid ())
			return kError_Failed;

		pendingDisconnect = NEW AsyncOperation;
		if(device->disconnectAsync () != kError_NoError)
		{
			pendingDisconnect.release ();
			return kError_Failed;
		}

		Promise (return_shared<IAsyncOperation> (pendingDisconnect)).then ([=] (IAsyncOperation& op)
		{
			pendingDisconnect.release ();
			if(op.getState () == IAsyncInfo::kCompleted)
			{
				connectedDevices.remove (return_shared<AndroidGattCentralDevice> (device));
				observers.notify (&IGattCentralObserver::onDisconnectCompleted, device, kError_NoError);
			}
			else
			{
				observers.notify (&IGattCentralObserver::onDisconnectCompleted, device, op.getResult ().asInt ());
			}
		});
		break;
	}
	return kError_NoError;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

jobject AndroidGattCentral::getDevice (CStringPtr identifier) const
{
	JniAccessor jni;
	JniString jDeviceAddress (jni, identifier);
	return GattCentral.getDevice(gattCentral, jDeviceAddress);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AndroidGattCentral::onDeviceFound (jobject jDevice, jobject record)
{
	auto* device = NEW AndroidGattCentralDevice (this, DeviceDetails (jDevice, record));
	device->retain ();
	devices.add (device);
	observers.notify (&IGattCentralObserver::onDeviceAdded, device);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AndroidGattCentral::onDeviceLost (jobject jDevice, jobject record)
{
	DeviceDetails details (jDevice, record);

	for(AndroidGattCentralDevice* device : devices)
	{
		if(CString (device->getIdentifier ()) == details.getIdentifier ())
		{
			onDeviceLost (device);
			return;
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AndroidGattCentral::onPermissionsUpdated (PermissionsState state)
{
	if(permissionsState != state)
	{
		permissionsState = state;
		(NEW Message ("stateChanged"))->post (this);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AndroidGattCentral::onScanningStarted ()
{
	(NEW Message ("scanningStarted"))->post (this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AndroidGattCentral::onScanningStopped ()
{
	(NEW Message ("scanningStopped"))->post (this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AndroidGattCentral::onDeviceConnected (ErrorCode result)
{
	ASSERT (pendingConnect)
	if(!pendingConnect)
		return;

	pendingConnect->setResult (result);
	pendingConnect->setStateDeferred ((result == kError_NoError) ? IAsyncInfo::kCompleted : IAsyncInfo::kFailed);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AndroidGattCentral::onDeviceDisconnected (ErrorCode result)
{
	ASSERT (pendingDisconnect)
	if(!pendingDisconnect)
		return;

	pendingDisconnect->setResult (result);
	pendingDisconnect->setStateDeferred ((result == kError_NoError) ? IAsyncInfo::kCompleted : IAsyncInfo::kFailed);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AndroidGattCentral::onDeviceLost (AndroidGattCentralDevice* device)
{
	observers.notify (&IGattCentralObserver::onDeviceRemoved, device);
	devices.remove (device);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AndroidGattCentral::cleanupDevices ()
{
	while(!devices.isEmpty ())
		close (devices.last ());
	ASSERT (connectedDevices.isEmpty ())
}

//************************************************************************************************
// GattCentral Java native methods
//************************************************************************************************

DECLARE_JNI_CLASS_METHOD (dev_ccl_services_bluetooth, void, GattCentral, onDeviceFoundNative, JniIntPtr nativePtr, jobject device, jobject record)
{
	AndroidGattCentral* gattCentral = JniCast<AndroidGattCentral>::fromIntPtr (nativePtr);
	if(!gattCentral)
		return;

	gattCentral->onDeviceFound (device, record);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DECLARE_JNI_CLASS_METHOD (dev_ccl_services_bluetooth, void, GattCentral, onDeviceLostNative, JniIntPtr nativePtr, jobject device, jobject record)
{
	AndroidGattCentral* gattCentral = JniCast<AndroidGattCentral>::fromIntPtr (nativePtr);
	if(!gattCentral)
		return;

	gattCentral->onDeviceLost (device, record);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DECLARE_JNI_CLASS_METHOD (dev_ccl_services_bluetooth, void, GattCentral, onPermissionsUpdatedNative, JniIntPtr nativePtr, int permissionsState)
{
	AndroidGattCentral* gattCentral = JniCast<AndroidGattCentral>::fromIntPtr (nativePtr);
	if(!gattCentral)
		return;

	gattCentral->onPermissionsUpdated (static_cast<PermissionsState> (permissionsState));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DECLARE_JNI_CLASS_METHOD (dev_ccl_services_bluetooth, void, GattCentral, onScanningStartedNative, JniIntPtr nativePtr)
{
	AndroidGattCentral* gattCentral = JniCast<AndroidGattCentral>::fromIntPtr (nativePtr);
	if(!gattCentral)
		return;

	gattCentral->onScanningStarted ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DECLARE_JNI_CLASS_METHOD (dev_ccl_services_bluetooth, void, GattCentral, onScanningStoppedNative, JniIntPtr nativePtr)
{
	AndroidGattCentral* gattCentral = JniCast<AndroidGattCentral>::fromIntPtr (nativePtr);
	if(!gattCentral)
		return;

	gattCentral->onScanningStopped ();
}
