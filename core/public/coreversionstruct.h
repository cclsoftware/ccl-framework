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
// Filename    : core/public/coreversionstruct.h
// Description : Version Structure
//
//************************************************************************************************

#ifndef _coreversionstruct_h
#define _coreversionstruct_h

#include "core/public/coretypes.h"

namespace Core {

//************************************************************************************************
// Version
//************************************************************************************************

struct Version
{
	int major;
	int minor;
	int revision;
	int build;

	Version (int major = 0, int minor = 0, int revision = 0, int build = 0)
	: major (major),
	  minor (minor),
	  revision (revision),
	  build (build)
	{}

	enum Format
	{
		kLong,		///< 1.0.0.0
		kMedium,	///< 1.0.0
		kShort		///< 1.0
	};

	template <typename StringType> void toCString (StringType& string, Format format = kLong) const;
	Version& fromCString (CStringPtr string);

	int compare (const Version& v) const;
	bool isWithin (const Version& minVersion, const Version& maxVersion) const;

	bool operator == (const Version& v) const { return compare (v) == 0; }
	bool operator != (const Version& v) const { return compare (v) != 0; }
	bool operator >  (const Version& v) const { return compare (v) <  0; }
	bool operator >= (const Version& v) const { return compare (v) <= 0; }
	bool operator <  (const Version& v) const { return compare (v) >  0; }
	bool operator <= (const Version& v) const { return compare (v) >= 0; }
};

//////////////////////////////////////////////////////////////////////////////////////////////////
// Version implementation
//////////////////////////////////////////////////////////////////////////////////////////////////

template <typename StringType> 
void Version::toCString (StringType& string, Format format) const
{
	switch(format)
	{
	case kShort : 
		string.appendFormat ("%d.%d", major, minor);
		break;
	case kMedium :
		string.appendFormat ("%d.%d.%d", major, minor, revision);
		break;
	default :
		string.appendFormat ("%d.%d.%d.%d", major, minor, revision, build);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline Version& Version::fromCString (CStringPtr cString)
{
	ASSERT (cString)
	::sscanf (cString, "%d.%d.%d.%d", &major, &minor, &revision, &build);
	return *this;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline bool Version::isWithin (const Version& minVersion, const Version& maxVersion) const
{
	return *this >= minVersion && *this <= maxVersion; 
}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline int Version::compare (const Version& v) const
{
	return	v.major == major ? 
				v.minor == minor ? 
					v.revision == revision ? 
						v.build == build ? 
							0 
						: v.build - build
					: v.revision - revision
				: v.minor - minor 
			: v.major - major;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

} // namespace Core

#endif // _coreversionstruct_h
