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
// Filename    : ccl/extras/analytics/segmentanalyticsclient.h
// Description : Segment HTTP Tracking API Client
//				 https://segment.com/docs/connections/sources/catalog/libraries/server/http-api/
//
//************************************************************************************************

#ifndef _ccl_segmentanalyticsclient_h
#define _ccl_segmentanalyticsclient_h

#include "ccl/base/storage/attributes.h"

#include "ccl/public/text/cclstring.h"
#include "ccl/public/system/ianalyticsmanager.h"

namespace CCL {

//************************************************************************************************
// SegementClientConfiguration
//************************************************************************************************

struct SegementClientConfiguration
{
	String writeKey; ///< Segment API key for application
};

//************************************************************************************************
// SegmentAnalyticsClient
//************************************************************************************************

class SegmentAnalyticsClient: public Object,
							  public IAnalyticsOutput
{
public:
	DECLARE_CLASS (SegmentAnalyticsClient, Object)

	void setConfiguration (const SegementClientConfiguration& configuration);
	
	IAsyncOperation* identify (StringID userId);
	IAsyncOperation* track (StringID eventId, const IAttributeList* properties = nullptr);

	// IAnalyticsOutput
	IAsyncOperation* CCL_API writeEvents (IAnalyticsEvent* events[], int count) override;

	CLASS_INTERFACE (IAnalyticsOutput, Object)

protected:
	SegementClientConfiguration configuration;

	void prepareEventAttributes (Attributes& eventAttributes, const IAttributeList& sourceAttributes);
	IAsyncOperation* send (const Attributes& content, CStringPtr endpoint);
};

} // namespace CCL

#endif // _ccl_segmentanalyticsclient_h
