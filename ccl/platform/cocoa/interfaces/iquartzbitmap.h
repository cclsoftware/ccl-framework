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
// Filename    : ccl/platform/cocoa/interfaces/iquartzbitmap.h
// Description : Quartz Graphics Interface
//
//************************************************************************************************

#ifndef _ccl_iquartz_bitmap_h
#define _ccl_iquartz_bitmap_h

#include "ccl/public/base/iunknown.h"

#include <CoreGraphics/CGImage.h>

namespace CCL {
namespace MacOS {

//************************************************************************************************
// IQuartzBitmap
//************************************************************************************************

interface IQuartzBitmap: IUnknown
{
	virtual CGImageRef CCL_API getCGImage () = 0;

	DECLARE_IID (IQuartzBitmap)
};

} // namespace MacOS
} // namespace CCL

#endif // _ccl_iquartz_bitmap_h
