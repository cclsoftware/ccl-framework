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
// Filename    : services/ccl/bluetooth/source/android/java/GattCentral.java
// Description : Bluetooth LE Gatt Central
//
//************************************************************************************************

package dev.ccl.services.bluetooth;

import android.bluetooth.*;
import android.bluetooth.le.*;
import android.content.pm.PackageManager;
import android.content.Context;
import android.content.Intent;
import android.os.Build;
import android.os.Handler;
import android.os.Looper;
import android.os.ParcelUuid;
import android.Manifest;

import androidx.annotation.Keep;

import dev.ccl.cclgui.FrameworkActivity;

import java.util.ArrayList;

//************************************************************************************************
// BluetoothPermissionsManager
//************************************************************************************************

class BluetoothPermissionsManager
{	
	private static final int kRequestEnableBluetooth = 100;

	private class RequestPermissionsResultHandler extends FrameworkActivity.RequestPermissionsResultHandler
	{
		@Override
		public void onRequestPermissionsResult (String[] permissions, int[] grantResults)
		{
			boolean allGranted = true;
			for(int grantResult : grantResults)
				if(grantResult != PackageManager.PERMISSION_GRANTED)
					allGranted = false;

			if(permissions.length > 0 && allGranted)
			{
				onPermissionsUpdated (BluetoothConstants.kPermissionsStateGranted);

				// check if Bluetooth is enabled, request it otherwise
				android.util.Log.w ("CCL", "bluetoothAdapter.isEnabled: " + bluetoothAdapter.isEnabled ());
				if(bluetoothAdapter.isEnabled ())
				{
					startScanning ();
					centrals.clear ();
				}
				else
				{
					Intent enableBtIntent = new Intent (BluetoothAdapter.ACTION_REQUEST_ENABLE);
					FrameworkActivity.getActivity ().startActivityWithCallback (enableBtIntent, kRequestEnableBluetooth, new EnableBluetoothResultHandler ());
				}
			}
			else
			{
				onPermissionsUpdated (BluetoothConstants.kPermissionsStateDenied);
				centrals.clear ();
			}
		}
	}

	private class EnableBluetoothResultHandler extends FrameworkActivity.ActivityCallback
	{
		@Override
		public void onActivityResult (int requestCode, boolean success, Intent resultData)
		{
			android.util.Log.w ("CCL", "kRequestEnableBluetooth: " + success);

			// continue if user has enabled Bluetooth
			if(requestCode == kRequestEnableBluetooth)
			{
				if(success)
					startScanning ();
				centrals.clear ();
			}
		}
	}

	private final ArrayList<GattCentral> centrals = new ArrayList<> ();

	private final BluetoothAdapter bluetoothAdapter;

	public BluetoothPermissionsManager (BluetoothAdapter bluetoothAdapter)
	{
		this.bluetoothAdapter = bluetoothAdapter;
	}

	private static ArrayList<String> getPermissionsList ()
	{
		ArrayList<String> permissions = new ArrayList<> ();

		// Bluetooth scanning requires location permission
		if(Build.VERSION.SDK_INT <= 28)
			permissions.add (Manifest.permission.ACCESS_COARSE_LOCATION);
		else if(Build.VERSION.SDK_INT <= 30)
			permissions.add (Manifest.permission.ACCESS_FINE_LOCATION);

		// Bluetooth scanning permission changed in API level 31
		if(Build.VERSION.SDK_INT <= 30)
		{
			permissions.add (Manifest.permission.BLUETOOTH);
			permissions.add (Manifest.permission.BLUETOOTH_ADMIN);
		}
		else
		{
			permissions.add ("android.permission.BLUETOOTH_SCAN");
			permissions.add ("android.permission.BLUETOOTH_CONNECT");
		}

		return permissions;
	}

