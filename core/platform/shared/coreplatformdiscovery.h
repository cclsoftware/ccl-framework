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
// Filename    : core/platform/shared/coreplatformdiscovery.h
// Description : DNS Service Discovery platform implementation base
//
//************************************************************************************************

#ifndef _coreplatformdiscovery_h
#define _coreplatformdiscovery_h

#include "core/public/coresocketaddress.h"
#include "core/public/corestringbuffer.h"

namespace Core {
namespace Platform {

//************************************************************************************************
// ITextRecord
//************************************************************************************************

struct ITextRecord
{
	virtual ~ITextRecord () {}

	virtual int getCount () const = 0;
	virtual bool getItemAt (CString64& key, CString64& value, int index) const = 0;
	virtual bool getValue (CString64& value, CStringPtr key) const = 0;
	virtual bool getIntValue (int64& value, CStringPtr key) const = 0;
};

//************************************************************************************************
// ITextRecordBuilder
/** Note: Text record keys should be max. 8 characters long! */
//************************************************************************************************

template<typename TextRecord>
class ITextRecordBuilder
{
public:
	virtual ~ITextRecordBuilder () {}

	virtual void setValue (CStringPtr key, CStringPtr value) = 0;
	virtual void setIntValue (CStringPtr key, int64 value) = 0;
	virtual TextRecord getTextRecord () const = 0;
};

//************************************************************************************************
// TypeDescriptor
//************************************************************************************************

struct TypeDescriptor
{
	CStringPtr type;			///< application-specific service type
	Sockets::ProtocolType protocol;		///< kTCP or kUDP

	TypeDescriptor ()
	: type (nullptr),
	  protocol (0)
	{}
};

//************************************************************************************************
// ServiceDescriptor
//************************************************************************************************

struct ServiceDescriptor: TypeDescriptor
{
	CStringPtr serviceName;		///< service name displayed to user

	ServiceDescriptor ()
	: serviceName (nullptr)
	{}
};

//************************************************************************************************
// ServiceTargetDescriptorBase
//************************************************************************************************

template<typename TextRecord>
struct ServiceTargetDescriptorBase: ServiceDescriptor
{
	CStringPtr hostname;			///< target host name, can be used with CoreNetwork::getAddressByHost()
	Sockets::PortNumber port;		///< port number
	const TextRecord* textRecord;	///< text record

	ServiceTargetDescriptorBase ()
	: hostname (nullptr),
	  port (0),
	  textRecord (nullptr)
	{}
};

//************************************************************************************************
// BrowseReplyHandlerBase
//************************************************************************************************

template<typename TextRecord, typename DiscoveryRef>
struct BrowseReplyHandlerBase
{
	virtual ~BrowseReplyHandlerBase () {}

	virtual void onServiceResolved (DiscoveryRef sdRef, const ServiceTargetDescriptorBase<TextRecord>& descriptor) = 0;

	virtual void onServiceRemoved (DiscoveryRef sdRef, const ServiceDescriptor& descriptor) = 0;
};

//************************************************************************************************
// RegisterReplyHandlerBase
//************************************************************************************************

template<typename DiscoveryRef>
struct RegisterReplyHandlerBase
{
	virtual ~RegisterReplyHandlerBase () {}

	/** Note: Service name might change upon registration when a name conflict occurred! */
	virtual void onServiceRegistered (DiscoveryRef sdRef, const ServiceDescriptor& descriptor) = 0;

	virtual void onServiceRegistrationFailed (DiscoveryRef sdRef) = 0;
};

//************************************************************************************************
// DiscoveryHandler
/** Handler for DNS Service Discovery. */
//************************************************************************************************

class DiscoveryHandler
{
public:
	/** Check if DNSSD is installed. */
	static bool isInstalled ();

	/** Register browser for given service type. */
	template<typename TextRecord, typename DiscoveryRef>
	static bool registerBrowser (DiscoveryRef& sdRef, const TypeDescriptor& descriptor, BrowseReplyHandlerBase<TextRecord, DiscoveryRef>* replyHandler);

	/** Register service. */
	template<typename TextRecord, typename DiscoveryRef>
	static bool registerService (DiscoveryRef& sdRef, const ServiceTargetDescriptorBase<TextRecord>& descriptor, RegisterReplyHandlerBase<DiscoveryRef>* replyHandler);

	/** Unregister given reference (browser or service). */
	template<typename DiscoveryRef>
	static void unregisterReference (DiscoveryRef sdRef);

	/** Process results for given reference (browser or service). */
	template<typename DiscoveryRef>
	static bool processResults (const DiscoveryRef sdRefList[], int count);
};

#if CORE_DISCOVERY_IMPLEMENTATION == CORE_FEATURE_UNIMPLEMENTED

typedef int DiscoveryRef;
typedef int TextRecordRef;

//************************************************************************************************
// TextRecord stub implementation
//************************************************************************************************

struct TextRecord: ITextRecord
{
	// ITextRecord
	int getCount () const { return 0; }
	bool getItemAt (CString64& key, CString64& value, int index) const { return false; }
	bool getValue (CString64& value, CStringPtr key) const { return false; }
	bool getIntValue (int64& value, CStringPtr key) const { return false; }
};

//************************************************************************************************
// TextRecordBuilder stub implementation
//************************************************************************************************

class TextRecordBuilder: public ITextRecordBuilder<TextRecord>
{
public:
	void setValue (CStringPtr key, CStringPtr value) {}
	void setIntValue (CStringPtr key, int64 value) {}
	TextRecord getTextRecord () const { return TextRecord (); }
};

//************************************************************************************************
// ServiceTargetDescriptor stub
//************************************************************************************************

typedef ServiceTargetDescriptorBase<TextRecord> ServiceTargetDescriptor;

//************************************************************************************************
// BrowseReplyHandler stub
//************************************************************************************************

typedef BrowseReplyHandlerBase<TextRecord, DiscoveryRef> BrowseReplyHandler;

//************************************************************************************************
// RegisterReplyHandler stub
//************************************************************************************************

typedef RegisterReplyHandlerBase<DiscoveryRef> RegisterReplyHandler;

//************************************************************************************************
// DiscoveryHandler stub implementation
//************************************************************************************************

inline bool DiscoveryHandler::isInstalled () 
{ return false; }

template<> inline bool DiscoveryHandler::registerBrowser (DiscoveryRef& sdRef, const TypeDescriptor& descriptor, BrowseReplyHandler* replyHandler) 
{ return false; }

template<> inline bool DiscoveryHandler::registerService (DiscoveryRef& sdRef, const ServiceTargetDescriptor& descriptor, RegisterReplyHandler* replyHandler) 
{ return false; }

template<> inline void DiscoveryHandler::unregisterReference (DiscoveryRef sdRef) 
{}

template<> inline bool DiscoveryHandler::processResults (const DiscoveryRef sdRefList[], int count) 
{ return false; }

#endif

} // namespace Platform
} // namespace Core

#endif // _coreplatformdiscovery_h
