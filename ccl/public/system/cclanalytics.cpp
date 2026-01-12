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
// Filename    : ccl/public/system/cclanalytics.cpp
// Description : Analytics Helpers
//
//************************************************************************************************

#include "ccl/public/system/cclanalytics.h"

#include "ccl/public/systemservices.h"

using namespace CCL;

//************************************************************************************************
// Helper functions
//************************************************************************************************

void CCL::ccl_analytics_event (StringID eventId, const IAttributeList* data)
{
	System::GetAnalyticsManager ().addEvent (eventId, data);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL::ccl_analytics_event (StringID eventId, const IAnalyticsDataProvider& dataProvider)
{
	IAnalyticsManager& manager = System::GetAnalyticsManager ();
	if(manager.isTrackingActive ())
	{
		AutoPtr<IAttributeList> data;
		manager.createInstance (ClassID::Attributes, ccl_iid<IAttributeList> (), data.as_ppv ());
		if(data)
		{
			dataProvider.getEventData (*data);
			manager.addEvent (eventId, data);
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

MutableCString CCL::ccl_analytics_classify (IAnalyticsEvent& e, IAnalyticsEventClassifier* eventClassifier)
{
	// appContext from event data 
	AttributeReadAccessor attributes (e.getData ());
	MutableCString context (attributes.getString (CCL::AnalyticsID::kApplicationContext));

	// additional context provided by classifier
	MutableCString eventContext;
	if(eventClassifier)
	{
		eventClassifier->classifyEvent (eventContext, e);

		// combine into one string
		if(!eventContext.isEmpty ())
		{
			if(!context.isEmpty ())
				context.append ('.');
			context.append (eventContext);
		}
	}
	return context;
}
