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
// Filename    : ccl/extras/analytics/analyticsevent.cpp
// Description : Analytics Event
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "ccl/extras/analytics/analyticsevent.h"

#include "ccl/base/storage/storage.h"

#include "ccl/public/base/datetime.h"
#include "ccl/public/system/formatter.h"
#include "ccl/public/system/isysteminfo.h"
#include "ccl/public/system/idiagnosticstore.h"
#include "ccl/public/systemservices.h"

#define DEBUG_IMPATIENTLY (0 && DEBUG) // much shorter evaluation periods

using namespace CCL;

//************************************************************************************************
// AnalyticsEvent
//************************************************************************************************

DEFINE_CLASS_HIDDEN (AnalyticsEvent, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

AnalyticsEvent::AnalyticsEvent (StringID id)
: id (id)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

AnalyticsEvent::AnalyticsEvent (const IAnalyticsEvent& other)
{
	setID (other.getID ());
	setData (other.getData ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Attributes& AnalyticsEvent::getMutableData ()
{
	return data;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int64 AnalyticsEvent::getTimestamp () const
{
	return data.getInt64 (AnalyticsID::kTimestamp);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int64 AnalyticsEvent::getLastTimestamp () const
{
	return data.getInt64 (AnalyticsID::kLastTimestamp);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringID CCL_API AnalyticsEvent::getID () const
{
	return id;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API AnalyticsEvent::setID (StringID _id)
{
	id = _id;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const IAttributeList& CCL_API AnalyticsEvent::getData () const
{
	return data;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API AnalyticsEvent::setData (const IAttributeList& _data)
{
	data.copyFrom (_data);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool AnalyticsEvent::load (const Storage& storage)
{
	auto& a = storage.getAttributes ();
	id = a.getCString ("eventId");
	data.load (storage);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool AnalyticsEvent::save (const Storage& storage) const
{
	auto& a = storage.getAttributes ();
	a.set ("eventId", id);
	data.save (storage);
	return true;
}

//************************************************************************************************
// AnalyticsEventFactory
//************************************************************************************************

DEFINE_CLASS_HIDDEN (AnalyticsEventFactory, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API AnalyticsEventFactory::createInstance (UIDRef cid, UIDRef iid, void** obj)
{
	if(cid == ClassID::AnalyticsEvent)
	{
		AutoPtr<IAnalyticsEvent> instance = NEW AnalyticsEvent;
		return instance->queryInterface (iid, obj);
	}
	else if(cid == ClassID::Attributes)
	{
		AutoPtr<IAttributeList> instance = NEW Attributes;
		return instance->queryInterface (iid, obj);
	}
	else
	{
		*obj = nullptr;
		return kResultClassNotFound;
	}	
}

//************************************************************************************************
// AnalyticsEventCache
//************************************************************************************************

DEFINE_CLASS_HIDDEN (AnalyticsEventCache, JsonStorableObject)

//////////////////////////////////////////////////////////////////////////////////////////////////

AnalyticsEventCache::AnalyticsEventCache ()
{
	events.objectCleanup (true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int AnalyticsEventCache::getCount () const
{
	return events.count ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const ObjectArray& AnalyticsEventCache::getEvents () const
{
	return events; 
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AnalyticsEventCache::addOwned (AnalyticsEvent* e)
{
	ASSERT (e)
	events.add (e);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AnalyticsEventCache::addShared (IAnalyticsEvent* e)
{
	ASSERT (e)
	if(auto e2 = unknown_cast<AnalyticsEvent> (e))
		events.add (return_shared (e2));
	else
		events.add (NEW AnalyticsEvent (*e));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AnalyticsEventCache::addAllFrom (const AnalyticsEventCache& cache)
{
	for(auto event : iterate_as<AnalyticsEvent> (cache.getEvents ()))
		addShared (event);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AnalyticsEventCache::removeAll ()
{
	events.removeAll ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool AnalyticsEventCache::load (const Storage& storage)
{
	auto& a = storage.getAttributes ();
	a.unqueueAndCreate (events, "events", ccl_typeid<AnalyticsEvent> (), storage.getArchive ());
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool AnalyticsEventCache::save (const Storage& storage) const
{
	auto& a = storage.getAttributes ();
	a.convertAndQueue ("events", events, storage.getArchive ());
	return true;
}

//************************************************************************************************
// AnalyticsTrackingPlan::PassThroughEventFilter
//************************************************************************************************

class AnalyticsTrackingPlan::PassThroughEventFilter: public Unknown,
													  public AbstractAnalyticsEventFilter
{
public:
	PassThroughEventFilter (StringID eventId)
	: eventId (eventId)
	{}

	PROPERTY_MUTABLE_CSTRING (eventId, EventId)

	// IAnalyticsEventFilter
	IAnalyticsEvent* CCL_API process (IAnalyticsEvent& e) override
	{
		if(e.getID () == getEventId ())
			return return_shared (&e);

		return nullptr;
	}

	CLASS_INTERFACE (IAnalyticsEventFilter, Unknown)
};

//************************************************************************************************
// AnalyticsTrackingPlan
//************************************************************************************************

StringID AnalyticsTrackingPlan::kContextNextEvaluationTime = CSTR ("analytics/nextEval");
StringID AnalyticsTrackingPlan::kContextLastEventTimestamp = CSTR ("analytics/lastEvent");
StringID AnalyticsTrackingPlan::kContextLastBatchTimestamp = CSTR ("analytics/lastBatch");

//////////////////////////////////////////////////////////////////////////////////////////////////

AnalyticsTrackingPlan::AnalyticsTrackingPlan ()
{
	batchGroups.objectCleanup (true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API AnalyticsTrackingPlan::addFilter (StringID eventId, IAnalyticsEventFilter* filter, int64 evaluationPeriod)
{
	addFilterInternal (eventId, filter, evaluationPeriod);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API AnalyticsTrackingPlan::addFilter (StringID inEventId, StringID outEventId, IAnalyticsEventFilter* filter, int64 evaluationPeriod)
{
	addFilterInternal (inEventId, filter, evaluationPeriod);
	addFilterInternal (outEventId, return_shared (filter), -1);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AnalyticsTrackingPlan::addFilter (const Vector<CString>& inEventIds, StringID outEventId, IAnalyticsEventFilter* filter, int64 evaluationPeriod)
{
	for(auto& id : inEventIds)
	{
		addFilterInternal (id, return_shared (filter), evaluationPeriod);
		evaluationPeriod = -1; // evaluation needed only once for this filter (for first input event)
	}

	addFilterInternal (outEventId, filter, -1);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AnalyticsTrackingPlan::addFilterInternal (StringID eventId, IAnalyticsEventFilter* filter, int64 evaluationPeriod)
{
	#if DEBUG_IMPATIENTLY
	evaluationPeriod = evaluationPeriod * 5 / DateTime::kSecondsInDay; // 5 seconds instead of 1 day
	#endif

	filterEntries.addSorted (FilterEntry (eventId, filter, evaluationPeriod));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AnalyticsTrackingPlan::addPassThroughFilter (StringID eventId)
{
	addFilterInternal (eventId, NEW PassThroughEventFilter (eventId));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

AnalyticsTrackingPlan::FilterEntry* AnalyticsTrackingPlan::findFilterEntry (StringID eventId) const
{
	return filterEntries.search ({eventId});
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IAnalyticsEventFilter* AnalyticsTrackingPlan::findFilter (StringID eventId) const
{
	FilterEntry* entry = findFilterEntry (eventId);
	return entry ? entry->filter : nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringID AnalyticsTrackingPlan::getEvaluationEventId (StringID outEventId) const
{
	if(FilterEntry* outEntry = findFilterEntry (outEventId))
	{
		// try given event 
		if(outEntry->evaluationPeriod > 0)
			return outEventId;

		// find another entry for the same filter that has an evaluation period
		for(auto& entry : filterEntries)
			if(entry.filter == outEntry->filter && entry.eventId != outEventId && entry.evaluationPeriod > 0)
				return entry.eventId;
	}
	return CString::kEmpty;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AnalyticsTrackingPlan::addBatchGroup (const Vector<CString>& eventIds)
{
	auto* batch = NEW BatchGroup;

	for(auto& id : eventIds)
		batch->addEventID (id);

	batchGroups.add (batch);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

AnalyticsTrackingPlan::BatchGroup* AnalyticsTrackingPlan::findBatchGroup (StringID eventId) const
{
	return static_cast<BatchGroup*> (batchGroups.findIf ([&] (Object* obj)
		{ return static_cast<BatchGroup*> (obj)->containsEvent (eventId); }));
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int64 AnalyticsTrackingPlan::getNextEvaluationTime () const
{
	int64 next = NumericLimits::kMaxInt64;

	for(auto& entry : filterEntries)
		if(entry.nextEvaluationTime < next)
			next = entry.nextEvaluationTime;

	return next == NumericLimits::kMaxInt64 ? - 1 : next;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int64 AnalyticsTrackingPlan::getEvaluationPeriod () const
{
	int64 period = NumericLimits::kMaxInt64;

	for(auto& entry : filterEntries)
		if(entry.evaluationPeriod < period && entry.evaluationPeriod > 0)
			period = entry.evaluationPeriod;

	return period;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AnalyticsTrackingPlan::setNextEvaluationTime (StringID eventId, int64 unixTime)
{
	if(FilterEntry* entry = findFilterEntry (eventId))
		setNextEvaluationTime (*entry, unixTime);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AnalyticsTrackingPlan::setNextEvaluationTime (FilterEntry& entry, int64 unixTime)
{
	entry.nextEvaluationTime = unixTime;

	#if 0 && DEBUG_LOG
	DateTime time (UnixTime::toUTC (entry.nextEvaluationTime));
	System::GetSystem ().convertUTCToLocalTime (time, time);
	CCL_PRINTF ("setNextEvaluationTime (%s): %s\n", entry.eventId.str (), MutableCString (Format::DateTime::print (time)).str ());
	#endif
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int64 AnalyticsTrackingPlan::getRandomizedEvaluationPeriod (int64 period)
{
	int64 maxDelay = ccl_max<int64> (1, period / 100); // add max. 1 %
	int64 delay = rand () % maxDelay;
	return period + delay;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int64 AnalyticsTrackingPlan::getLastEventTimestamp (StringID eventId) const
{
	for(auto& entry : filterEntries)
		if(entry.eventId == eventId)
			return entry.lastEventTimestamp;

	return 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AnalyticsTrackingPlan::setLastEventTimestamp (StringID eventId, int64 timestamp)
{
	for(auto& entry : filterEntries)
		if(entry.eventId == eventId)
		{
			entry.lastEventTimestamp = timestamp;
			break;
		}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AnalyticsTrackingPlan::initializeLastTimeStamps (int64 unixTime, bool force)
{
	// initialize lastEventTimestamps to ensure a reasonable tracking interval for the first event
	for(auto& entry : filterEntries)
		if(force || entry.lastEventTimestamp == 0)
			entry.lastEventTimestamp = unixTime;

	for(auto* batch: iterate_as<BatchGroup> (batchGroups))
		if(force || batch->getLastTimeStamp ()  == 0)
			batch->setLastTimeStamp (unixTime);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AnalyticsTrackingPlan::onTimer (int64 unixTime)
{
	for(auto& entry : filterEntries)
		if(entry.nextEvaluationTime <= unixTime)
		{
			entry.filter->evaluateData ();
			setNextEvaluationTime (entry, unixTime + getRandomizedEvaluationPeriod (entry.evaluationPeriod));
		}
		else
			entry.filter->onIdle ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AnalyticsTrackingPlan::terminate ()
{
	for(auto& entry : filterEntries)
		entry.filter->terminate ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AnalyticsTrackingPlan::storeSettings ()
{
	// store nextEvaluationTime and lastEventTimestamp in DiagnosticStore
	DiagnosticStoreAccessor diagnostics (System::GetDiagnosticStore ());

	for(auto& entry : filterEntries)
	{
		if(entry.evaluationPeriod > 0)
		{
			diagnostics.setPlainValue (kContextNextEvaluationTime, entry.eventId, entry.nextEvaluationTime);
			CCL_PRINTF ("AnalyticsTrackingPlan: store (%s) nextEvalTime %s \n", entry.eventId.str (), MutableCString (Format::DateTime::print (UnixTime::toUTC (entry.nextEvaluationTime))).str ())
		}

		if(entry.lastEventTimestamp > 0)
			diagnostics.setPlainValue (kContextLastEventTimestamp, entry.eventId, entry.lastEventTimestamp);
	}

	for(auto* batch: iterate_as<BatchGroup> (batchGroups))
		batch->storeSettings ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AnalyticsTrackingPlan::restoreSettings ()
{
	DiagnosticStoreAccessor diagnostics (System::GetDiagnosticStore ());
	int64 now = UnixTime::getTime ();

	for(auto& entry : filterEntries)
	{
		if(entry.evaluationPeriod > 0)
		{
			Variant nextEvalTime;
			if(diagnostics.getPlainValue (nextEvalTime, kContextNextEvaluationTime, entry.eventId) && !DEBUG_IMPATIENTLY)
				setNextEvaluationTime (entry, nextEvalTime);
			else
			{
				// not stored: period starts now, but shorten it to get the first report earlier
				int64 period = entry.evaluationPeriod;
				if(period >= 4 * DateTime::kSecondsInDay)
					period /= 4;
				else if(period >= 2 * DateTime::kSecondsInDay)
					period /= 2;

				setNextEvaluationTime (entry, now + getRandomizedEvaluationPeriod (period));
			}
		}

		Variant lastEventTimestamp;
		if(diagnostics.getPlainValue (lastEventTimestamp, kContextLastEventTimestamp, entry.eventId))
			entry.lastEventTimestamp = lastEventTimestamp;
	}

	for(auto* batch: iterate_as<BatchGroup> (batchGroups))
		batch->restoreSettings ();
}

//************************************************************************************************
// AnalyticsTrackingPlan::BatchGroup
//************************************************************************************************

AnalyticsTrackingPlan::BatchGroup::BatchGroup ()
: currentTimeStamp (0),
  lastTimeStamp (0)
{
	eventEntries.objectCleanup (true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AnalyticsTrackingPlan::BatchGroup::addEventID (StringID eventId)
{
	eventEntries.add (NEW EventEntry (eventId));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool AnalyticsTrackingPlan::BatchGroup::containsEvent (StringID eventId) const
{
	return findEventEntry (eventId) != nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

AnalyticsTrackingPlan::BatchGroup::EventEntry* AnalyticsTrackingPlan::BatchGroup::findEventEntry (StringID eventId) const
{
	return static_cast<EventEntry*> (eventEntries.findIf ([&] (Object* obj)
		{ return static_cast<EventEntry*> (obj)->getEventID () == eventId; }));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

String AnalyticsTrackingPlan::BatchGroup::getBatchID (StringID eventId)
{
	EventEntry* eventEntry = findEventEntry (eventId);
	ASSERT (eventEntry)
	if(eventEntry)
	{
		// it's time to start a new batch when the current batchID was already used for this eventID
		if(getCurrentBatchID ().isEmpty () || eventEntry->getLastBatchID () == getCurrentBatchID ())
			prepareNextBatch ();

		eventEntry->setLastBatchID (getCurrentBatchID ());
		return getCurrentBatchID ();
	}
	return String::kEmpty;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AnalyticsTrackingPlan::BatchGroup::prepareNextBatch ()
{
	currentBatchId = generateBatchID ();
	lastTimeStamp = currentTimeStamp;
	currentTimeStamp = UnixTime::getTime ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

String AnalyticsTrackingPlan::BatchGroup::generateBatchID () const
{
	UID uid;
	uid.generate ();

	String id;
	uid.toString (id, UID::kCompact);
	return id.toLowercase ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CStringPtr AnalyticsTrackingPlan::BatchGroup::getStorageID () const
{
	auto* firstEventEntry = static_cast<EventEntry*> (eventEntries.first ());
	return firstEventEntry ? firstEventEntry->getEventID () : nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AnalyticsTrackingPlan::BatchGroup::storeSettings ()
{
	CStringPtr storageId = getStorageID ();

	if(storageId && lastTimeStamp > 0)
	{
		// store lastTimeStamp in DiagnosticStore
		DiagnosticStoreAccessor diagnostics (System::GetDiagnosticStore ());
		diagnostics.setPlainValue (kContextLastBatchTimestamp, storageId, lastTimeStamp);
		CCL_PRINTF ("BatchGroup: store (%" FORMAT_INT64 "d) lastTimeStamp %s \n", lastTimeStamp, MutableCString (Format::DateTime::print (UnixTime::toUTC (lastTimeStamp))).str ())
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AnalyticsTrackingPlan::BatchGroup::restoreSettings ()
{
	ASSERT (currentBatchId.isEmpty ())

	CStringPtr storageId = getStorageID ();
	if(storageId)
	{
		DiagnosticStoreAccessor diagnostics (System::GetDiagnosticStore ());

		Variant var;
		if(diagnostics.getPlainValue (var, kContextLastBatchTimestamp, storageId))
			currentTimeStamp = var; // will become lastTimeStamp on prepareNextBatch
	}
}
