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
// Filename    : core/network/corediscovery.h
// Description : DNS Service Discovery
//
//************************************************************************************************

#ifndef _corediscovery_h
#define _corediscovery_h

#include "core/platform/corefeatures.h"

#if CORE_DISCOVERY_IMPLEMENTATION == CORE_PLATFORM_IMPLEMENTATION
	#include CORE_PLATFORM_IMPLEMENTATION_HEADER (corediscovery)
#elif CORE_DISCOVERY_IMPLEMENTATION == CORE_EXTERNAL_PLATFORM_IMPLEMENTATION
	#include CORE_EXTERNAL_PLATFORM_IMPLEMENTATION_HEADER (corediscovery)
#elif CORE_DISCOVERY_IMPLEMENTATION == CORE_FEATURE_IMPLEMENTATION_POSIX
	#include "core/platform/shared/posix/corediscovery.posix.h"
#elif CORE_DISCOVERY_IMPLEMENTATION == CORE_FEATURE_UNIMPLEMENTED
	#include "core/platform/shared/coreplatformdiscovery.h"
#endif

namespace Core {
namespace Sockets {
namespace Discovery {

typedef Platform::DiscoveryRef DiscoveryRef;
typedef Platform::TypeDescriptor TypeDescriptor;
typedef Platform::ServiceDescriptor ServiceDescriptor;
typedef Platform::ServiceTargetDescriptor ServiceTargetDescriptor;

//************************************************************************************************
// TextRecord
//************************************************************************************************

struct TextRecord: Platform::TextRecord
{
	TextRecord (const Platform::TextRecord& other);

	// Platform::TextRecord
	int getCount () const override;
	bool getItemAt (CString64& key, CString64& value, int index) const override;
	bool getValue (CString64& value, CStringPtr key) const override;
	bool getIntValue (int64& value, CStringPtr key) const override;
};

//************************************************************************************************
// TextRecordBuilder
//************************************************************************************************

class TextRecordBuilder
{
public:
	void setValue (CStringPtr key, CStringPtr value);
	void setIntValue (CStringPtr key, int64 value);

	TextRecord getTextRecord () const;

protected:
	Platform::TextRecordBuilder platformBuilder;
};

//************************************************************************************************
// BrowseReplyHandler
//************************************************************************************************

struct BrowseReplyHandler: Platform::BrowseReplyHandler
{
	// Platform::BrowseReplyHandler
	virtual void onServiceResolved (DiscoveryRef sdRef, const ServiceTargetDescriptor& descriptor) override = 0;
	virtual void onServiceRemoved (DiscoveryRef sdRef, const ServiceDescriptor& descriptor) override = 0;
};

//************************************************************************************************
// RegisterReplyHandler
//************************************************************************************************

struct RegisterReplyHandler: Platform::RegisterReplyHandler
{
	// Platform::RegisterReplyHandler
	/** Note: Service name might change upon registration when a name conflict occurred! */
	virtual void onServiceRegistered (DiscoveryRef sdRef, const ServiceDescriptor& descriptor) override = 0;
	virtual void onServiceRegistrationFailed (DiscoveryRef sdRef) override = 0;
};

//************************************************************************************************
// DiscoveryHandler
//************************************************************************************************

class DiscoveryHandler
{
public:
	/** Check if DNSSD is installed. */
	static bool isInstalled ();

	/** Register browser for given service type. */
	static bool registerBrowser (DiscoveryRef& sdRef, const TypeDescriptor& descriptor, BrowseReplyHandler* replyHandler);

	/** Register service. */
	static bool registerService (DiscoveryRef& sdRef, const ServiceTargetDescriptor& descriptor, RegisterReplyHandler* replyHandler);

	/** Unregister given reference (browser or service). */
	static void unregisterReference (DiscoveryRef sdRef);

	/** Process results for given reference (browser or service). */
	static bool processResults (const DiscoveryRef sdRefList[], int count);
};

//************************************************************************************************
// TextRecord implementation
//************************************************************************************************

inline TextRecord::TextRecord (const Platform::TextRecord& other)
: Platform::TextRecord (other)
{}

inline int TextRecord::getCount () const
{ return Platform::TextRecord::getCount (); }
	
inline bool TextRecord::getItemAt (CString64& key, CString64& value, int index) const
{ return Platform::TextRecord::getItemAt (key, value, index); }
	
inline bool TextRecord::getValue (CString64& value, CStringPtr key) const
{ return Platform::TextRecord::getValue (value, key); }
	
inline bool TextRecord::getIntValue (int64& value, CStringPtr key) const
{ return Platform::TextRecord::getIntValue (value, key); }

//************************************************************************************************
// TextRecordBuilder implementation
//************************************************************************************************

inline void TextRecordBuilder::setValue (CStringPtr key, CStringPtr value)
{ platformBuilder.setValue (key, value); }

inline void TextRecordBuilder::setIntValue (CStringPtr key, int64 value)
{ platformBuilder.setIntValue (key, value); }

inline TextRecord TextRecordBuilder::getTextRecord () const
{ return platformBuilder.getTextRecord (); }

//************************************************************************************************
// DiscoveryHandler implementation
//************************************************************************************************

inline bool DiscoveryHandler::isInstalled ()
{ return Platform::DiscoveryHandler::isInstalled (); }

inline bool DiscoveryHandler::registerBrowser (DiscoveryRef& sdRef, const TypeDescriptor& descriptor, BrowseReplyHandler* replyHandler)
{ return Platform::DiscoveryHandler::registerBrowser (sdRef, descriptor, replyHandler); }

inline bool DiscoveryHandler::registerService (DiscoveryRef& sdRef, const ServiceTargetDescriptor& descriptor, RegisterReplyHandler* replyHandler)
{ return Platform::DiscoveryHandler::registerService (sdRef, descriptor, replyHandler); }

inline void DiscoveryHandler::unregisterReference (DiscoveryRef sdRef)
{ Platform::DiscoveryHandler::unregisterReference (sdRef); }

inline bool DiscoveryHandler::processResults (const DiscoveryRef sdRefList[], int count)
{ return Platform::DiscoveryHandler::processResults (sdRefList, count); }

} // namespace Discovery
} // namespace Sockets
} // namespace Core

#endif // _corediscovery_h
