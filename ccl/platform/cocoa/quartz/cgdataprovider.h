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
// Filename    : ccl/platform/cocoa/quartz/cgdataprovider.h
// Description : Quartz data provider
//
//************************************************************************************************

#ifndef _ccl_cgdataprovider_h
#define _ccl_cgdataprovider_h

#include "ccl/public/base/platform.h"

#include <CoreGraphics/CGDataProvider.h>

namespace CCL {

interface IStream;

//************************************************************************************************
// CGStreamDataProvider
//************************************************************************************************

class CGStreamDataProvider
{
public:
	static CGDataProviderRef create (IStream* stream);

protected:
	CGStreamDataProvider (IStream* stream);
	virtual ~CGStreamDataProvider ();

	IStream* stream;

	static size_t getBytes (void* info, void* buffer, size_t count);
	static off_t skipBytes (void* info, off_t count);
	static void rewind (void* info);
	static void releaseInfo (void* info);
};

} // namespace CCL

#endif // _ccl_cgdataprovider_h
