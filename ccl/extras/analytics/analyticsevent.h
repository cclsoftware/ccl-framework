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
// Filename    : ccl/extras/analytics/analyticsevent.h
// Description : Analytics Event
//
//************************************************************************************************

#ifndef _ccl_analyticsevent_h
#define _ccl_analyticsevent_h

#include "ccl/base/storage/attributes.h"
#include "ccl/base/storage/storableobject.h"

#include "ccl/public/system/ianalyticsmanager.h"

namespace CCL {

//************************************************************************************************
// AnalyticsEvent
//************************************************************************************************

class AnalyticsEvent: public Object,
					  public IAnalyticsEvent
{
public:
	DECLARE_CLASS (AnalyticsEvent, Object)

	AnalyticsEvent (StringID id = nullptr);
	AnalyticsEvent (const IAnalyticsEvent& other);

	Attributes& getMutableData ();

	int64 getTimestamp () const;
	int64 getLastTimestamp () const;

	// IAnalyticsEvent
	StringID CCL_API getID () const override;	
	void CCL_API setID (StringID id) override;
	const IAttributeList& CCL_API getData () const override;
	void CCL_API setData (const IAttributeList& data) override;

	// Object
	bool load (const Storage& storage) override;
	bool save (const Storage& storage) const override;

	CLASS_INTERFACE (IAnalyticsEvent, Object)

protected:
	MutableCString id;
	Attributes data;
};

//************************************************************************************************
// AnalyticsEventFactory
//************************************************************************************************

class AnalyticsEventFactory: public Object,
							 public IClassAllocator
{
public:
	DECLARE_CLASS (AnalyticsEventFactory, Object)

	// IClassAllocator
	tresult CCL_API createInstance (UIDRef cid, UIDRef iid, void** obj) override;

	CLASS_INTERFACE (IClassAllocator, Object)
};

//************************************************************************************************
// AnalyticsEventCache
//************************************************************************************************

class AnalyticsEventCache: public JsonStorableObject
{
public:
	DECLARE_CLASS (AnalyticsEventCache, JsonStorableObject)

	AnalyticsEventCache ();

	int getCount () const;
	const ObjectArray& getEvents () const;	
	void addOwned (AnalyticsEvent* e);
	void addShared (IAnalyticsEvent* e);
	void addAllFrom (const AnalyticsEventCache& cache);
	void removeAll ();

