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
// Filename    : ccl/public/system/ianalyticsmanager.h
// Description : Analytics Manager Interface
//
//************************************************************************************************

#ifndef _ccl_ianalyticsmanager_h
#define _ccl_ianalyticsmanager_h

#include "ccl/public/base/iunknown.h"

namespace CCL {

interface IAttributeList;
interface IAsyncOperation;
class MutableCString;

//////////////////////////////////////////////////////////////////////////////////////////////////

namespace ClassID
{
	DEFINE_CID (AnalyticsEvent, 0x496dfa85, 0x9e58, 0x468a, 0x88, 0x6e, 0xc8, 0x45, 0x37, 0x61, 0xd9, 0x2b)
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// Analytics Identifier
//////////////////////////////////////////////////////////////////////////////////////////////////

namespace AnalyticsID
{
	// Common event properties
	const CStringPtr kApplicationName = "appName";
	const CStringPtr kApplicationVersion = "appVersion";
	const CStringPtr kBuildNumber = "buildNumber";
	const CStringPtr kPlatform = "platform";
	const CStringPtr kArchitecture = "architecture";

	const CStringPtr kUserID = "userId";
	const CStringPtr kTimestamp = "timestamp";
	const CStringPtr kLastTimestamp = "lastTimestamp";
	const CStringPtr kBatchID = "batchId";

	// optional property: string indicating the context where an event originated
	const CStringPtr kApplicationContext = "appContext";
}

//************************************************************************************************
// IAnalyticsEvent
//************************************************************************************************

interface IAnalyticsEvent: IUnknown
{
	/** Get event identifier. */
	virtual StringID CCL_API getID () const = 0;

	/** Set event identifier. */
	virtual void CCL_API setID (StringID id) = 0;
	
	/** Get event data. */
	virtual const IAttributeList& CCL_API getData () const = 0;

	/** Set event data. */
	virtual void CCL_API setData (const IAttributeList& data) = 0;

	DECLARE_IID (IAnalyticsEvent)
};

DEFINE_IID (IAnalyticsEvent, 0xda2e7060, 0xd9c0, 0x4656, 0x87, 0x1f, 0xc2, 0xe, 0xd2, 0xfc, 0x6f, 0x3d)

//************************************************************************************************
// IAnalyticsEventSink
//************************************************************************************************

interface IAnalyticsEventSink: IUnknown
{
	/** Add analytics event. */
	virtual void CCL_API addEvent (IAnalyticsEvent& e) = 0;

	DECLARE_IID (IAnalyticsEventSink)
};

DEFINE_IID (IAnalyticsEventSink, 0x138f3bb1, 0x6835, 0x45c9, 0x93, 0xf4, 0xd3, 0x79, 0xdf, 0x22, 0x7e, 0x39)

//************************************************************************************************
// IAnalyticsManager
//************************************************************************************************

interface IAnalyticsManager: IClassAllocator
{
	/** Assign allocator, must support ClassID::AnalyticsEvent and ClassID::Attributes. */
	virtual tresult CCL_API setEventAllocator (IClassAllocator* allocator) = 0;

	/** Register analytics event sink. */
	virtual tresult CCL_API addEventSink (IAnalyticsEventSink* eventSink) = 0;
	
	/** Unregister analytics event sink. */
	virtual tresult CCL_API removeEventSink (IAnalyticsEventSink* eventSink) = 0;	

	/** Check if tracking is currently active, i.e. one or more sinks registered. */
	virtual tbool CCL_API isTrackingActive () const = 0;

	/** Add analytics event. The event is passed on to all sinks. */
	virtual tresult CCL_API addEvent (StringID eventId, const IAttributeList* data = nullptr) = 0;

	DECLARE_IID (IAnalyticsManager)
};

DEFINE_IID (IAnalyticsManager, 0x9068de65, 0x426c, 0x4832, 0x94, 0x4e, 0xf0, 0x61, 0xe4, 0xbe, 0x52, 0x41)

//////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////
// Additonal Analytics Interfaces
//////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////

//************************************************************************************************
// IAnalyticsEventFilter
//************************************************************************************************

interface IAnalyticsEventFilter: IUnknown
{
	virtual IAnalyticsEvent* CCL_API process (IAnalyticsEvent& e) = 0;

	virtual void CCL_API evaluateData () = 0;

	virtual void CCL_API onWriteCompleted (StringID eventId) = 0;

	virtual void CCL_API onIdle () = 0;

	virtual void CCL_API terminate () = 0;

	DECLARE_IID (IAnalyticsEventFilter)
};

DEFINE_IID (IAnalyticsEventFilter, 0xFFB75A27, 0x6060, 0x416E, 0xB4, 0x2D, 0xD2, 0xBD, 0x25, 0xD9, 0x3B, 0xF5)

//************************************************************************************************
// AbstractAnalyticsEventFilter
//************************************************************************************************

class AbstractAnalyticsEventFilter: public IAnalyticsEventFilter
{
public:
	IAnalyticsEvent* CCL_API process (IAnalyticsEvent& e) override { return nullptr; }
	void CCL_API evaluateData () override {}
	void CCL_API onWriteCompleted (StringID eventId) override {}
	void CCL_API onIdle () override {}
	void CCL_API terminate () override {}
};

//************************************************************************************************
// IAnalyticsTrackingPlan
//************************************************************************************************

interface IAnalyticsTrackingPlan: public IUnknown
{
	/** Add a filter that processes a given event. */
	virtual void CCL_API addFilter (StringID eventId, IAnalyticsEventFilter* filter, int64 evaluationPeriod = -1) = 0;

	/** Add a filter that evalutes inEventId with the given period and emits & processes outEventId. */
	virtual void CCL_API addFilter (StringID inEventId, StringID outEventId, IAnalyticsEventFilter* filter, int64 evaluationPeriod = -1) = 0;

	DECLARE_IID (IAnalyticsTrackingPlan)
};

DEFINE_IID (IAnalyticsTrackingPlan, 0x972CBD7E, 0x06B3, 0x48AB, 0xAA, 0x3C, 0x87, 0xB2, 0x05, 0x13, 0x6B, 0x50)

//************************************************************************************************
// IAnalyticsOutput
//************************************************************************************************

interface IAnalyticsOutput: IUnknown
{
	virtual IAsyncOperation* CCL_API writeEvents (IAnalyticsEvent* events[], int count) = 0;

	DECLARE_IID (IAnalyticsOutput)
};

DEFINE_IID (IAnalyticsOutput, 0xc516c174, 0xf49d, 0x47e4, 0x86, 0xf9, 0xa1, 0x29, 0x3e, 0xd9, 0xad, 0xff)

//************************************************************************************************
// IAnalyticsEventClassifier
//************************************************************************************************

interface IAnalyticsEventClassifier: IUnknown
{
	/** Optionally provide a string describing the context of an event. */
	virtual void CCL_API classifyEvent (CCL::MutableCString& context, const IAnalyticsEvent& e) = 0;

	DECLARE_IID (IAnalyticsEventClassifier)
};

DEFINE_IID (IAnalyticsEventClassifier, 0x609A3C9F, 0x00F5, 0x4B18, 0x80, 0x49, 0x9A, 0xDF, 0x0B, 0xAB, 0xF3, 0xC8)

} // namespace CCL

#endif // _ccl_ianalyticsmanager_h
