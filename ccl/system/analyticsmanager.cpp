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
// Filename    : ccl/system/analyticsmanager.cpp
// Description : Analytics Manager
//
//************************************************************************************************

#include "ccl/system/analyticsmanager.h"

#include "ccl/public/storage/iattributelist.h"
#include "ccl/public/systemservices.h"

using namespace CCL;

//////////////////////////////////////////////////////////////////////////////////////////////////
// System Services API
//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_EXPORT IAnalyticsManager& CCL_API System::CCL_ISOLATED (GetAnalyticsManager) ()
{
	return AnalyticsManager::instance ();
}

//************************************************************************************************
// AnalyticsManager
//************************************************************************************************

DEFINE_SINGLETON (AnalyticsManager)

//////////////////////////////////////////////////////////////////////////////////////////////////

AnalyticsManager::~AnalyticsManager ()
{
	ASSERT (eventSinks.isEmpty ())
	ASSERT (!eventAllocator)
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API AnalyticsManager::createInstance (UIDRef cid, UIDRef iid, void** obj)
{
	if(eventAllocator)
		return eventAllocator->createInstance (cid, iid, obj);
	
	*obj = nullptr;
	return kResultClassNotFound;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API AnalyticsManager::setEventAllocator (IClassAllocator* allocator)
{
	ASSERT (System::IsInMainThread ())
	if(!System::IsInMainThread ())
		return kResultWrongThread;

	eventAllocator = allocator;
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API AnalyticsManager::addEventSink (IAnalyticsEventSink* eventSink)
{
	if(!eventSink)
		return kResultInvalidPointer;

	ASSERT (System::IsInMainThread ())
	if(!System::IsInMainThread ())
		return kResultWrongThread;

	eventSinks.add (eventSink);
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API AnalyticsManager::removeEventSink (IAnalyticsEventSink* eventSink)
{
	if(!eventSink)
		return kResultInvalidPointer;

	ASSERT (System::IsInMainThread ())
	if(!System::IsInMainThread ())
		return kResultWrongThread;

	if(!eventSinks.remove (eventSink))
		return kResultInvalidArgument;

	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API AnalyticsManager::isTrackingActive () const
{
	return eventAllocator != nullptr && !eventSinks.isEmpty ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API AnalyticsManager::addEvent (StringID eventId, const IAttributeList* data)
{
	ASSERT (System::IsInMainThread ())
	if(!System::IsInMainThread ())
		return kResultWrongThread;

	if(!eventAllocator) // no error, assume analytics isn't being used
		return kResultFalse;
	
	if(!eventSinks.isEmpty ())
	{
		AutoPtr<IAnalyticsEvent> e;
		eventAllocator->createInstance (ClassID::AnalyticsEvent, ccl_iid<IAnalyticsEvent> (), e.as_ppv ());
		ASSERT (e)
		if(!e)
			return kResultOutOfMemory;

		e->setID (eventId);
		if(data)
			e->setData (*data);

		for(auto sink : eventSinks)
			sink->addEvent (*e);
	}
	return kResultOk;
}