	public boolean requestPermissionsAndStartScanning (GattCentral central)
	{
		if(bluetoothAdapter == null)
			return false;

		centrals.add (central);

		// check if a bringup request is already running
		if(centrals.size () > 1)
			return true;

		// acquire permissions
		onPermissionsUpdated (BluetoothConstants.kPermissionsStateRequested);

		ArrayList<String> permissions = getPermissionsList ();
		FrameworkActivity.getActivity ().checkAndRequestPermissions (permissions.toArray (new String[0]), new RequestPermissionsResultHandler ());
		return true;
	}

	public void cancelPermissionsRequest (GattCentral central)
	{
		central.onPermissionsUpdated (BluetoothConstants.kPermissionsStateUnknown);
		centrals.remove (central);
	}

	private void onPermissionsUpdated (int permissions)
	{
		for(GattCentral central : centrals)
			central.onPermissionsUpdated (permissions);
	}

	private void startScanning ()
	{
		for(GattCentral central : centrals)
			central.startScanning ();
	}
}

//************************************************************************************************
// GattCentral
//************************************************************************************************

@Keep
class GattCentral
{
	static BluetoothPermissionsManager permissionsManager;

	private static final long kAdvertisementTimeoutCheckPeriod = 500; // check for lost devices every x milliseconds
	private static final long kDefaultAdvertisementTimeout = 5000; // clear devices not seen for x milliseconds

	BluetoothAdapter bluetoothAdapter;
	private final Handler handler = new Handler (Looper.getMainLooper ());
	private final long nativeResultHandler;
	private boolean isScanning = false;
	private String[] serviceIds;
	private long advertisementTimeout = kDefaultAdvertisementTimeout;

	// keeps track of devices seen during scan
	protected static class BluetoothDeviceInfo
	{
		public BluetoothDevice device;
		public ScanRecord record;
		public long lastSeen;
		public boolean connected;

		public BluetoothDeviceInfo (BluetoothDevice device, ScanRecord record)
		{
			this.device = device;
			this.record = record;
			this.lastSeen = System.currentTimeMillis ();
			this.connected = false;
		}
	}

	ArrayList<BluetoothDeviceInfo> devices = new ArrayList<> ();

	public GattCentral (long nativeResultHandler)
	{
		this.nativeResultHandler = nativeResultHandler;

		// check if BLE is supported on the device at all
		FrameworkActivity activity = FrameworkActivity.getActivity ();
		if(activity.getPackageManager ().hasSystemFeature (PackageManager.FEATURE_BLUETOOTH_LE))
		{
			// get Bluetooth adapter via Bluetooth manager
			BluetoothManager bluetoothManager = (BluetoothManager) activity.getSystemService (Context.BLUETOOTH_SERVICE);
			if(bluetoothManager != null)
				bluetoothAdapter = bluetoothManager.getAdapter ();

			if(bluetoothAdapter != null)
				if(permissionsManager == null)
					permissionsManager = new BluetoothPermissionsManager (bluetoothAdapter);
		}
	}

	public int getState ()
	{
		if(bluetoothAdapter == null)
			return 0;

		return bluetoothAdapter.getState ();
	}

	public void startScanning (String[] serviceIds, int advertisementTimeout)
	{
		if(bluetoothAdapter == null)
			return;

		this.serviceIds = serviceIds;
		this.advertisementTimeout = advertisementTimeout;

		permissionsManager.requestPermissionsAndStartScanning (this);
	}

	protected void startScanning ()
	{
		if(bluetoothAdapter == null)
			return;

		BluetoothLeScanner bluetoothLeScanner = bluetoothAdapter.getBluetoothLeScanner ();
		if(bluetoothLeScanner == null)
			return;
			
		isScanning = true;
		devices.clear ();

		ArrayList<ScanFilter> scanFilters = new ArrayList<> ();
		for(String serviceId : serviceIds)
			scanFilters.add (new ScanFilter.Builder ().setServiceUuid (ParcelUuid.fromString (serviceId)).build ());
		ScanSettings scanSettings = new ScanSettings.Builder ().setScanMode (ScanSettings.SCAN_MODE_LOW_LATENCY)
															   .setCallbackType (ScanSettings.CALLBACK_TYPE_ALL_MATCHES)
															   .build ();
		bluetoothLeScanner.startScan (scanFilters, scanSettings, scanCallback);

		onScanningStarted ();

		// scan for lost devices periodically
		handler.postDelayed (deviceTimeoutCheck, kAdvertisementTimeoutCheckPeriod);
	}
	
