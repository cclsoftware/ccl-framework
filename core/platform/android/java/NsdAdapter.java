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
// Filename    : core/platform/android/java/NsdAdapter.java
// Description : DNS Service Discovery Android implementation
//
//************************************************************************************************

package dev.ccl.core;

import android.content.Context;
import android.net.nsd.*;

import androidx.annotation.Keep;

import java.net.*;

//************************************************************************************************
// NsdDiscoveryHandler
//************************************************************************************************

class NsdDiscoveryHandler implements NsdManager.DiscoveryListener
{
	private final long nativeHandler;

	public NsdDiscoveryHandler (long nativeHandler)
	{
		this.nativeHandler = nativeHandler;
	}

	@Override
	public void onDiscoveryStarted (String serviceType)
	{}

	@Override
	public void onDiscoveryStopped (String serviceType)
	{}

	@Override
	public void onServiceFound (NsdServiceInfo serviceInfo)
	{
		onServiceFound (nativeHandler, serviceInfo);
	}

	@Override
	public void onServiceLost (NsdServiceInfo serviceInfo)
	{
		onServiceLost (nativeHandler, serviceInfo);
	}

	@Override
	public void onStartDiscoveryFailed (String serviceType, int errorCode)
	{}

	@Override
	public void onStopDiscoveryFailed (String serviceType, int errorCode)
	{}

	private native void onServiceFound (long nativeHandler, NsdServiceInfo serviceInfo);
	private native void onServiceLost (long nativeHandler, NsdServiceInfo serviceInfo);
}

//************************************************************************************************
// NsdResolveHandler
//************************************************************************************************

class NsdResolveHandler implements NsdManager.ResolveListener
{
	private final long nativeHandler;

	public NsdResolveHandler (long nativeHandler)
	{
		this.nativeHandler = nativeHandler;
	}

	@Override
	public void onResolveFailed (NsdServiceInfo serviceInfo, int errorCode)
	{}

	@Override
	public void onServiceResolved (NsdServiceInfo serviceInfo)
	{
		onServiceResolved (nativeHandler, serviceInfo);
	}

	private native void onServiceResolved (long nativeHandler, NsdServiceInfo serviceInfo);
}

//************************************************************************************************
// NsdRegistrationHandler
//************************************************************************************************

class NsdRegistrationHandler implements NsdManager.RegistrationListener
{
	private final long nativeHandler;

	public NsdRegistrationHandler (long nativeHandler)
	{
		this.nativeHandler = nativeHandler;
	}

	@Override
	public void onRegistrationFailed (NsdServiceInfo serviceInfo, int errorCode)
	{
		onRegistrationFailed (nativeHandler, serviceInfo, errorCode);
	}

	@Override
	public void onServiceRegistered (NsdServiceInfo serviceInfo)
	{
		onServiceRegistered (nativeHandler, serviceInfo);
	}

	@Override
	public void onServiceUnregistered (NsdServiceInfo serviceInfo)
	{}

	@Override
	public void onUnregistrationFailed (NsdServiceInfo serviceInfo, int errorCode)
	{}

	private native void onRegistrationFailed (long nativeHandler, NsdServiceInfo serviceInfo, int errorCode);
	private native void onServiceRegistered (long nativeHandler, NsdServiceInfo serviceInfo);
}

//************************************************************************************************
// NsdAdapter
//************************************************************************************************

@Keep
public class NsdAdapter
{
	private final long nativeHandler;

	private NsdManager nsdManager;

	public NsdAdapter (long nativeHandler)
	{
		this.nativeHandler = nativeHandler;

		Context context = CurrentContext.get ();
		if(context != null)
			nsdManager = (NsdManager) context.getSystemService (Context.NSD_SERVICE);
	}

	public NsdDiscoveryHandler discoverServices (String serviceType)
	{
		if(nsdManager == null)
			return null;

		NsdDiscoveryHandler handler = new NsdDiscoveryHandler (nativeHandler);

		try
		{
			nsdManager.discoverServices (serviceType, NsdManager.PROTOCOL_DNS_SD, handler);
		}
		catch(Exception e)
		{
			return null;
		}

		return handler;
	}

	public void stopDiscovery (NsdDiscoveryHandler handler)
	{
		if(nsdManager != null && handler != null)
			nsdManager.stopServiceDiscovery (handler);
	}

	public NsdResolveHandler resolveService (NsdServiceInfo serviceInfo)
	{
		if(nsdManager == null)
			return null;

		NsdResolveHandler handler = new NsdResolveHandler (nativeHandler);

		try
		{
			nsdManager.resolveService (serviceInfo, handler);
		}
		catch(Exception e)
		{
			return null;
		}

		return handler;
	}

	public NsdRegistrationHandler registerService (NsdServiceInfo serviceInfo)
	{
		if(nsdManager == null)
			return null;

		NsdRegistrationHandler handler = new NsdRegistrationHandler (nativeHandler);

		try
		{
			nsdManager.registerService (serviceInfo, NsdManager.PROTOCOL_DNS_SD, handler);
		}
		catch(Exception e)
		{
			return null;
		}

		return handler;
	}

	public void unregisterService (NsdRegistrationHandler handler)
	{
		if(nsdManager != null && handler != null)
			nsdManager.unregisterService (handler);
	}
}
