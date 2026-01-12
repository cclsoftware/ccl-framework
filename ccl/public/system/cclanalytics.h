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
// Filename    : ccl/public/system/cclanalytics.h
// Description : Analytics Helpers
//
//************************************************************************************************

#ifndef _cclanalytics_h
#define _cclanalytics_h

#include "ccl/public/system/ianalyticsmanager.h"

#include "ccl/public/storage/iattributelist.h"

namespace CCL {

interface IAnalyticsDataProvider;

//************************************************************************************************
// Helper functions
//************************************************************************************************

/** Shorcut to add analytics event via global manager. 
	\ingroup ccl_system */
void ccl_analytics_event (StringID eventId, const IAttributeList* data = nullptr);

/**	Shorcut to add analytics event via global manager.
	Checks if tracking is enabled, then queries data provider for event data
	\ingroup ccl_system */
void ccl_analytics_event (StringID eventId, const IAnalyticsDataProvider& dataProvider);

/** Build a string combining kApplicationContext from event data and context provided by classifier. */
MutableCString ccl_analytics_classify (IAnalyticsEvent& e, IAnalyticsEventClassifier* eventClassifier);

//************************************************************************************************
// IAnalyticsDataProvider
//************************************************************************************************

interface IAnalyticsDataProvider
{
	virtual void CCL_API getEventData (IAttributeList& data) const = 0;
};

//************************************************************************************************
// LambdaAnalyticsDataProvider
//************************************************************************************************

template<typename Lambda>
class LambdaAnalyticsDataProvider: public IAnalyticsDataProvider
{
public:
	LambdaAnalyticsDataProvider (Lambda getData)
	: getData (getData)
	{}

	// IAnalyticsDataProvider
	void CCL_API getEventData (IAttributeList& data) const
	{
		AttributeAccessor attributes (data);
		getData (attributes);
	}

protected:
	Lambda getData;
};

//////////////////////////////////////////////////////////////////////////////////////////////////

namespace AnalyticsID
{
	template<typename Lambda>
	inline LambdaAnalyticsDataProvider<Lambda> provide (const Lambda& getData)
	{
		return LambdaAnalyticsDataProvider<Lambda> (getData);
	}
}

} // namespace CCL

#endif // _cclanalytics_h
