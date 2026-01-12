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
// Filename    : ccl/network/netdiscovery.cpp
// Description : DNS Service Discovery
//
//************************************************************************************************

#include "ccl/network/netdiscovery.h"

#include "ccl/base/message.h"

#include "ccl/public/system/ithreadpool.h"

namespace CCL {
namespace Net {

//************************************************************************************************
// DiscoveryHandler::Processor
//************************************************************************************************

class DiscoveryHandler::Processor: public Object,
								   public Threading::IPeriodicItem
{
public:
	Processor (DiscoveryHandler& handler);

	// IPeriodicItem
	int64 CCL_API getExecutionTime () const override;
	void CCL_API execute (int64 now) override;

	CLASS_INTERFACE (IPeriodicItem, Object)

protected:
	DiscoveryHandler& handler;
	int64 nextExecutionTime;
};

} // namespace Net
} // namespace CCL

using namespace CCL;
using namespace Net;

//************************************************************************************************
// ServiceBrowser
//************************************************************************************************

ServiceBrowser::ServiceBrowser (IObserver* observer)
: observer (observer),
  sdRef (nullptr)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

ServiceBrowser::ServiceBrowser (Core::Sockets::Discovery::DiscoveryRef sdRef)
: observer (nullptr),
  sdRef (sdRef)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ServiceBrowser::equals (const Object& obj) const
{
	const ServiceBrowser& other = (const ServiceBrowser&)obj;
	if(other.observer)
		return observer == other.observer;
	else
	{
		ASSERT (sdRef != nullptr)
		return sdRef == other.sdRef;
	}
}

//************************************************************************************************
// ServiceDescriptor
//************************************************************************************************

