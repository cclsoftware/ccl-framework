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
// Filename    : core/platform/win/corediscovery.win.cpp
// Description : DNS Service Discovery Windows implementation
//
//************************************************************************************************

#include "core/platform/win/corediscovery.win.h"

#include "core/network/coresocket.h"
#include "core/system/corethread.h"

#pragma comment (lib, "dnsapi.lib")

namespace Core {
namespace Platform {

//************************************************************************************************
// RegTypeString
//************************************************************************************************

class RegTypeString
{
public:
	RegTypeString ()
	{}

	RegTypeString (const TypeDescriptor& descriptor)
	{
		ASSERT (descriptor.protocol == Sockets::kTCP || descriptor.protocol == Sockets::kUDP)
		_snwprintf (buffer, kBufferSize, L"_%S._%S.local", descriptor.type, descriptor.protocol == Sockets::kTCP ? "tcp" : "udp");
	}

	void assign (UStringPtr regtype)
	{
		_snwprintf (buffer, kBufferSize, L"%s", regtype);
	}

	void getType (CString64& type) const
	{
		if(const wchar_t* end = wcschr (buffer, '.'))
			snprintf (type.getBuffer (), end - buffer, "%S", buffer + 1);
	}

	Sockets::ProtocolType getProtocol () const
	{
		return wcsstr (buffer, L"._tcp") ? Sockets::kTCP : Sockets::kUDP;
	}

	operator UStringPtr () const { return buffer; }

private:
	static constexpr int kBufferSize = 64;
	uchar buffer[kBufferSize] = { 0 };
};

//************************************************************************************************
// DiscoveryContext
//************************************************************************************************

class DiscoveryContext
{
public:
	virtual ~DiscoveryContext () {}

	virtual bool processResults () = 0;

protected:
	static void hostNameFromWideString (CString256& hostName, const wchar_t* wideString);
	static void serviceNameFromInstanceName (CString64& serviceName, const wchar_t* instanceName);
	static void regTypeFromInstanceName (RegTypeString& regtype, const wchar_t* instanceName);

	DNS_SERVICE_CANCEL cancelHandler = {};
};

//************************************************************************************************
// DiscoveryBrowseContext
//************************************************************************************************

class DiscoveryBrowseContext: public DiscoveryContext
{
public:
	DiscoveryBrowseContext (const TypeDescriptor& descriptor, BrowseReplyHandler* browseHandler);
	~DiscoveryBrowseContext ();

	bool registerBrowser ();

	// DiscoveryContext
	bool processResults () override;

private:
	RegTypeString regType;
	BrowseReplyHandler* browseHandler;

	DNS_SERVICE_BROWSE_REQUEST browseRequest;

	Threads::Lock lock;
	Vector<PDNS_RECORD> records;

	static void WINAPI browseCallback (DWORD status, PVOID pQueryContext, PDNS_RECORD pDnsRecord);
};

//************************************************************************************************
// DiscoveryServiceContext
//************************************************************************************************

class DiscoveryServiceContext: public DiscoveryContext
{
public:
	DiscoveryServiceContext (const ServiceTargetDescriptor& descriptor, RegisterReplyHandler* registerHandler);
	~DiscoveryServiceContext ();

	bool registerService ();

	// DiscoveryContext
	bool processResults () override;

private:
	RegisterReplyHandler* registerHandler;

	DNS_SERVICE_REGISTER_REQUEST registerRequest;
	DNS_SERVICE_INSTANCE serviceInstance;

	void setServiceInformation (const ServiceTargetDescriptor& descriptor);
	void setInstanceName (CStringPtr service, const TypeDescriptor& type);
	void setHostName (CStringPtr host);
	void useLocalHostName ();
	void setTextRecord (const TextRecord& textRecord);

	static void normalizeServiceName (CString64& serviceName);
	static Sockets::PortNumber findPort ();

	Threads::Lock lock;
	bool processed;
	PDNS_SERVICE_INSTANCE instance;

	static void WINAPI registerCompletionCallback (DWORD status, PVOID pQueryContext, PDNS_SERVICE_INSTANCE pInstance);
};

}}

using namespace Core;
using namespace Sockets;
using namespace Platform;

//************************************************************************************************
// DiscoveryContext
//************************************************************************************************