	public void stopScanning ()
	{
		if(bluetoothAdapter == null)
			return;

		BluetoothLeScanner bluetoothLeScanner = bluetoothAdapter.getBluetoothLeScanner ();
		if(bluetoothLeScanner == null)
			return;

		if(isScanning)
		{
			bluetoothLeScanner.stopScan (scanCallback);
			isScanning = false;
			onScanningStopped ();
		}
		else
			permissionsManager.cancelPermissionsRequest (this);
	}

	public BluetoothDevice getDevice (String address)
	{
		if(bluetoothAdapter == null)
			return null;

		try
		{
			return bluetoothAdapter.getRemoteDevice (address);
		}
		catch(Exception e)
		{}

		return null;
	}

	public void onPermissionsUpdated (int permissionsState)
	{
		onPermissionsUpdatedNative (nativeResultHandler, permissionsState);
	}

	public void onScanningStarted ()
	{
		onScanningStartedNative (nativeResultHandler);
	}

	public void onScanningStopped ()
	{
		onScanningStoppedNative (nativeResultHandler);
	}

	public void onDeviceConnected (BluetoothDevice device)
	{
		for(BluetoothDeviceInfo info : devices)
			if(info.device.getAddress ().equals (device.getAddress ()))
				info.connected = true;
	}

	public void onDeviceDisconnected (BluetoothDevice device)
	{
		for(BluetoothDeviceInfo info : devices)
			if(info.device.getAddress ().equals (device.getAddress ()))
				info.connected = false;
	}

	// callback invoked when a device is seen during scan
	private final ScanCallback scanCallback = new ScanCallback ()
	{
		@Override
		public void onScanResult (int callbackType, ScanResult result)
		{
			BluetoothDevice device = result.getDevice ();
			ScanRecord record = result.getScanRecord ();

			boolean known = false;
			for(BluetoothDeviceInfo info : devices)
			{
				if(info.device.getAddress ().equals (device.getAddress ()))
				{
					known = true;
					info.lastSeen = System.currentTimeMillis ();
					break;
				}
			}

			if(!known)
			{
				android.util.Log.w ("CCL", "onScanResult: " + device.getName ());
				devices.add (new BluetoothDeviceInfo (device, record));
				onDeviceFoundNative (nativeResultHandler, device, record);
			}
		}
	};

	// periodic check for devices no longer available
	private final Runnable deviceTimeoutCheck = new Runnable ()
	{
		@Override
		public void run ()
		{
			if(!isScanning)
				return;

			long currentTime = System.currentTimeMillis ();
			for(int i = devices.size () - 1; i >= 0; i--)
			{
				BluetoothDeviceInfo record = devices.get (i);
				if(record.connected || record.lastSeen > currentTime - advertisementTimeout)
					continue;

				devices.remove (record);
				onDeviceLostNative (nativeResultHandler, record.device, record.record);
			}

			handler.postDelayed (this, kAdvertisementTimeoutCheckPeriod);
		}
	};

	public native void onDeviceFoundNative (long nativeResultHandler, BluetoothDevice device, ScanRecord record);
	public native void onDeviceLostNative (long nativeResultHandler, BluetoothDevice device, ScanRecord record);
	public native void onPermissionsUpdatedNative (long nativeResultHandler, int permissionsState);
	public native void onScanningStartedNative (long nativeResultHandler);
	public native void onScanningStoppedNative (long nativeResultHandler);
}
