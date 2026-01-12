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
// Filename    : ccl/base/storage/persistence/dataitem.h
// Description : Item in DataStore
//
//************************************************************************************************

#ifndef _ccl_dataitem_h
#define _ccl_dataitem_h

#include "ccl/base/storage/persistence/persistence.h"

#include "ccl/base/storage/url.h"

#include "ccl/public/base/datetime.h"

namespace CCL {

//************************************************************************************************
// DataItem
//************************************************************************************************

class DataItem: public Persistence::PersistentObject<Object>
{
public:
	DECLARE_CLASS_ABSTRACT (DataItem, Object)
	DECLARE_PROPERTY_NAMES (DataItem)

	DataItem ();

	PROPERTY_OBJECT (Url, url, Url)
	PROPERTY_STRING (title, Title)
	PROPERTY_VARIABLE (int, useCount, UseCount)

	bool wasLastModifiedAt (const DateTime& date) const;
	void setLastModified (const DateTime& date);
	void addUsage ();

	// IPersistentObject
	void CCL_API storeMembers (Persistence::IObjectState& state) const override;
	void CCL_API restoreMembers (Persistence::IObjectState& state) override;

private:
	int64 lastUsed; ///< ordinal value, unspecified base, may only be used for comparison
	int64 modified; ///< ordinal value, unspecified base, may only be used for comparison
};

} // namespace CCL

#endif // _ccl_dataitem_h