void DiscoveryContext::hostNameFromWideString (CString256& hostName, const wchar_t* wideString)
{
	snprintf (hostName.getBuffer (), hostName.getSize (), "%S", wideString);

	if(hostName.endsWith (".local"))
		hostName.append (".");
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DiscoveryContext::serviceNameFromInstanceName (CString64& serviceName, const wchar_t* instanceName)
{
	if(const wchar_t* end = wcsstr (instanceName, L"._"))
		snprintf (serviceName.getBuffer (), end - instanceName + 1, "%S", instanceName);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DiscoveryContext::regTypeFromInstanceName (RegTypeString& regType, const wchar_t* instanceName)
{
	if(const wchar_t* start = wcsstr (instanceName, L"._") + 1)
		regType.assign (start);
}

//************************************************************************************************
// DiscoveryBrowseContext
//************************************************************************************************

DiscoveryBrowseContext::DiscoveryBrowseContext (const TypeDescriptor& descriptor, BrowseReplyHandler* browseHandler)
: regType (descriptor),
  browseHandler (browseHandler),
  browseRequest {}
{
	browseRequest.Version = DNS_QUERY_REQUEST_VERSION1;
	browseRequest.QueryName = regType;
	browseRequest.pBrowseCallback = &browseCallback;
	browseRequest.pQueryContext = this;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DiscoveryBrowseContext::~DiscoveryBrowseContext ()
{
	::DnsServiceBrowseCancel (&cancelHandler);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool DiscoveryBrowseContext::registerBrowser ()
{
	return ::DnsServiceBrowse (&browseRequest, &cancelHandler) == DNS_REQUEST_PENDING;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool DiscoveryBrowseContext::processResults ()
{
	Threads::ScopedLock scope (lock);

	if(records.count () == 0)
		return false;

	for(PDNS_RECORD record : records)
	{
		bool isAlive = false;
		CString64 serviceName;
		CString256 hostName;
		RegTypeString regType;
		Sockets::PortNumber port = 0;
		Vector<DNS_TXT_DATA*> txtRecords;

		for(PDNS_RECORD current = record; current; current = current->pNext)
		{
			switch(current->wType)
			{
			case DNS_TYPE_PTR:
				serviceNameFromInstanceName (serviceName, current->Data.Ptr.pNameHost);
				regType.assign (current->pName);
				isAlive = (current->dwTtl != 0);
				break;
			case DNS_TYPE_SRV:
				hostNameFromWideString (hostName, current->Data.Srv.pNameTarget);
				port = current->Data.Srv.wPort;
				break;
			case DNS_TYPE_TEXT:
				txtRecords.add (&current->Data.Txt);
				break;
			}
		}

		CString64 type;
		regType.getType (type);

		if(isAlive)
		{
			// service added
			ServiceTargetDescriptor descriptor;
			descriptor.type = type;
			descriptor.protocol = regType.getProtocol ();
			descriptor.serviceName = serviceName;
			descriptor.hostname = hostName;
			descriptor.port = port;

			TextRecord textRecord (txtRecords);
			descriptor.textRecord = &textRecord;

			browseHandler->onServiceResolved (this, descriptor);
		}
		else
		{
			// service removed
			ServiceDescriptor descriptor;
			descriptor.type = type;
			descriptor.protocol = regType.getProtocol ();
			descriptor.serviceName = serviceName;

			browseHandler->onServiceRemoved (this, descriptor);
		}

		::DnsRecordListFree (record, DnsFreeRecordList);
	}

	records.empty ();

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WINAPI DiscoveryBrowseContext::browseCallback (DWORD status, PVOID pQueryContext, PDNS_RECORD pDnsRecord)
{
	DiscoveryBrowseContext* context = reinterpret_cast<DiscoveryBrowseContext*> (pQueryContext);

	if(context && status == ERROR_SUCCESS)
	{
		Threads::ScopedLock scope (context->lock);

		context->records.add (pDnsRecord);
	}
	else if(pDnsRecord)
		::DnsRecordListFree (pDnsRecord, DnsFreeRecordList);
}

//************************************************************************************************
// DiscoveryServiceContext
//************************************************************************************************

DiscoveryServiceContext::DiscoveryServiceContext (const ServiceTargetDescriptor& descriptor, RegisterReplyHandler* registerHandler)
: registerHandler (registerHandler),
  registerRequest {},
  serviceInstance {},
  processed (false),
  instance (nullptr)
{
	registerRequest.Version = DNS_QUERY_REQUEST_VERSION1;
	registerRequest.pServiceInstance = &serviceInstance;
	registerRequest.pRegisterCompletionCallback = &registerCompletionCallback;
	registerRequest.pQueryContext = this;

	setServiceInformation (descriptor);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DiscoveryServiceContext::~DiscoveryServiceContext ()
{
	registerRequest.pQueryContext = nullptr;

	::DnsServiceDeRegister (&registerRequest, nullptr);

	delete[] serviceInstance.pszInstanceName;
	delete[] serviceInstance.pszHostName;

	for(int i = 0; i < serviceInstance.dwPropertyCount; i++)
	{
		delete[] serviceInstance.keys[i];
		delete[] serviceInstance.values[i];
	}

	delete[] serviceInstance.keys;
	delete[] serviceInstance.values;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool DiscoveryServiceContext::registerService ()
{
	return ::DnsServiceRegister (&registerRequest, &cancelHandler) == DNS_REQUEST_PENDING;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DiscoveryServiceContext::setServiceInformation (const ServiceTargetDescriptor& descriptor)
{
	// instance name
	CString64 serviceName = descriptor.serviceName;
	normalizeServiceName (serviceName);
	setInstanceName (serviceName, descriptor);

	// host name
	if(descriptor.hostname && descriptor.hostname[0] != 0)
		setHostName (descriptor.hostname);
	else
		useLocalHostName ();

	// port
	if(descriptor.port)
		serviceInstance.wPort = descriptor.port;
	else
		serviceInstance.wPort = findPort ();

	// txt data
	setTextRecord (*descriptor.textRecord);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DiscoveryServiceContext::setInstanceName (CStringPtr service, const TypeDescriptor& type)
{
	int instanceNameLength = strlen (service) + strlen (type.type) + 14;

	serviceInstance.pszInstanceName = NEW wchar_t[instanceNameLength];

	_snwprintf (serviceInstance.pszInstanceName, instanceNameLength, L"%S._%S._%S.local", service, type.type, type.protocol == kTCP ? "tcp" : "udp");
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DiscoveryServiceContext::setHostName (CStringPtr host)
{
	CString256 hostName = host;
	if(!hostName.endsWith (".local"))
		hostName.append (".local");

	int hostNameLength = hostName.length () + 1;

	serviceInstance.pszHostName = NEW wchar_t[hostNameLength];

	_snwprintf (serviceInstance.pszHostName, hostNameLength, L"%S", hostName.getBuffer ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DiscoveryServiceContext::useLocalHostName ()
{
	CString256 hostName;
	Sockets::Network::getLocalHostname (hostName);
	setHostName (hostName);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DiscoveryServiceContext::setTextRecord (const TextRecord& textRecord)
{
	serviceInstance.dwPropertyCount = textRecord.getCount ();

	if(serviceInstance.dwPropertyCount)
	{
		serviceInstance.keys = NEW wchar_t* [serviceInstance.dwPropertyCount];
		serviceInstance.values = NEW wchar_t* [serviceInstance.dwPropertyCount];

		for(int i = 0; i < serviceInstance.dwPropertyCount; i++)
		{
			CString64 key;
			CString64 value;
			textRecord.getItemAt (key, value, i);

			int keyLength = key.length () + 1;
			int valueLength = value.length () + 1;

			serviceInstance.keys[i] = NEW wchar_t[keyLength];
			serviceInstance.values[i] = NEW wchar_t[valueLength];

			_snwprintf (serviceInstance.keys[i], keyLength, L"%S", key.getBuffer ());
			_snwprintf (serviceInstance.values[i], valueLength, L"%S", value.getBuffer ());
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DiscoveryServiceContext::normalizeServiceName (CString64& serviceName)
{
	// the Windows mDNS implementation has issues with dots in service names
	serviceName.replace ('.', '_');
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Sockets::PortNumber DiscoveryServiceContext::findPort ()
{
	IPAddress address;
	address.setIP (127, 0, 0, 1);

	// use bind to figure out a free port
	Sockets::Socket socket (kInternet, kStream, kTCP);
	if(socket.bind (address))
	{
		socket.getLocalAddress (address);
		socket.disconnect ();

		return address.port;
	}

	return 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool DiscoveryServiceContext::processResults ()
{
	Threads::ScopedLock scope (lock);

	if(!processed)
		return false;

	if(instance)
	{
		CString64 serviceName;
		serviceNameFromInstanceName (serviceName, instance->pszInstanceName);

		RegTypeString regType;
		regTypeFromInstanceName (regType, instance->pszInstanceName);

		CString64 type;
		regType.getType (type);

		ServiceDescriptor descriptor;
		descriptor.type = type;
		descriptor.protocol = regType.getProtocol ();
		descriptor.serviceName = serviceName;

		registerHandler->onServiceRegistered (this, descriptor);

		::DnsServiceFreeInstance (instance);
	}
	else
		registerHandler->onServiceRegistrationFailed (this);

	processed = false;
	instance = nullptr;

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WINAPI DiscoveryServiceContext::registerCompletionCallback (DWORD status, PVOID pQueryContext, PDNS_SERVICE_INSTANCE pInstance)
{
	DiscoveryServiceContext* context = reinterpret_cast<DiscoveryServiceContext*> (pQueryContext);

	if(context)
	{
		Threads::ScopedLock scope (context->lock);

		if(status == ERROR_SUCCESS)
			context->instance = pInstance;
		else if(pInstance)
			::DnsServiceFreeInstance (pInstance);

		context->processed = true;
	}
	else if(pInstance)
		::DnsServiceFreeInstance (pInstance);
}

//************************************************************************************************
// DiscoveryHandler
//************************************************************************************************

bool DiscoveryHandler::isInstalled ()
{
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<>
bool DiscoveryHandler::registerBrowser (DiscoveryRef& sdRef, const TypeDescriptor& descriptor, BrowseReplyHandler* replyHandler)
{
	DiscoveryBrowseContext* context = NEW DiscoveryBrowseContext (descriptor, replyHandler);

	sdRef = context;

	return context->registerBrowser ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<>
bool DiscoveryHandler::registerService (DiscoveryRef& sdRef, const ServiceTargetDescriptor& descriptor, RegisterReplyHandler* replyHandler)
{
	DiscoveryServiceContext* context = NEW DiscoveryServiceContext (descriptor, replyHandler);

	sdRef = context;

	return context->registerService ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<>
void DiscoveryHandler::unregisterReference (DiscoveryRef sdRef)
{
	delete sdRef;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<>
bool DiscoveryHandler::processResults (const DiscoveryRef sdRefList[], int count)
{
	bool anyActivity = false;
	for(int i = 0; i < count; i++)
		anyActivity |= const_cast<DiscoveryContext*> (sdRefList[i])->processResults ();

	return anyActivity;
}

//************************************************************************************************
// WindowsTextRecord
//************************************************************************************************

WindowsTextRecord::WindowsTextRecord (const ConstVector<DNS_TXT_DATA*>& records)
: records (records)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

int WindowsTextRecord::getCount () const
{
	int count = 0;
	for(int i = 0; i < records.count (); i++)
		count += records[i]->dwStringCount;
	return count;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool WindowsTextRecord::getItemAt (CString64& key, CString64& value, int index) const
{
	int count = 0;
	for(int i = 0; i < records.count (); i++)
	{
		if(records[i]->dwStringCount > index - count)
		{
			wchar_t* string = records[i]->pStringArray[index - count];

			if(const wchar_t* split = wcschr (string, '='))
			{
				snprintf (key.getBuffer (), split - string + 1, "%S", string);
				snprintf (value.getBuffer (), value.getSize (), "%S", split + 1);

				return true;
			}

			return false;
		}

		count += records[i]->dwStringCount;
	}

	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool WindowsTextRecord::getValue (CString64& value, CStringPtr key) const
{
	int count = getCount ();
	for(int i = 0; i < count; i++)
	{
		CString64 iKey, iValue;
		getItemAt (iKey, iValue, i);
		if(iKey != key)
			continue;

		value = iValue;
		return true;
	}

	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool WindowsTextRecord::getIntValue (int64& value, CStringPtr key) const
{
	CString64 string;
	if(!getValue (string, key))
		return false;

	return string.getIntValue (value);
}

//************************************************************************************************
// WindowsTextRecordBuilder
//************************************************************************************************

WindowsTextRecordBuilder::~WindowsTextRecordBuilder ()
{
	for(int i = 0; i < records.count (); i++)
	{
		delete[] records[i]->pStringArray[0];
		delete records[i];
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WindowsTextRecordBuilder::setValue (CStringPtr key, CStringPtr value)
{
	DNS_TXT_DATA* record = NEW DNS_TXT_DATA;
	int entryLength = strlen (key) + strlen (value) + 2;

	record->dwStringCount = 1;
	record->pStringArray[0] = NEW wchar_t[entryLength];

	_snwprintf (record->pStringArray[0], entryLength, L"%S=%S", key, value);
	records.add (record);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WindowsTextRecordBuilder::setIntValue (CStringPtr key, int64 value)
{
	CString64 string;
	string.appendFormat ("%" FORMAT_INT64 "d", value);
	setValue (key, string);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

WindowsTextRecord WindowsTextRecordBuilder::getTextRecord () const
{
	return WindowsTextRecord (records);
}
