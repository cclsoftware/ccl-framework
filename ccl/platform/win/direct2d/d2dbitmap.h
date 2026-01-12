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
// Filename    : ccl/platform/win/direct2d/d2dbitmap.h
// Description : Direct2D Bitmap
//
//************************************************************************************************

#ifndef _ccl_direct2d_bitmap_h
#define _ccl_direct2d_bitmap_h

#include "ccl/platform/win/direct2d/d2dbase.h"
#include "ccl/platform/win/interfaces/iwin32graphics.h"

#include "ccl/gui/graphics/nativegraphics.h"

#include <wincodec.h>

namespace CCL {
namespace Win32 {

//************************************************************************************************
// D2DBitmap
//************************************************************************************************

class D2DBitmap: public NativeBitmap,
				 public IWin32Bitmap,
				 private D2DResource
{
public:
	DECLARE_CLASS (D2DBitmap, NativeBitmap)

	D2DBitmap (IWICBitmapSource* bitmapSource = nullptr, bool alphaChannelUsed = true); ///< takes ownership!
	D2DBitmap (IBitmapDecoder* customDecoder, bool alphaChannelUsed = true); ///< takes ownership!
	D2DBitmap (PointRef sizeInPixel, bool alphaChannelUsed, float contentScaleFactor);
	~D2DBitmap ();

	static D2DBitmap* createFromExistingBitmap (ID2D1Bitmap1* d2dBitmap);

	IWICBitmapSource* createWICBitmapSource (); ///< called to save bitmap to stream and to create GDI HBITMAP

	// called by D2DBitmapRenderTarget:
	ID2D1Bitmap1* beginUpdate ();
	void endUpdate ();
	bool isAlphaChannelUsed () const;

	// NativeBitmap
	tresult draw (NativeGraphicsDevice& device, PointRef pos, const ImageMode* mode = nullptr) override;
	tresult draw (NativeGraphicsDevice& device, PointFRef pos, const ImageMode* mode = nullptr) override;
	tresult draw (NativeGraphicsDevice& device, RectRef src, RectRef dst, const ImageMode* mode = nullptr) override;
	tresult draw (NativeGraphicsDevice& device, RectFRef src, RectFRef dst, const ImageMode* mode = nullptr) override;
	tresult tile (NativeGraphicsDevice& device, int method, RectRef src, RectRef dest, RectRef clip, RectRef margins) override;
	PixelFormat CCL_API getPixelFormat () const override { return kRGBAlpha; }
	tresult CCL_API lockBits (BitmapLockData& data, PixelFormat format, int mode) override;
	tresult CCL_API unlockBits (BitmapLockData& data) override;
	tresult CCL_API scrollPixelRect (const Rect& rect, const Point& delta) override;

	CLASS_INTERFACE (IWin32Bitmap, NativeBitmap)

protected:
	DXGIEngine& engine;
	bool alphaChannelUsed;
	AutoPtr<IBitmapDecoder> customDecoder;
	ComPtr<IWICBitmapSource> encodedBitmap;
	ComPtr<IWICBitmap> wicBitmap;
	ComPtr<ID2D1Bitmap1> d2dBitmap;
	HBITMAP hCachedGdiBitmap;

	enum LastRenderOperation
	{
		kNoRendering,
		kSoftwareRendering,
		kDirect2DRendering,
		kBitmapsSynced
	};

	LastRenderOperation lastRenderOperation;

	ID2D1Bitmap1* getDirect2DBitmap (bool targetOptionEnabled = false);
	IWICBitmap* getWICBitmap ();
	void discardCachedGdiBitmap ();

	// IWin32Bitmap
	tbool CCL_API isAlphaPixelFormat () const override { return true; }
	HBITMAP CCL_API getHBITMAP () override;
	HBITMAP CCL_API detachHBITMAP () override;

	// D2DResource
	void discardDirect2dResource (bool isShutdown) override;
};

//************************************************************************************************
// D2DBitmapRenderTarget
//************************************************************************************************

class D2DBitmapRenderTarget: public Object,
							 public D2DRenderTarget
{
public:
	D2DBitmapRenderTarget (D2DBitmap& nativeBitmap);
	~D2DBitmapRenderTarget ();

	// D2DRenderTarget
	bool isAlphChannelUsed () const override;
	float getContentScaleFactor () const override;

protected:
	D2DBitmap& nativeBitmap;
};

} // namespace Win32
} // namespace CCL

#endif // _ccl_direct2d_bitmap_h
