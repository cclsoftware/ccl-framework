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
// Filename    : ccl/system/analyticsmanager.h
// Description : Analytics Manager
//
//************************************************************************************************

#ifndef _ccl_analyticsmanager_h
#define _ccl_analyticsmanager_h

#include "ccl/public/system/ianalyticsmanager.h"

#include "ccl/base/singleton.h"

#include "ccl/public/collections/vector.h"

namespace CCL {

//************************************************************************************************
// AnalyticsManager
//************************************************************************************************

class AnalyticsManager: public Object,
						public IAnalyticsManager,
						public Singleton<AnalyticsManager>
{
public:
	~AnalyticsManager ();

	// IAnalyticsManager
	tresult CCL_API createInstance (UIDRef cid, UIDRef iid, void** obj) override;
	tresult CCL_API setEventAllocator (IClassAllocator* allocator) override;
	tresult CCL_API addEventSink (IAnalyticsEventSink* eventSink) override;
	tresult CCL_API removeEventSink (IAnalyticsEventSink* eventSink) override;
	tbool CCL_API isTrackingActive () const override;
	tresult CCL_API addEvent (StringID eventId, const IAttributeList* data = nullptr) override;

	CLASS_INTERFACE2 (IAnalyticsManager, IClassAllocator, Object)

protected:
	SharedPtr<IClassAllocator> eventAllocator;
	Vector<IAnalyticsEventSink*> eventSinks;
};

} // namespace CCL

#endif // _ccl_ianalyticsmanager_h
