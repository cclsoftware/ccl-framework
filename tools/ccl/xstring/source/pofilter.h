//************************************************************************************************
//
// CCL String Extractor
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
// Filename    : pofilter.h
// Description : Portable Object Filter
//
//************************************************************************************************

#ifndef _pofilter_h
#define _pofilter_h

#include "xstringfilter.h"

namespace XString {

//************************************************************************************************
// PortableObjectFilter
//************************************************************************************************

class PortableObjectFilter: public Filter
{
public:
	PortableObjectFilter (Bundle& bundle, UrlRef path);

	// Filter
	bool create () override;
};

} // namespace XString

#endif // _pofilter_h
