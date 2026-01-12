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
// Filename    : ccl/platform/shared/skia/skiabitmap.h
// Description : Skia Engine
//
//************************************************************************************************

#ifndef _ccl_skiabitmap_h
#define _ccl_skiabitmap_h

#include "ccl/platform/shared/skia/skiaglue.h"

#include "ccl/platform/shared/skia/skiarendertarget.h"

#include "ccl/gui/graphics/nativegraphics.h"
#include "ccl/public/base/buffer.h"

namespace CCL {

interface IStream;

//************************************************************************************************
// SkiaBitmap
//************************************************************************************************

class SkiaBitmap: public NativeBitmap
{
public:
	DECLARE_CLASS (SkiaBitmap, NativeBitmap)

	SkiaBitmap ();
	SkiaBitmap (PointRef sizeInPixel, PixelFormat format = kAny, float contentScaleFactor = 1.f);
	SkiaBitmap (IBitmapDecoder* customDecoder, bool alphaChannelUsed = true);
	SkiaBitmap (IMemoryStream* stream, bool alphaChannelUsed = true);

	SkCanvas* getCanvas ();
	bool saveTo (IStream& stream, const FileType& format);
	void flush ();
	
	static constexpr SkColorType colorType = kBGRA_8888_SkColorType;
	
	sk_sp<SkImage> getSkiaImage () const;
	
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

protected:
	SkImageInfo imageInfo;

	sk_sp<SkSurface> surface; // render target for drawing into
	SkBitmap bitmap; // raster data in RAM, read/write
	mutable sk_sp<SkImage> image; // texture in GFX memory, only draw

	AutoPtr<IBitmapDecoder> decoder;
	bool mustDecode;
	
	void decode ();
	SkSurface* getSurface ();
	void allocateBitmap ();
	void* getBits ();
};

//************************************************************************************************
// SkiaBitmapRenderBitmap
//************************************************************************************************

class SkiaBitmapRenderTarget: public Object,
							  public SkiaRenderTarget
{
public:
	SkiaBitmapRenderTarget (SkiaBitmap& nativeBitmap);
	~SkiaBitmapRenderTarget ();

	//SkiaRenderTarget
	SkCanvas* getCanvas ();
	float getContentScaleFactor () const;

protected:
	SkiaBitmap& bitmap;
};

} // namespace CCL

#endif // _ccl_skiabitmap_h
