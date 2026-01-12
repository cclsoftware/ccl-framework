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
// Filename    : ccl/app/controls/statsdata.h
// Description : Statistics Data classes
//
//************************************************************************************************

#ifndef _statsdata_h
#define _statsdata_h

#include "ccl/public/text/cstring.h"
#include "ccl/public/system/istatistics.h"

#include "ccl/base/collections/objectarray.h"

namespace CCL {

//************************************************************************************************
// StatsCollection
//************************************************************************************************

class StatsCollection: public Object,
					   public IStatisticsCollection
{
public:
	DECLARE_CLASS_ABSTRACT (IStatisticsCollection, Object)
	
	StatsCollection (StringID name = CString::kEmpty);
	
	PROPERTY_MUTABLE_CSTRING (name, Name)
	
	void addValue (int64 timestamp, float normalizedValue);
	IStatisticsCollection::Value lastValue () const;
	
	// IStatisticsCollection
	int CCL_API countValues () const override;
	IStatisticsCollection::Value CCL_API getValue (int index) const override;

	CLASS_INTERFACE (IStatisticsCollection, Object)
	
protected:
	Vector<IStatisticsCollection::Value> stats;
};

//************************************************************************************************
// StatsCollectionSet
//************************************************************************************************

class StatsCollectionSet
{
public:
	StatsCollectionSet ();
	
	void addCollection (StringID collectionName);
	StatsCollection* lookupCollection (StringID collectionName) const;
	
protected:
	ObjectArray statsCollections;
};

} // namespace CCL

#endif // _statsdata_h