void ServiceDescriptor::setInfo (const ServiceBasicInformation& info)
{
	this->info = info;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ServiceDescriptor::setTextRecord (const ICStringDictionary& textRecord)
{
	this->textRecord.copyFrom (textRecord);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ServiceDescriptor::assign (const Core::Sockets::Discovery::ServiceDescriptor& d)
{
	info = ServiceBasicInformation ();
	textRecord.removeAll ();

	info.name.appendCString (Text::kUTF8, d.serviceName);
	info.type = d.type;
	info.protocol = d.protocol;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ServiceDescriptor::assign (const Core::Sockets::Discovery::ServiceTargetDescriptor& td)
{
	assign (static_cast<const Core::Sockets::Discovery::ServiceDescriptor&> (td));

	info.hostname.appendCString (Text::kUTF8, td.hostname);
	info.port = td.port;

	if(td.textRecord)
		for(int i = 0; i < td.textRecord->getCount (); i++)
		{
			Core::CString64 key, value;
			if(td.textRecord->getItemAt (key, value, i))
				textRecord.appendEntry (CString (key), CString (value));
		}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const ServiceBasicInformation& CCL_API ServiceDescriptor::getBasicInformation () const
{
	return info;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const ICStringDictionary& CCL_API ServiceDescriptor::getTextRecord () const
{
	return textRecord;
}

//************************************************************************************************
// ServiceInstance
//************************************************************************************************

DEFINE_CLASS_HIDDEN (ServiceInstance, ServiceDescriptor)

//////////////////////////////////////////////////////////////////////////////////////////////////

ServiceInstance::ServiceInstance ()
: observer (nullptr),
  sdRef (nullptr)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

ServiceInstance::ServiceInstance (Core::Sockets::Discovery::DiscoveryRef sdRef)
: observer (nullptr),
  sdRef (sdRef)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ServiceInstance::update (const Core::Sockets::Discovery::ServiceDescriptor& d)
{
	String serviceName;
	serviceName.appendCString (Text::kUTF8, d.serviceName);
	info.name = serviceName;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ServiceInstance::equals (const Object& obj) const
{
	const ServiceInstance& other = (const ServiceInstance&)obj;
	ASSERT (sdRef != nullptr)
	return sdRef == other.sdRef;
}

//************************************************************************************************
// DiscoveryHandler
//************************************************************************************************

DiscoveryHandler::DiscoveryHandler ()
: processor (nullptr)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

DiscoveryHandler::~DiscoveryHandler ()
{
	ASSERT (browsers.isEmpty ())
	ASSERT (services.isEmpty ())
	ASSERT (references.isEmpty ())
	ASSERT (processor == nullptr)
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API DiscoveryHandler::isInstalled ()
{
	return Core::Sockets::Discovery::DiscoveryHandler::isInstalled ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API DiscoveryHandler::startBrowsing (IObserver* observer, StringID type, ProtocolType protocol)
{
	ASSERT (System::IsInMainThread ())
	if(!System::IsInMainThread ())
		return kResultWrongThread;

	ASSERT (isInstalled ())
	if(!isInstalled ())
		return kResultUnexpected;

	ASSERT (observer != nullptr)
	if(!observer)
		return kResultInvalidPointer;

	{
		Threading::ScopedLock scopedLock (processLock);

		Core::Sockets::Discovery::TypeDescriptor descriptor;
		descriptor.type = type.str ();
		descriptor.protocol = protocol;

		Core::Sockets::Discovery::DiscoveryRef sdRef = nullptr;
		if(!Core::Sockets::Discovery::DiscoveryHandler::registerBrowser (sdRef, descriptor, this))
			return kResultFailed;

		ServiceBrowser* browser = NEW ServiceBrowser (observer);
		browser->setReference (sdRef);
		browsers.add (browser);
		references.add (sdRef);
	}

	checkProcessing ();
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API DiscoveryHandler::stopBrowsing (IObserver* observer)
{
	ASSERT (System::IsInMainThread ())
	if(!System::IsInMainThread ())
		return kResultWrongThread;

	ASSERT (isInstalled ())
	if(!isInstalled ())
		return kResultUnexpected;

	ServiceBrowser* browser = (ServiceBrowser*)browsers.findEqual (ServiceBrowser (observer));
	ASSERT (browser != nullptr)
	if(browser == nullptr)
		return kResultInvalidArgument;

	{
		Threading::ScopedLock scopedLock (processLock);

		Core::Sockets::Discovery::DiscoveryHandler::unregisterReference (browser->getReference ());
		references.remove (browser->getReference ());
		browsers.remove (browser);
		browser->release ();
	}

	checkProcessing ();
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IServiceDescriptor* CCL_API DiscoveryHandler::createService (const ServiceBasicInformation& info, const ICStringDictionary* textRecord)
{
	ServiceInstance* service = NEW ServiceInstance;
	service->setInfo (info);
	if(textRecord)
		service->setTextRecord (*textRecord);
	return service;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API DiscoveryHandler::registerService (IServiceDescriptor* _service, IObserver* observer)
{
	ASSERT (System::IsInMainThread ())
	if(!System::IsInMainThread ())
		return kResultWrongThread;

	ASSERT (isInstalled ())
	if(!isInstalled ())
		return kResultUnexpected;

	ServiceInstance* service = unknown_cast<ServiceInstance> (_service);
	if(!service)
		return kResultInvalidArgument;

	const ServiceBasicInformation& info = service->getBasicInformation ();
	Core::Sockets::Discovery::ServiceTargetDescriptor td;
	td.type = info.type.str ();
	td.protocol = info.protocol;
	MutableCString serviceName (info.name, Text::kUTF8);
	td.serviceName = serviceName.str ();
	MutableCString hostname (info.hostname, Text::kUTF8);
	td.hostname = hostname.str ();
	td.port = info.port;

	const ICStringDictionary& textRecord = service->getTextRecord ();
	Core::Sockets::Discovery::TextRecordBuilder builder;
	bool hasText = textRecord.countEntries () > 0;
	if(hasText) for(int i = 0; i < textRecord.countEntries (); i++)
		builder.setValue (textRecord.getKeyAt (i), textRecord.getValueAt (i));

	Core::Sockets::Discovery::TextRecord tr = builder.getTextRecord ();
	td.textRecord = hasText ? &tr : nullptr;

	{
		Threading::ScopedLock scopedLock (processLock);

		Core::Sockets::Discovery::DiscoveryRef sdRef = nullptr;
		if(!Core::Sockets::Discovery::DiscoveryHandler::registerService (sdRef, td, this))
			return kResultFailed;

		service->setObserver (observer);
		service->setReference (sdRef);
		services.add (service);
		references.add (sdRef);
	}

	checkProcessing ();
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API DiscoveryHandler::unregisterService (IServiceDescriptor* _service)
{
	ASSERT (System::IsInMainThread ())
	if(!System::IsInMainThread ())
		return kResultWrongThread;

	ASSERT (isInstalled ())
	if(!isInstalled ())
		return kResultUnexpected;

	ServiceInstance* service = unknown_cast<ServiceInstance> (_service);
	if(!service)
		return kResultInvalidArgument;

	{
		Threading::ScopedLock scopedLock (processLock);

		Core::Sockets::Discovery::DiscoveryHandler::unregisterReference (service->getReference ());
		references.remove (service->getReference ());
		services.remove (service);
		service->release ();
	}

	checkProcessing ();
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DiscoveryHandler::checkProcessing ()
{
	if(!references.isEmpty ())
	{
		if(processor == nullptr)
		{
			processor = NEW Processor (*this);
			System::GetThreadPool ().addPeriodic (processor);
		}
	}
	else
	{
		if(processor != nullptr)
		{
			System::GetThreadPool ().removePeriodic (processor);
			safe_release (processor);
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DiscoveryHandler::process ()
{
	Threading::ScopedLock scopedLock (processLock);

	Core::Sockets::Discovery::DiscoveryHandler::processResults (references, references.count ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DiscoveryHandler::onServiceResolved (Core::Sockets::Discovery::DiscoveryRef sdRef, const Core::Sockets::Discovery::ServiceTargetDescriptor& descriptor)
{
	ServiceBrowser* browser = (ServiceBrowser*)browsers.findEqual (ServiceBrowser (sdRef));
	ASSERT (browser != nullptr)
	if(browser)
	{
		AutoPtr<ServiceDescriptor> d = NEW ServiceDescriptor;
		d->assign (descriptor);
		(NEW Message (kServiceResolved, d->asUnknown ()))->post (browser->getObserver ());
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DiscoveryHandler::onServiceRemoved (Core::Sockets::Discovery::DiscoveryRef sdRef, const Core::Sockets::Discovery::ServiceDescriptor& descriptor)
{
	ServiceBrowser* browser = (ServiceBrowser*)browsers.findEqual (ServiceBrowser (sdRef));
	ASSERT (browser != nullptr)
	if(browser)
	{
		AutoPtr<ServiceDescriptor> d = NEW ServiceDescriptor;
		d->assign (descriptor);
		(NEW Message (kServiceRemoved, d->asUnknown ()))->post (browser->getObserver ());
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DiscoveryHandler::onServiceRegistered (Core::Sockets::Discovery::DiscoveryRef sdRef, const Core::Sockets::Discovery::ServiceDescriptor& descriptor)
{
	ServiceInstance* service = (ServiceInstance*)services.findEqual (ServiceInstance (sdRef));
	ASSERT (service != nullptr)
	if(service)
	{
		service->update (descriptor);

		if(service->getObserver ())
			(NEW Message (kServiceRegistered, service->asUnknown ()))->post (service->getObserver ());
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DiscoveryHandler::onServiceRegistrationFailed (Core::Sockets::Discovery::DiscoveryRef sdRef)
{
	ServiceInstance* service = (ServiceInstance*)services.findEqual (ServiceInstance (sdRef));
	ASSERT (service != nullptr)
	if(service)
	{
		if(service->getObserver ())
			(NEW Message (kServiceRegistrationFailed, service->asUnknown ()))->post (service->getObserver ());
	}
}

//************************************************************************************************
// DiscoveryHandler::Processor
//************************************************************************************************

DiscoveryHandler::Processor::Processor (DiscoveryHandler& handler)
: handler (handler),
  nextExecutionTime (0)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

int64 CCL_API DiscoveryHandler::Processor::getExecutionTime () const
{
	return nextExecutionTime;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API DiscoveryHandler::Processor::execute (int64 now)
{
	handler.process ();
	nextExecutionTime = now + 1000; // 1 second
}
