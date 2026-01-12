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
// Filename    : ccl/public/gui/graphics/iimagecache.h
// Description : Image Cache Interface
//
//************************************************************************************************

#ifndef _ccl_iimagecache_h
#define _ccl_iimagecache_h

#include "ccl/public/base/iunknown.h"

#include "ccl/public/gui/graphics/color.h"

namespace CCL {

interface IImage;

//////////////////////////////////////////////////////////////////////////////////////////////////

namespace ClassID
{
	DEFINE_CID (ImageCache, 0x9bc5ef85, 0x43cd, 0x4b45, 0xaf, 0x42, 0x48, 0x3c, 0x24, 0xae, 0x41, 0x8a)
}

//************************************************************************************************
// IImageCache
//************************************************************************************************

interface IImageCache: IUnknown
{
	virtual IImage* CCL_API lookup (IImage* image, ColorRef color, tbool drawAsTemplate = false) = 0;

	DECLARE_IID (IImageCache)
};

DEFINE_IID (IImageCache, 0x2f0db643, 0x5be0, 0x4b30, 0x89, 0x79, 0x96, 0xca, 0xb0, 0xf4, 0xad, 0x43)

} // namespace CCL

#endif // _ccl_iimagecache_h