	// JsonStorableObject
	using SuperClass::load;
	using SuperClass::save;
	bool load (const Storage& storage) override;
	bool save (const Storage& storage) const override;

protected:
	ObjectArray events;
};

//************************************************************************************************
// LambdaAnalyticsEventFilter
//************************************************************************************************

template <typename T>
class LambdaAnalyticsEventFilter: public Unknown,
								  public AbstractAnalyticsEventFilter
{
public:
	LambdaAnalyticsEventFilter (const T& lambda)
	: lambda (lambda)
	{}

	// IAnalyticsEventFilter
	IAnalyticsEvent* CCL_API process (IAnalyticsEvent& e) { return lambda (e); }

	CLASS_INTERFACE (IAnalyticsEventFilter, Unknown)

protected:
	T lambda;
};

//************************************************************************************************
// AnalyticsTrackingPlan
//************************************************************************************************

class AnalyticsTrackingPlan: public Unknown,
							 public IAnalyticsTrackingPlan
							  
{
public:
	AnalyticsTrackingPlan ();

	// IAnalyticsTrackingPlan
	void CCL_API addFilter (StringID eventId, IAnalyticsEventFilter* filter, int64 evaluationPeriod = -1) override;
	void CCL_API addFilter (StringID inEventId, StringID outEventId, IAnalyticsEventFilter* filter, int64 evaluationPeriod = -1) override;

	void addFilter (const Vector<CString>& inEventIds, StringID outEventId, IAnalyticsEventFilter* filter, int64 evaluationPeriod = -1);
	template<typename Lambda> void addEventFilter (StringID eventId, const Lambda& process);

	void addPassThroughFilter (StringID eventId);

	IAnalyticsEventFilter* findFilter (StringID eventId) const;
	StringID getEvaluationEventId (StringID outEventId) const;

	class BatchGroup;
	void addBatchGroup (const Vector<CString>& eventIds);
	BatchGroup* findBatchGroup (StringID eventId) const;

	void initializeLastTimeStamps (int64 unixTime, bool force = false);
	void onTimer (int64 unixTime);
	int64 getEvaluationPeriod () const; ///< shortest evaluation period of all filters
	int64 getNextEvaluationTime () const; ///< in seconds (unix time); next time when evaluateData is required
	void setNextEvaluationTime (StringID eventid, int64 unixTime);

	int64 getLastEventTimestamp (StringID eventId) const; ///< in seconds (unix time); last time when event was successfuly sent to output
	void setLastEventTimestamp (StringID eventId, int64 timestamp);

	void storeSettings ();
	void restoreSettings ();
	void terminate ();

	CLASS_INTERFACE (IAnalyticsTrackingPlan, Unknown)

protected:
	struct FilterEntry
	{
		MutableCString eventId;
		AutoPtr<IAnalyticsEventFilter> filter;
		int64 evaluationPeriod; // in seconds
		int64 nextEvaluationTime; // in seconds (unix time)
		int64 lastEventTimestamp; // in seconds (unix time)

		FilterEntry (StringID eventId = CString::kEmpty, IAnalyticsEventFilter* filter = nullptr, int64 evaluationPeriod = - 1)
		: eventId (eventId),
		  filter (filter),
		  evaluationPeriod (evaluationPeriod),
		  nextEvaluationTime (NumericLimits::kMaxInt64),
		  lastEventTimestamp (0)
		{}

		bool operator > (const FilterEntry& other) const { return eventId.compare (other.eventId) > 0; }
		bool operator == (const FilterEntry& other) const { return eventId == other.eventId; }
	};

	Vector<FilterEntry> filterEntries;
	ObjectArray batchGroups;

	static StringID kContextNextEvaluationTime;
	static StringID kContextLastEventTimestamp;
	static StringID kContextLastBatchTimestamp;

	class PassThroughEventFilter;

	static void setNextEvaluationTime (FilterEntry& entry, int64 unixTime);
	static int64 getRandomizedEvaluationPeriod (int64 period);

	FilterEntry* findFilterEntry (StringID eventId) const;
	void addFilterInternal (StringID eventId, IAnalyticsEventFilter* filter, int64 evaluationPeriod = -1);
};

//************************************************************************************************
// AnalyticsTrackingPlan::BatchGroup
//************************************************************************************************

class AnalyticsTrackingPlan::BatchGroup: public Object
{
public:
	BatchGroup ();

	void addEventID (StringID eventId);
	bool containsEvent (StringID eventId) const;

	String getBatchID (StringID eventId);

	String getCurrentBatchID () const;
	int64 getCurrentTimeStamp () const;

	int64 getLastTimeStamp () const;
	void setLastTimeStamp (int64 unixTime);

	void storeSettings ();
	void restoreSettings ();

private:
	String currentBatchId;
	int64 currentTimeStamp;
	int64 lastTimeStamp;
	ObjectArray eventEntries;

	class EventEntry: public Object
	{
	public:
		EventEntry (StringID eventId)
		: eventId (eventId)
		{}

		PROPERTY_MUTABLE_CSTRING (eventId, EventID)
		PROPERTY_STRING (lastBatchId, LastBatchID)
	};

	EventEntry* findEventEntry (StringID eventId) const;
	void prepareNextBatch ();
	String generateBatchID () const;
	CStringPtr getStorageID () const;
};

//////////////////////////////////////////////////////////////////////////////////////////////////
// inline
//////////////////////////////////////////////////////////////////////////////////////////////////

template<typename Lambda>
inline void AnalyticsTrackingPlan::addEventFilter (StringID eventId, const Lambda& process)
{ addFilter (eventId, NEW LambdaAnalyticsEventFilter<Lambda> (process)); }

inline String AnalyticsTrackingPlan::BatchGroup::getCurrentBatchID () const
{ return currentBatchId; }

inline int64 AnalyticsTrackingPlan::BatchGroup::getCurrentTimeStamp () const
{ return currentTimeStamp; }

inline int64 AnalyticsTrackingPlan::BatchGroup::getLastTimeStamp () const
{ return lastTimeStamp; }

inline void AnalyticsTrackingPlan::BatchGroup::setLastTimeStamp (int64 unixTime)
{ lastTimeStamp = unixTime; }

} // namespace CCL

#endif // _ccl_apptrackinghandler_h
