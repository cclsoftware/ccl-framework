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
// Filename    : ccl/platform/win/system/management.h
// Description : Windows Management Instrumentation (WMI) Helper Classes
//
//************************************************************************************************

#ifndef _ccl_win32_management_h
#define _ccl_win32_management_h

#include "ccl/public/base/unknown.h"
#include "ccl/public/base/variant.h"
#include "ccl/public/collections/vector.h"

struct IWbemServices;
struct IWbemLocator;
struct IEnumWbemClassObject;
struct IWbemClassObject;

namespace CCL {
class StringList;

namespace Win32 {

class ManagementObject;
class ManagementEnumerator;

//************************************************************************************************
// ManagementServices
//************************************************************************************************

class ManagementServices: public Unknown
{
public:
	static const String kRootNamespace;

	ManagementServices (StringRef resourceName = kRootNamespace);
	~ManagementServices ();

	bool isValid () const;
	ManagementEnumerator* execQuery (StringRef query); ///< enumerator must be released by caller!

	#if DEBUG
	void dumpAll ();
	#endif

protected:
	IWbemLocator* locator;
	IWbemServices* services;

	void construct (StringRef resourceName);
};

//************************************************************************************************
// ManagementEnumerator
//************************************************************************************************

class ManagementEnumerator: public Unknown
{
public:
	ManagementEnumerator (IEnumWbemClassObject* enumerator);
	~ManagementEnumerator ();

	ManagementObject* next (); ///< object must be released by caller!

protected:
	IEnumWbemClassObject* enumerator;
};

//************************************************************************************************
// ManagementObject
//************************************************************************************************

class ManagementObject: public Unknown
{
public:
	ManagementObject (IWbemClassObject* object);
	~ManagementObject ();

	bool getPropertyNames (CCL::StringList& names);
	bool getProperty (Variant& value, StringRef name);
	bool getPropertyString (String& string, StringRef name);

	#if DEBUG
	void dumpAll ();
	#endif

protected:
	IWbemClassObject* object;
};

//************************************************************************************************
// ManagementRegistry
//************************************************************************************************

class ManagementRegistry
{
public:
	ManagementRegistry (ManagementServices& services);

	static bool getUserValue (uint32& value, StringRef name);

	int getCpuClockSpeed () const;

protected:
	ManagementServices& services;
};

//************************************************************************************************
// NetworkAdapterList
//************************************************************************************************

class NetworkAdapterList
{
public:
	struct AdapterInfo
	{
		String name;
		String macAddress;

		AdapterInfo (StringRef name = nullptr,
					 StringRef macAddress = nullptr)
		: name (name),
		  macAddress (macAddress)
		{}
	};

	void collect (ManagementServices& services);
	bool getPrimaryAdapterInfo (AdapterInfo& info);
	const ConstVector<AdapterInfo>& getAdapters () const { return adapters; }

protected:
	Vector<AdapterInfo> adapters;

	AdapterInfo* getFirstAdapter ();
	AdapterInfo* findAdapter (StringRef name);
};

} // namespace Win32
} // namespace CCL

#endif // _ccl_win32_management_h
