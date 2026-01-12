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
// Filename    : ccl/platform/linux/system/mountinfo.h
// Description : Class providing information about mount points
//
//************************************************************************************************

#ifndef _ccl_mountinfo_h
#define _ccl_mountinfo_h

#include "ccl/public/text/cstring.h"
#include "ccl/public/collections/vector.h"

namespace CCL {

//************************************************************************************************
// MountInfo
//************************************************************************************************

class MountInfo
{
public:
	struct Entry
	{
		uint32 mountID;
		uint32 parentID;
		dev_t deviceID;
		String root;
		String mountPoint;
		String mountOptions;
		String optionalFields;
		String filesystemType;
		String mountSource;
		String superOptions;
	};

	bool load ();
	const Entry* find (UrlRef path) const;
	const Vector<Entry>& getEntries () const { return entries; }

private:
	Vector<Entry> entries;

	void parseLine (StringRef line);
};

} // namespace CCL

#endif // _ccl_mountinfo_h
