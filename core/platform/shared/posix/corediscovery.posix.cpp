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
// Filename    : core/platform/shared/posix/corediscovery.posix.cpp
// Description : DNS Service Discovery POSIX implementation
//
//************************************************************************************************

#include "core/platform/corefeatures.h"

#if CORE_DISCOVERY_IMPLEMENTATION == CORE_PLATFORM_IMPLEMENTATION
#include CORE_PLATFORM_IMPLEMENTATION_HEADER (corediscovery)
#else
#include "core/platform/shared/posix/corediscovery.posix.h"
#endif

#if CORE_DISCOVERY_IMPLEMENTATION == CORE_FEATURE_IMPLEMENTATION_POSIX
#include <arpa/inet.h>
#endif

using namespace Core;
using namespace Sockets;
using namespace Platform;

//************************************************************************************************
// Helpers
//************************************************************************************************

struct ResolveContext
{
	CStringPtr serviceName;
	CStringPtr regtype;
	BrowseReplyHandler* replyHandler;
};

//////////////////////////////////////////////////////////////////////////////////////////////////

class RegTypeString: public CString64
{
public:
	RegTypeString (const TypeDescriptor& descriptor)
	{
		ASSERT (descriptor.protocol == kTCP || descriptor.protocol == kUDP)
		appendFormat ("_%s._%s.", descriptor.type, descriptor.protocol == kTCP ? "tcp" : "udp");
	}

	RegTypeString (CStringPtr regtype)
	: CString64 (regtype)
	{}

	void getType (CString64& type) const	{ subString (type, 1, index (".")-1); }
	ProtocolType getProtocol () const		{ return contains ("._tcp") ? kTCP : kUDP; }
};

//************************************************************************************************
// Callbacks
//************************************************************************************************

