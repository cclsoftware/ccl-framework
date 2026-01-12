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
// Filename    : ccl/platform/win/system/management.cpp
// Description : Windows Management Instrumentation (WMI) Helper Classes
//
//************************************************************************************************

// Example: Getting WMI Data from the Local Computer
// http://msdn.microsoft.com/en-us/library/aa390423(VS.85).aspx

#include "ccl/base/boxedtypes.h"
#include "ccl/base/collections/stringlist.h"

#include "ccl/public/cclversion.h"

#include "ccl/platform/win/system/management.h"
#include "ccl/platform/win/system/cclcom.h"
#include "ccl/platform/win/system/registry.h"

#include <Wbemidl.h>

#pragma comment (lib, "Wbemuuid.lib")

using namespace CCL;
using namespace Win32;

#define CCL_SYSTEMINFORMATION_KEY \
	"Software\\" CCL_SETTINGS_NAME "\\SystemInformation"

typedef NativeString<BSTR> bstr_t;

//************************************************************************************************
// ManagementServices
//************************************************************************************************

const String ManagementServices::kRootNamespace ("ROOT\\CIMV2");

////////////////////////////////////////////////////////////////////////////////////////////////////

ManagementServices::ManagementServices (StringRef resourceName)
: locator (nullptr),
  services (nullptr)
{
	construct (resourceName);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

ManagementServices::~ManagementServices ()
{
	if(services)
		services->Release ();
	if(locator)
		locator->Release ();
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void ManagementServices::construct (StringRef resourceName)
{
	// Obtain the initial locator to WMI
	if(SUCCEEDED (::CoCreateInstance (CLSID_WbemLocator, nullptr, CLSCTX_INPROC_SERVER, IID_IWbemLocator, (LPVOID*)&locator)))
	{
		// Connect to WMI with the current user
		if(SUCCEEDED (locator->ConnectServer (
			 bstr_t (resourceName),   // Object path of WMI namespace
			 NULL,                    // User name. NULL = current user
			 NULL,                    // User password. NULL = current
			 NULL,                    // Locale. NULL indicates current
			 NULL,                    // Security flags.
			 NULL,                    // Authority (e.g. Kerberos)
			 nullptr,                 // Context object 
			 &services                // pointer to IWbemServices proxy
			 )))
		{
			// Set security levels on the proxy
			HRESULT result = ::CoSetProxyBlanket (
				   services,                    // Indicates the proxy to set
				   RPC_C_AUTHN_WINNT,           // RPC_C_AUTHN_xxx
				   RPC_C_AUTHZ_NONE,            // RPC_C_AUTHZ_xxx
				   nullptr,                     // Server principal name 
				   RPC_C_AUTHN_LEVEL_CALL,      // RPC_C_AUTHN_LEVEL_xxx 
				   RPC_C_IMP_LEVEL_IMPERSONATE, // RPC_C_IMP_LEVEL_xxx
				   NULL,                        // client identity
				   EOAC_NONE                    // proxy capabilities 
				);

			ASSERT (SUCCEEDED (result))
		}
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////

bool ManagementServices::isValid () const
{
	return services != nullptr;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

ManagementEnumerator* ManagementServices::execQuery (StringRef query)
{
	ASSERT (isValid ())
	if(!isValid ())
		return nullptr;

	IEnumWbemClassObject* enumerator = nullptr;

	if(SUCCEEDED (services->ExecQuery (
			bstr_t ("WQL"), 
			bstr_t (query),
			WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
			nullptr,
			&enumerator)))
				return NEW ManagementEnumerator (enumerator);

	return nullptr;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

#if DEBUG
void ManagementServices::dumpAll ()
{
	static const char* queryList[] =
	{
		"SELECT * FROM Win32_Processor",
		"SELECT * FROM Win32_DiskDrive",
		"SELECT * FROM Win32_PhysicalMemory",
		"SELECT * FROM Win32_BaseBoard",
		"SELECT * FROM Win32_BIOS",
		"SELECT * from Win32_NetworkAdapter WHERE AdapterType=\"Ethernet 802.3\""
	};

	AutoPtr<ManagementObject> object;
	AutoPtr<ManagementEnumerator> enumerator;

	for(int iQuery = 0; iQuery < ARRAY_COUNT (queryList); iQuery++)
	{
		Debugger::printf ("### %s ###\n", queryList[iQuery]);
		int iObject = 0;
		if(enumerator = execQuery (queryList[iQuery]))
			while(object = enumerator->next ())
			{
				Debugger::printf ("--- (%d)\n", ++iObject);
				object->dumpAll ();
			}
	}
}
#endif

//************************************************************************************************
// ManagementEnumerator
//************************************************************************************************

ManagementEnumerator::ManagementEnumerator (IEnumWbemClassObject* enumerator)
: enumerator (enumerator)
{
	ASSERT (enumerator != nullptr)
}

////////////////////////////////////////////////////////////////////////////////////////////////////

ManagementEnumerator::~ManagementEnumerator ()
{
	enumerator->Release ();
}

////////////////////////////////////////////////////////////////////////////////////////////////////

ManagementObject* ManagementEnumerator::next ()
{
	IWbemClassObject* object = nullptr;
	ULONG returned = 0;

	if(SUCCEEDED (enumerator->Next (WBEM_INFINITE, 1, &object, &returned)) && object != nullptr)
		return NEW ManagementObject (object);

	return nullptr;
}

//************************************************************************************************
// ManagementObject
//************************************************************************************************

ManagementObject::ManagementObject (IWbemClassObject* object)
: object (object)
{
	ASSERT (object != nullptr)
}

////////////////////////////////////////////////////////////////////////////////////////////////////

ManagementObject::~ManagementObject ()
{
	object->Release ();
}

////////////////////////////////////////////////////////////////////////////////////////////////////

bool ManagementObject::getPropertyNames (StringList& nameList)
{
	SAFEARRAY* names = nullptr;
	HRESULT result = object->GetNames (nullptr, WBEM_FLAG_ALWAYS, nullptr, &names);
	if(SUCCEEDED (result))
	{
		LONG iStart = 0;
		LONG iEnd = 0;
		::SafeArrayGetLBound (names, 1, &iStart);
		::SafeArrayGetUBound (names, 1, &iEnd);

		for(LONG i = iStart; i <= iEnd; i++)
		{
			BSTR bstr = nullptr;
			LONG indices[1] = {i};
			result = ::SafeArrayGetElement (names, indices, &bstr);
			if(SUCCEEDED (result))
			{
				String string;
				string.appendNativeString (bstr);
				nameList.add (string);
				
				::SysFreeString (bstr);
			}
		}
	}
	::SafeArrayDestroy (names);
	return SUCCEEDED (result);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

bool ManagementObject::getProperty (Variant& value, StringRef name)
{
	value.clear ();
	bool result = false;

	ComVariant var;
	if(SUCCEEDED (object->Get (StringChars (name), 0, &var, nullptr, nullptr)))
		result = var.toVariant (value);

	return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

bool ManagementObject::getPropertyString (String& string, StringRef name)
{
	Variant value;
	bool result = getProperty (value, name);
	string = VariantString (value);
	return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

#if DEBUG
void ManagementObject::dumpAll ()
{
	StringList names;
	getPropertyNames (names);

	ForEach (names, Boxed::String, name)
		if(name->startsWith ("__"))
			continue;

		Variant value;
		getProperty (value, *name);
	
		String string;
		value.toString (string);

		Debugger::print (*name);
		Debugger::print (" = ");
		Debugger::println (string);
	EndFor
}
#endif

//************************************************************************************************
// ManagementRegistry
//************************************************************************************************

bool ManagementRegistry::getUserValue (uint32& value, StringRef name)
{
	Registry::Accessor accessor (Registry::kKeyCurrentUser, CCL_SYSTEMINFORMATION_KEY);	
	return accessor.readDWORD (value, nullptr, name);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

ManagementRegistry::ManagementRegistry (ManagementServices& services)
: services (services)
{}

////////////////////////////////////////////////////////////////////////////////////////////////////

int ManagementRegistry::getCpuClockSpeed () const
{
	String cpuSpeed;

	// new approach: try to read from shared location
	Registry::IniAccessor& sharedAcc = Registry::IniAccessor::getSharedInstance ();
	sharedAcc.readString (cpuSpeed, CCL_SYSTEMINFORMATION_KEY, "CpuClockSpeed");
	if(cpuSpeed.isEmpty ())
	{
		Registry::Accessor accessor (Registry::kKeyCurrentUser, CCL_SYSTEMINFORMATION_KEY);	
		accessor.readString (cpuSpeed, nullptr, "CpuClockSpeed");
		if(cpuSpeed.isEmpty ())
		{
			AutoPtr<Win32::ManagementObject> object;
			AutoPtr<Win32::ManagementEnumerator> enumerator;
			if(enumerator = services.execQuery ("SELECT * FROM Win32_Processor"))
				if(object = enumerator->next ())
				{
					Variant value;
					if(object->getProperty (value, "MaxClockSpeed"))
					{
						cpuSpeed << value.asInt ();
						accessor.writeString (cpuSpeed, nullptr, "CpuClockSpeed");
					}
				}
		}

		// save to shared location
		sharedAcc.writeString (CCL_SYSTEMINFORMATION_KEY, "CpuClockSpeed", cpuSpeed);
	}

	int64 value = 0;
	cpuSpeed.getIntValue (value);
	ASSERT (value != 0)
	return (int)value;
}

//************************************************************************************************
// NetworkAdapterList
//************************************************************************************************

static bool ignoreAdapterByName (StringRef name)
{
	static const String ignoreList[] =
	{
		String ("1394"), String ("firewire"),
		String ("wlan"), String ("wireless"),
		String ("WAN Miniport")
	};

	bool ignore = false;
	for(int i = 0; i < ARRAY_COUNT (ignoreList); i++)
	{
		if(name.contains (ignoreList[i], false))
		{
			ignore = true;
			break;
		}
	}

	return ignore;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void NetworkAdapterList::collect (ManagementServices& services)
{
	AutoPtr<Win32::ManagementEnumerator> enumerator;
	AutoPtr<Win32::ManagementObject> object;
	if(enumerator = services.execQuery ("SELECT * from Win32_NetworkAdapter WHERE AdapterType=\"Ethernet 802.3\" AND PhysicalAdapter=true"))
		while(object = enumerator->next ())
		{
			String pnpId;
			object->getPropertyString (pnpId, "PNPDeviceID");
			pnpId.trimWhitespace ();
			
			String name;
			object->getPropertyString (name, "Name");
			
			String macAddress;
			object->getPropertyString (macAddress, "MACAddress");

			//ASSERT (!pnpId.isEmpty () && !name.isEmpty () && !macAddress.isEmpty ())
			if(pnpId.isEmpty () || name.isEmpty () || macAddress.isEmpty ())
				continue;

			#if 1 // filter virtual interfaces (should be out already with PhysicalAdapter=true)
			if(pnpId.startsWith ("root\\", false))
				continue;
			#endif

			#if 1 // filter firewire, wireless, etc.
			if(ignoreAdapterByName (name))
				continue;
			#endif

			#if DEBUG_LOG
			object->dumpAll ();
			#endif

			adapters.add (AdapterInfo (name, macAddress));
		}
}

////////////////////////////////////////////////////////////////////////////////////////////////////

bool NetworkAdapterList::getPrimaryAdapterInfo (AdapterInfo& info)
{
	String primaryAdapterName;
	Registry::Accessor accessor (Registry::kKeyCurrentUser, CCL_SYSTEMINFORMATION_KEY);
	accessor.readString (primaryAdapterName, nullptr, "PrimaryNetworkAdapter");
	
	AdapterInfo* primaryAdapterInfo = nullptr;
	if(!primaryAdapterName.isEmpty ())
		primaryAdapterInfo = findAdapter (primaryAdapterName);

	if(primaryAdapterInfo == nullptr)
		primaryAdapterInfo = getFirstAdapter ();

	ASSERT (primaryAdapterInfo != nullptr)
	if(primaryAdapterInfo == nullptr)
		return false;

	if(primaryAdapterInfo->name != primaryAdapterName)
		accessor.writeString (primaryAdapterInfo->name, nullptr, "PrimaryNetworkAdapter");

	info = *primaryAdapterInfo;
	return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

NetworkAdapterList::AdapterInfo* NetworkAdapterList::getFirstAdapter ()
{
	return adapters.count () >= 1 ? &adapters.at (0) : nullptr;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

NetworkAdapterList::AdapterInfo* NetworkAdapterList::findAdapter (StringRef name)
{
	for(int i = 0; i < adapters.count (); i++)
	{
		AdapterInfo& info = adapters.at (i);
		if(info.name.compare (name, false) == 0)
			return &info;
	}
	return nullptr;
}
