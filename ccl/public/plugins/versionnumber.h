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
// Filename    : ccl/public/plugins/versionnumber.h
// Description : Version Number
//
//************************************************************************************************

#ifndef _ccl_versionnumber_h
#define _ccl_versionnumber_h

#include "core/public/coreversionstruct.h"

#include "ccl/public/text/cclstring.h"

namespace CCL {

//************************************************************************************************
// VersionNumber
//************************************************************************************************

struct VersionNumber: Core::Version
{
	using Version::Version;

	String print (Format format = kLong) const;
	VersionNumber& scan (StringRef string);

	operator String () const { return print (); }
};

} // namespace CCL

#endif // _ccl_versionnumber_h
