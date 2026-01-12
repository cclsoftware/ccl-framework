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
// Filename    : ccl/extras/analytics/segmentanalyticsclient.cpp
// Description : Segment HTTP Tracking API Client
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "ccl/extras/analytics/segmentanalyticsclient.h"
#include "ccl/extras/web/webxhroperation.h"

#include "ccl/base/asyncoperation.h"
#include "ccl/base/storage/jsonarchive.h"
#include "ccl/base/storage/url.h"
#include "ccl/base/security/cryptomaterial.h"

#include "ccl/public/network/web/httpstatus.h"
#include "ccl/public/network/web/iwebrequest.h"
#include "ccl/public/network/web/iwebcredentials.h"
#include "ccl/public/plugservices.h"

using namespace CCL;
using namespace Web;

#define SKIP_SENDING_DATA (0 && DEBUG) // e.g. while experimenting with data format

//************************************************************************************************
// SegmentApiIds
//************************************************************************************************

namespace SegmentApiIds
{
	const CStringPtr kIdentify = "identify";
	const CStringPtr kTrack = "track";
	const CStringPtr kBatch = "batch";

	const CStringPtr kEvent = "event";
	const CStringPtr kUserID = "userId";
	const CStringPtr kProperties = "properties";
	const CStringPtr kType = "type";
}

#define SEGMENT_API_URL "https://api.segment.io/v1"

//************************************************************************************************
// SegmentAnalyticsClient
//************************************************************************************************

DEFINE_CLASS_HIDDEN (SegmentAnalyticsClient, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

void SegmentAnalyticsClient::setConfiguration (const SegementClientConfiguration& _configuration)
{
	configuration = _configuration;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IAsyncOperation* SegmentAnalyticsClient::identify (StringID userId)
{
	Attributes jsonStructure;
	jsonStructure.set (SegmentApiIds::kUserID, userId);

	return send (jsonStructure, SegmentApiIds::kIdentify);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IAsyncOperation* SegmentAnalyticsClient::track (StringID eventId, const IAttributeList* properties)
{
	Attributes jsonStructure;
	jsonStructure.set (SegmentApiIds::kEvent, eventId);
	prepareEventAttributes (jsonStructure, *properties);
	return send (jsonStructure, SegmentApiIds::kTrack);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IAsyncOperation* CCL_API SegmentAnalyticsClient::writeEvents (IAnalyticsEvent* events[], int count)
{
	Variant userId;
	Attributes jsonStructure;

	for(int i = 0; i < count; ++i)
	{
		auto event = events[i];
		Attributes* eventAttributes = NEW Attributes;
		eventAttributes->set (SegmentApiIds::kType, SegmentApiIds::kTrack);
		eventAttributes->set (SegmentApiIds::kEvent, event->getID ());

		prepareEventAttributes (*eventAttributes, event->getData ());

		jsonStructure.queue (SegmentApiIds::kBatch, eventAttributes, Attributes::kOwns);
	}
	return send (jsonStructure, SegmentApiIds::kBatch);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SegmentAnalyticsClient::prepareEventAttributes (Attributes& eventAttributes, const IAttributeList& sourceAttributes)
{
	AutoPtr<Attributes> adjustedData (NEW Attributes);
	adjustedData->copyFrom (sourceAttributes);
	auto userId = adjustedData->getVariant (AnalyticsID::kUserID).toString ();
	adjustedData->remove (AnalyticsID::kUserID);
	eventAttributes.set (SegmentApiIds::kUserID, userId);

	if(adjustedData->countAttributes () > 0)
		eventAttributes.set (SegmentApiIds::kProperties, adjustedData, Attributes::kShare);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IAsyncOperation* SegmentAnalyticsClient::send (const Attributes& content, CStringPtr endpoint)
{
	AutoPtr<IStream> jsonData = JsonUtils::serialize (content);
	Url url (SEGMENT_API_URL);
	url.descend (endpoint);

	#if DEBUG_LOG
	UnknownPtr<IMemoryStream> memStream (jsonData);
	CCL_PRINTF ("Segment: %.*s\n", memStream ? memStream->getBytesWritten () : 0, memStream ? memStream->getMemoryAddress () : "")
	#endif

	#if SKIP_SENDING_DATA
	ASSERT (DEBUG)
	return AsyncOperation::createCompleted ();
	#endif

	SharedPtr<IXMLHttpRequest> request = ccl_new<IXMLHttpRequest> (ClassID::XMLHttpRequest);

	request->open (HTTP::kPOST, url, true, configuration.writeKey, "", String (Meta::kBasic));
	request->setRequestHeader (Meta::kContentType, JsonArchive::kMimeType);
	request->send (Variant (jsonData));

	auto operation = NEW AsyncXHROperation (request);
	operation->setState (IAsyncInfo::kStarted);

	return operation;
}