static void DNSSD_API DNSServiceRegisterReplyHandler
    (
    DNSServiceRef                       sdRef,
    DNSServiceFlags                     flags,
    DNSServiceErrorType                 errorCode,
    const char                          *name,
    const char                          *_regtype,
    const char                          *domain,
    void                                *context
    )
{
	RegisterReplyHandler* handler = reinterpret_cast<RegisterReplyHandler*> (context);
	if(errorCode == kDNSServiceErr_NoError)
	{
		CString64 type;
		RegTypeString regtype (_regtype);
		regtype.getType (type);

		ServiceDescriptor descriptor;
		descriptor.type = type;
		descriptor.protocol = regtype.getProtocol ();
		descriptor.serviceName = name;

		handler->onServiceRegistered (sdRef, descriptor);
	}
	else
	{
		handler->onServiceRegistrationFailed (sdRef);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

static void DNSSD_API DNSServiceResolveReplyHandler
    (
    DNSServiceRef                       sdRef,
    DNSServiceFlags                     flags,
    uint32_t                            interfaceIndex,
    DNSServiceErrorType                 errorCode,
    const char                          *fullname,
    const char                          *hosttarget,
    uint16_t                            port,
    uint16_t                            txtLen,
    const char                          *txtRecord,
    void                                *context
    )
{
	if(errorCode != kDNSServiceErr_NoError)
		return;

	ResolveContext* resolveContext = reinterpret_cast<ResolveContext*> (context);

	RegTypeString regtype (resolveContext->regtype);
	CString64 type;
	regtype.getType (type);

	ServiceTargetDescriptor descriptor;
	descriptor.type = type;
	descriptor.protocol = regtype.getProtocol ();
	descriptor.serviceName = resolveContext->serviceName;
	descriptor.hostname = hosttarget;
	descriptor.port = ntohs (port);

	TextRecord textRecord (txtRecord, txtLen);
	descriptor.textRecord = &textRecord;

	resolveContext->replyHandler->onServiceResolved (sdRef, descriptor);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

static void DNSSD_API DNSServiceBrowseReplyHandler
    (
    DNSServiceRef                       sdRef,
    DNSServiceFlags                     flags,
    uint32_t                            interfaceIndex,
    DNSServiceErrorType                 errorCode,
    const char                          *serviceName,
    const char                          *_regtype,
    const char                          *replyDomain,
    void                                *context
    )
{
	if(errorCode != kDNSServiceErr_NoError)
		return;

	BrowseReplyHandler* handler = reinterpret_cast<BrowseReplyHandler*> (context);

	if(flags & kDNSServiceFlagsAdd)
	{
		// service added => resolve its address and text record

		DNSServiceRef sdResolveRef = nullptr;
		ResolveContext resolveContext = {serviceName, _regtype, handler};
		::DNSServiceResolve (&sdResolveRef, 0, interfaceIndex, serviceName, _regtype, replyDomain, (DNSServiceResolveReply)DNSServiceResolveReplyHandler, &resolveContext);
		::DNSServiceProcessResult (sdResolveRef);
		::DNSServiceRefDeallocate (sdResolveRef);
	}
	else
	{
		// service removed
		RegTypeString regtype (_regtype);
		CString64 type;
		regtype.getType (type);

		ServiceDescriptor descriptor;
		descriptor.type = type;
		descriptor.protocol = regtype.getProtocol ();
		descriptor.serviceName = serviceName;

		handler->onServiceRemoved (sdRef, descriptor);
	}
}

//************************************************************************************************
// DiscoveryHandler
//************************************************************************************************

#if CORE_DISCOVERY_IMPLEMENTATION == CORE_FEATURE_IMPLEMENTATION_POSIX

bool DiscoveryHandler::isInstalled ()
{
	return PosixDiscoveryHandler::isInstalled ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<>
bool DiscoveryHandler::registerBrowser (DiscoveryRef& sdRef, const TypeDescriptor& descriptor, BrowseReplyHandler* replyHandler)
{
	return PosixDiscoveryHandler::registerBrowser (sdRef, descriptor, replyHandler);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<>
bool DiscoveryHandler::registerService (DiscoveryRef& sdRef, const ServiceTargetDescriptor& descriptor, RegisterReplyHandler* replyHandler)
{
	return PosixDiscoveryHandler::registerService (sdRef, descriptor, replyHandler);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<>
void DiscoveryHandler::unregisterReference (DiscoveryRef sdRef)
{
	PosixDiscoveryHandler::unregisterReference (sdRef);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<>
bool DiscoveryHandler::processResults (const DiscoveryRef sdRefList[], int count)
{
	return PosixDiscoveryHandler::processResults (sdRefList, count);
}

#endif

//************************************************************************************************
// PosixDiscoveryHandler
//************************************************************************************************

bool PosixDiscoveryHandler::isInstalled ()
{
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PosixDiscoveryHandler::registerBrowser (DiscoveryRef& sdRef, const TypeDescriptor& descriptor, BrowseReplyHandler* replyHandler)
{
	RegTypeString regtype (descriptor);

	sdRef = nullptr;
	DNSServiceErrorType error = ::DNSServiceBrowse (&sdRef, 0/*flags*/, 0/*interfaceIndex*/, regtype,
									nullptr/*domain*/, DNSServiceBrowseReplyHandler, replyHandler);

	return error == kDNSServiceErr_NoError;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PosixDiscoveryHandler::registerService (DiscoveryRef& sdRef, const ServiceTargetDescriptor& descriptor, RegisterReplyHandler* replyHandler)
{
	RegTypeString regtype (descriptor);
	PortNumber port = htons (descriptor.port);

	uint16_t txtLen = descriptor.textRecord ? descriptor.textRecord->length : 0;
	const void* txtRecord = descriptor.textRecord ? descriptor.textRecord->buffer : nullptr;

	sdRef = nullptr;
	DNSServiceErrorType error = ::DNSServiceRegister (&sdRef, 0/*flags*/, 0/*interfaceIndex*/, descriptor.serviceName, regtype, 
									nullptr/*domain*/, descriptor.hostname, port, txtLen, txtRecord, 
									DNSServiceRegisterReplyHandler, replyHandler);

	return error == kDNSServiceErr_NoError;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PosixDiscoveryHandler::unregisterReference (DiscoveryRef sdRef)
{
	::DNSServiceRefDeallocate (sdRef);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PosixDiscoveryHandler::processResults (const DiscoveryRef sdRefList[], int count)
{
	fd_set readfds;
	FD_ZERO (&readfds);
	int nfds = -1;

	for(int i = 0; i < count; i++)
	{
		int socket = ::DNSServiceRefSockFD (sdRefList[i]);
		ASSERT (socket != INVALID_SOCKET)
		FD_SET (socket, &readfds);
		if(socket+1 > nfds)
			nfds = socket+1;
	}

	bool anyActivity = false;
	timeval timeout = {0, 0};
	int result = ::select (nfds, &readfds, nullptr, nullptr, &timeout);
	if(result > 0)
	{
		for(int i = 0; i < count; i++)
			if(FD_ISSET (::DNSServiceRefSockFD (sdRefList[i]), &readfds))
			{
				anyActivity = true;
				::DNSServiceProcessResult (sdRefList[i]);
			}
	}
	return anyActivity;
}

//************************************************************************************************
// PosixTextRecord
//************************************************************************************************

PosixTextRecord::PosixTextRecord (const void* buffer, uint16_t length)
: buffer (buffer),
  length (length)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

PosixTextRecord::~PosixTextRecord ()
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

int PosixTextRecord::getCount () const
{
	return ::TXTRecordGetCount (length, buffer);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PosixTextRecord::getItemAt (CString64& key, CString64& value, int index) const
{
	const char* valuePtr = nullptr;
	uint8_t valueLen = 0;
	
	DNSServiceErrorType error = ::TXTRecordGetItemAtIndex (length, buffer, (uint16_t)index, (uint16_t)key.getSize (), key.getBuffer (), &valueLen, (const void**)&valuePtr);
	if(error != kDNSServiceErr_NoError)
		return false;

	value.empty ();
	value.append (valuePtr, valueLen);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PosixTextRecord::getValue (CString64& value, CStringPtr key) const
{
	uint8_t valueLen = 0;
	const char* valuePtr = (const char*)::TXTRecordGetValuePtr (length, buffer, key, &valueLen);
	if(!valuePtr)
		return false;

	value.append (valuePtr, valueLen);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PosixTextRecord::getIntValue (int64& value, CStringPtr key) const
{
	CString64 string;
	if(!getValue (string, key))
		return false;

	return string.getIntValue (value);
}

//************************************************************************************************
// TextRecordBuilder
//************************************************************************************************

PosixTextRecordBuilder::PosixTextRecordBuilder ()
{
	::TXTRecordCreate (&textRecord, kMaxBufferSize, textRecordBuffer);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

PosixTextRecordBuilder::~PosixTextRecordBuilder ()
{
	::TXTRecordDeallocate (&textRecord);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PosixTextRecordBuilder::setValue (CStringPtr key, CStringPtr value)
{
	::TXTRecordSetValue (&textRecord, key, (uint8_t)::strlen (value), value);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PosixTextRecordBuilder::setIntValue (CStringPtr key, int64 value)
{
	CString64 string;
	string.appendFormat ("%" FORMAT_INT64 "d", value);
	setValue (key, string);	
}

//////////////////////////////////////////////////////////////////////////////////////////////////

PosixTextRecord PosixTextRecordBuilder::getTextRecord () const
{
	ASSERT (::TXTRecordGetLength (&textRecord) <= kMaxBufferSize)
	return TextRecord (::TXTRecordGetBytesPtr (&textRecord), ::TXTRecordGetLength (&textRecord));
}
