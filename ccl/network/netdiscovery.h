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
// Filename    : ccl/network/netdiscovery.h
// Description : DNS Service Discovery
//
//************************************************************************************************

#ifndef _ccl_netdiscovery_h
#define _ccl_netdiscovery_h

#include "core/network/corediscovery.h"

#include "ccl/base/collections/objectarray.h"
#include "ccl/base/collections/stringdictionary.h"

#include "ccl/public/system/threadsync.h"
#include "ccl/public/network/inetdiscovery.h"

namespace CCL {
namespace Net {

//************************************************************************************************
// ServiceBrowser
//************************************************************************************************

class ServiceBrowser: public Object
{
public:
	ServiceBrowser (IObserver* observer);
	ServiceBrowser (Core::Sockets::Discovery::DiscoveryRef sdRef);

	PROPERTY_POINTER (IObserver, observer, Observer)
	PROPERTY_VARIABLE (Core::Sockets::Discovery::DiscoveryRef, sdRef, Reference)

	// Object
	bool equals (const Object& obj) const override;
};

//************************************************************************************************
// ServiceDescriptor
//************************************************************************************************

class ServiceDescriptor: public Object,
						 public IServiceDescriptor
{
public:
	void setInfo (const ServiceBasicInformation& info);
	void setTextRecord (const ICStringDictionary& textRecord);

	void assign (const Core::Sockets::Discovery::ServiceDescriptor& d);
	void assign (const Core::Sockets::Discovery::ServiceTargetDescriptor& td);

	// IServiceDescriptor
	const ServiceBasicInformation& CCL_API getBasicInformation () const override;
	const ICStringDictionary& CCL_API getTextRecord () const override;

	CLASS_INTERFACE (IServiceDescriptor, Object)

protected:
	ServiceBasicInformation info;
	CStringDictionary textRecord;
};

//************************************************************************************************
// ServiceInstance
//************************************************************************************************

class ServiceInstance: public ServiceDescriptor
{
public:
	DECLARE_CLASS (ServiceInstance, ServiceDescriptor)

	ServiceInstance ();
	ServiceInstance (Core::Sockets::Discovery::DiscoveryRef sdRef);

	void update (const Core::Sockets::Discovery::ServiceDescriptor& d);

	PROPERTY_POINTER (IObserver, observer, Observer)
	PROPERTY_VARIABLE (Core::Sockets::Discovery::DiscoveryRef, sdRef, Reference)

	// Object
	bool equals (const Object& obj) const override;
};

//************************************************************************************************
// DiscoveryHandler
//************************************************************************************************

class DiscoveryHandler: public Object,
						public IDiscoveryHandler,
						public Core::Sockets::Discovery::BrowseReplyHandler,
						public Core::Sockets::Discovery::RegisterReplyHandler
{
public:
	DiscoveryHandler ();
	~DiscoveryHandler ();

	void process ();

	// IDiscoveryHandler
	tbool CCL_API isInstalled () override;
	tresult CCL_API startBrowsing (IObserver* observer, StringID type, ProtocolType protocol) override;
	tresult CCL_API stopBrowsing (IObserver* observer) override;
	IServiceDescriptor* CCL_API createService (const ServiceBasicInformation& info, const ICStringDictionary* textRecord = nullptr) override;
	tresult CCL_API registerService (IServiceDescriptor* service, IObserver* observer) override;
	tresult CCL_API unregisterService (IServiceDescriptor* service) override;

	CLASS_INTERFACE (IDiscoveryHandler, Object)

protected:
	class Processor;

	Processor* processor;
	ObjectArray browsers;
	ObjectArray services;
	Threading::CriticalSection processLock;
	Vector<Core::Sockets::Discovery::DiscoveryRef> references;

	void checkProcessing ();

	// BrowseReplyHandler
	void onServiceResolved (Core::Sockets::Discovery::DiscoveryRef sdRef, const Core::Sockets::Discovery::ServiceTargetDescriptor& descriptor) override;
	void onServiceRemoved (Core::Sockets::Discovery::DiscoveryRef sdRef, const Core::Sockets::Discovery::ServiceDescriptor& descriptor) override;

	// RegisterReplyHandler
	void onServiceRegistered (Core::Sockets::Discovery::DiscoveryRef sdRef, const Core::Sockets::Discovery::ServiceDescriptor& descriptor) override;
	void onServiceRegistrationFailed (Core::Sockets::Discovery::DiscoveryRef sdRef) override;
};

} // namespace Net
} // namespace CCL

#endif // _ccl_netdiscovery_h
