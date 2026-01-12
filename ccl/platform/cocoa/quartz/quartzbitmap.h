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
// Filename    : ccl/platform/cocoa/quartz/quartzbitmap.h
// Description : Quartz Bitmap
//
//************************************************************************************************

#ifndef _ccl_quartz_bitmap_h
#define _ccl_quartz_bitmap_h

#include "ccl/gui/graphics/nativegraphics.h"
#include "ccl/platform/cocoa/interfaces/iquartzbitmap.h"
#include "ccl/platform/cocoa/quartz/quartzrendertarget.h"
#include "ccl/public/base/buffer.h"

#include <CoreGraphics/CGImage.h>
#include <CoreGraphics/CGLayer.h>

namespace CCL {
interface IStream;

namespace MacOS {

//************************************************************************************************
// QuartzBitmap
//************************************************************************************************

class QuartzBitmap: public NativeBitmap,
					public IQuartzBitmap
{
public:
	DECLARE_CLASS (QuartzBitmap, NativeBitmap)

	QuartzBitmap (PointRef sizeInPixel = Point (1,1), PixelFormat format = kAny, float contentScaleFactor = 1.f);
	QuartzBitmap (IStream* stream);
	QuartzBitmap (CGImageRef image);
	QuartzBitmap (IBitmapDecoder* customDecoder);
	~QuartzBitmap ();

	void* getBits () { return bits ? bits->getBufferAddress () : 0; }
	void recreate ();
	tresult draw (CGContextRef context, RectRef src, RectRef dst, const ImageMode* mode);
	tresult draw (CGContextRef context, RectFRef src, RectFRef dst, const ImageMode* mode);

	// IQuartzBitmap
	CGImageRef getCGImage () override { return image; }

	// NativeBitmap
	tresult draw (NativeGraphicsDevice& device, PointRef pos, const ImageMode* mode) override;
	tresult draw (NativeGraphicsDevice& device, PointFRef pos, const ImageMode* mode) override;
	tresult draw (NativeGraphicsDevice& device, RectRef src, RectRef dst, const ImageMode* mode) override;
	tresult draw (NativeGraphicsDevice& device, RectFRef src, RectFRef dst, const ImageMode* mode) override;
	tresult tile (NativeGraphicsDevice& device, int method, RectRef src, RectRef dest, RectRef clip, RectRef margins) override;
	PixelFormat CCL_API getPixelFormat () const override { return kRGBAlpha; }
	tresult CCL_API lockBits (BitmapLockData& data, PixelFormat format, int mode) override;
	tresult CCL_API unlockBits (BitmapLockData& data) override;
	tresult CCL_API scrollPixelRect (const Rect& rect, const Point& delta) override;

	CLASS_INTERFACE (IQuartzBitmap, NativeBitmap)

protected:
	CGImageRef image;
	AutoPtr<Buffer> bits;
	bool mustDecode;
	AutoPtr<IBitmapDecoder> customDecoder;
	
	friend class QuartzBitmapDecoder;
	void decode ();
};

//************************************************************************************************
// QuartzBitmapRenderBitmap
//************************************************************************************************

class QuartzBitmapRenderTarget: public Object,
							 	public QuartzRenderTarget
{
public:
	QuartzBitmapRenderTarget (QuartzBitmap& nativeBitmap);
	~QuartzBitmapRenderTarget ();

	void onScroll (RectRef rect, PointRef delta);
	
	//QuartzRenderTarget
	CGContextRef getContext () { return context; }
	float getContentScaleFactor () const;
	void flush ();

protected:
	QuartzBitmap& bitmap;
	CGContextRef context;
	
	void createContext ();
	void releaseContext ();
};

} // namespace MacOS
} // namespace CCL

#endif // _ccl_quartz_bitmap_h
