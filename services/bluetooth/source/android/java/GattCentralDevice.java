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
// Filename    : services/ccl/bluetooth/source/android/java/GattCentralDevice.java
// Description : Bluetooth LE Gatt Central Device
//
//************************************************************************************************

package dev.ccl.services.bluetooth;

import dev.ccl.cclgui.FrameworkActivity;

import android.bluetooth.*;
import android.bluetooth.le.*;

import androidx.annotation.Keep;

import java.util.List;
import java.util.UUID;

//************************************************************************************************
// GattCentralDevice
//************************************************************************************************

@Keep
class GattCentralDevice extends BluetoothGattCallback
{
	private final GattCentral central;

	private final BluetoothDevice device;
	private BluetoothGatt gatt;

	private final long nativeResultHandler;

	private BluetoothGattDescriptor subscribeConfigDescriptor;

	public GattCentralDevice (GattCentral central, BluetoothDevice device, long nativeResultHandler)
	{
		this.central = central;
		this.device = device;
		this.nativeResultHandler = nativeResultHandler;
	}
	
	private static FrameworkActivity getActivity ()
	{
		return FrameworkActivity.getActivity ();
	}

	public boolean connect ()
	{
		try
		{
			if(gatt == null)
				gatt = device.connectGatt (getActivity (), false, this, BluetoothDevice.TRANSPORT_LE);
			else
				gatt.connect ();
			central.onDeviceConnected (device);
		}
		catch(Exception e)
		{
			return false;
		}

		return true;
	}

	public boolean disconnect ()
	{
		try
		{
			gatt.disconnect ();
			central.onDeviceDisconnected (device);
		}
		catch(Exception e)
		{
			return false;
		}

		return true;
	}

	public boolean requestConnectionPriority (int connectionPriority)
	{
		if(gatt == null)
			return false;

		return gatt.requestConnectionPriority (connectionPriority);
	}

	public boolean discoverServices ()
	{
		if(gatt == null)
			return false;

		return gatt.discoverServices ();
	}

	public boolean readDescriptor (BluetoothGattDescriptor descriptor)
	{
		if(gatt == null)
			return false;

		return gatt.readDescriptor (descriptor);
	}

	public boolean writeDescriptor (BluetoothGattDescriptor descriptor, byte[] value)
	{
		if(gatt == null)
			return false;

		descriptor.setValue (value);

		return gatt.writeDescriptor (descriptor);
	}

	public boolean readCharacteristic (BluetoothGattCharacteristic characteristic)
	{
		if(gatt == null)
			return false;

		return gatt.readCharacteristic (characteristic);
	}

	public boolean writeCharacteristic (BluetoothGattCharacteristic characteristic, byte[] value)
	{
		if(gatt == null)
			return false;

		characteristic.setValue (value);

		return gatt.writeCharacteristic (characteristic);
	}

	public boolean setCharacteristicNotification (BluetoothGattCharacteristic characteristic, boolean enable)
	{
		if(gatt == null)
			return false;

		if(subscribeConfigDescriptor != null)
			return false;

		// configure local notifications
		if(!gatt.setCharacteristicNotification (characteristic, enable))
			return false;

		// configure remote notifications
		final UUID CONFIG_DESCRIPTOR = UUID.fromString ("00002902-0000-1000-8000-00805f9b34fb");

		subscribeConfigDescriptor = characteristic.getDescriptor (CONFIG_DESCRIPTOR);
		subscribeConfigDescriptor.setValue (enable ? BluetoothGattDescriptor.ENABLE_NOTIFICATION_VALUE : BluetoothGattDescriptor.DISABLE_NOTIFICATION_VALUE);

		return gatt.writeDescriptor (subscribeConfigDescriptor);
	}

	public void close ()
	{
		if(gatt == null)
			return;

		gatt.close ();
		gatt = null;
		central.onDeviceDisconnected (device);
	}

	@Override
	public void onConnectionStateChange (BluetoothGatt gatt, int status, int newState)
	{
		onConnectionStateChangeNative (nativeResultHandler, status, newState);
	}

	@Override
	public void onServicesDiscovered (BluetoothGatt gatt, int status)
	{
		List<BluetoothGattService> services = gatt.getServices ();

		onServicesDiscoveredNative (nativeResultHandler, status, services.toArray ());
	}
	
	@Override
	public void onDescriptorRead (BluetoothGatt gatt, BluetoothGattDescriptor descriptor, int status)
	{
		onAttributeReadNative (nativeResultHandler, status, descriptor.getValue ());
	}
	
	@Override
	public void onDescriptorWrite (BluetoothGatt gatt, BluetoothGattDescriptor descriptor, int status)
	{
		if(descriptor == subscribeConfigDescriptor)
		{
			onSubscribeCompletedNative (nativeResultHandler, status);

			subscribeConfigDescriptor = null;
		}
		else
			onAttributeWriteNative (nativeResultHandler, status);
	}

	@Override
	public void onCharacteristicRead (BluetoothGatt gatt, BluetoothGattCharacteristic characteristic, int status)
	{
		onAttributeReadNative (nativeResultHandler, status, characteristic.getValue ());
	}

	@Override
	public void onCharacteristicWrite (BluetoothGatt gatt, BluetoothGattCharacteristic characteristic, int status)
	{
		onAttributeWriteNative (nativeResultHandler, status);
	}

	@Override
	public void onCharacteristicChanged (BluetoothGatt gatt, BluetoothGattCharacteristic characteristic)
	{
		onCharacteristicChangedNative (nativeResultHandler, characteristic, characteristic.getValue ());
	}

	public native void onConnectionStateChangeNative (long nativeResultHandler, int status, int state);
	public native void onServicesDiscoveredNative (long nativeResultHandler, int status, Object[] services);
	
	public native void onAttributeReadNative (long nativeResultHandler, int status, byte[] value);
	public native void onAttributeWriteNative (long nativeResultHandler, int status);

	public native void onSubscribeCompletedNative (long nativeResultHandler, int status);
	
	public native void onCharacteristicChangedNative (long nativeResultHandler, Object characteristic, byte[] value);
}
