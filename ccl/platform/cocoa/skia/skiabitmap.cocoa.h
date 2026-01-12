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
// Filename    : ccl/platform/cocoa/skia/skiabitmap.cocoa.h
// Description : Cocoa Skia Bitmap
//
//************************************************************************************************

#ifndef _ccl_skia_bitmap_cocoa_h
#define _ccl_skia_bitmap_cocoa_h

#include "ccl/platform/shared/skia/skiabitmap.h"
#include "ccl/platform/cocoa/quartz/quartzbitmap.h"

namespace CCL {

using namespace MacOS;

//************************************************************************************************
// CocoaSkiaBitmap
//************************************************************************************************

class CocoaSkiaBitmap: public SkiaBitmap,
					   public IQuartzBitmap
{
public:
	DECLARE_CLASS (CocoaSkiaBitmap, SkiaBitmap)
	
	CocoaSkiaBitmap (PointRef sizeInPixel = Point (1,1), PixelFormat format = kAny, float contentScaleFactor = 1.f);
	CocoaSkiaBitmap (IBitmapDecoder* customDecoder, bool alphaChannelUsed = true);
	CocoaSkiaBitmap (IMemoryStream* stream);
	CocoaSkiaBitmap (QuartzBitmap* quartzBitmap);

	bool isValid () const { return valid; }

	// SkiaBitmap
	tresult CCL_API unlockBits (BitmapLockData& data) override;
	tresult CCL_API scrollPixelRect (const Rect& rect, const Point& delta) override;

	// IQuartzBitmap
	CGImageRef getCGImage () override;

	CLASS_INTERFACE (IQuartzBitmap, SkiaBitmap)

protected:
	AutoPtr<QuartzBitmap> cachedBitmap;
	bool valid;
	
	void initWithQuartzBitmap (QuartzBitmap* quartzBitmap);
};

} // namespace CCL

#endif // _ccl_skia_bitmap_cocoa_h
