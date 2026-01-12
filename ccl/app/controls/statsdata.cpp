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
// Filename    : ccl/app/controls/statsdata.cpp
// Description : Statistics Data classes
//
//************************************************************************************************

#include "ccl/app/controls/statsdata.h"

#include "ccl/public/math/mathprimitives.h"

using namespace CCL;

//************************************************************************************************
// StatsCollection
//************************************************************************************************

DEFINE_CLASS_HIDDEN (StatsCollection, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

StatsCollection::StatsCollection (StringID name)
: name (name) 
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void StatsCollection::addValue (int64 timestamp, float normalized)
{
	ccl_upper_limit (normalized, 1.f);
	ccl_lower_limit (normalized, 0.f);
	
	IStatisticsCollection::Value value;
	value.timestamp = timestamp;
	value.normalized = normalized;
	
	stats.add (value);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IStatisticsCollection::Value StatsCollection::lastValue () const
{
	if(stats.count () > 0)
		return getValue (stats.count () - 1);
	return Value ();
}
						 
//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API StatsCollection::countValues () const
{
	return stats.count ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IStatisticsCollection::Value CCL_API StatsCollection::getValue (int index) const
{
	if(index >= 0 && index < stats.count ())
		return stats.at (index);
	ASSERT (false)
	return IStatisticsCollection::Value ();
}

//************************************************************************************************
// StatsCollectionSet
//************************************************************************************************

StatsCollectionSet::StatsCollectionSet ()
{
	statsCollections.objectCleanup (true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void StatsCollectionSet::addCollection (StringID collectionName)
{
	ASSERT (lookupCollection (collectionName) == nullptr) // duplicate name
	
	statsCollections.add (NEW StatsCollection (collectionName));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StatsCollection* StatsCollectionSet::lookupCollection (StringID collectionName) const
{
	ArrayForEach (statsCollections, StatsCollection, collection)
		if(collection->getName () == collectionName)
			return collection;
	EndFor
	return nullptr;
}
