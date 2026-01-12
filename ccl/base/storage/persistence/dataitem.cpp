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
// Filename    : ccl/base/storage/persistence/dataitem.cpp
// Description : Item in DataStore
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "ccl/base/storage/persistence/dataitem.h"

#include "ccl/public/system/isysteminfo.h"
#include "ccl/public/systemservices.h"

using namespace CCL;
using namespace Persistence;

//************************************************************************************************
// DataItem
//************************************************************************************************

DEFINE_CLASS (DataItem, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_PROPERTY_NAMES (DataItem)
	DEFINE_PROPERTY_TYPE ("url", ITypeInfo::kString)
	DEFINE_PROPERTY_TYPE ("title", ITypeInfo::kString)
	DEFINE_PROPERTY_TYPE ("useCount", ITypeInfo::kInt)
	DEFINE_PROPERTY_TYPE ("lastUsed", ITypeInfo::kInt)
	DEFINE_PROPERTY_TYPE ("modified", ITypeInfo::kInt)
END_PROPERTY_NAMES (DataItem)

//////////////////////////////////////////////////////////////////////////////////////////////////

DataItem::DataItem ()
: useCount (0),
  lastUsed (0),
  modified (0)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API DataItem::storeMembers (IObjectState& state) const
{
	state.setString ("url", UrlFullString (url, true));
	state.set ("title", title);
	state.set ("useCount", useCount);
	state.set ("lastUsed", lastUsed);
	state.set ("modified", modified);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API DataItem::restoreMembers (IObjectState& state)
{
	url.setUrl (state.get ("url"), IUrl::kDetect);
	title = state.get ("title");
	useCount = state.get ("useCount");
	lastUsed = state.get ("lastUsed");
	modified = state.get ("modified");
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool DataItem::wasLastModifiedAt (const DateTime& date) const
{
	return date.toOrdinal () == modified;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DataItem::setLastModified (const DateTime& date)
{
	modified = date.toOrdinal ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DataItem::addUsage ()
{
	DateTime now;
	System::GetSystem ().getLocalTime (now);

	lastUsed = now.toOrdinal ();
	useCount++;
}
