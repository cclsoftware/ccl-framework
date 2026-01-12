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
// Filename    : ccl/platform/win/direct2d/d2dbitmap.cpp
// Description : Direct2D Bitmap
//
//************************************************************************************************

#include "ccl/platform/win/direct2d/d2dbitmap.h"
#include "ccl/platform/win/direct2d/d2ddevice.h"
#include "ccl/platform/win/direct2d/dxgiengine.h"
#include "ccl/platform/win/direct2d/wicbitmaphandler.h"

#include "ccl/gui/graphics/imaging/tiler.h"

#include "ccl/public/gui/graphics/dpiscale.h"

#include "ccl/public/text/cstring.h"

namespace CCL {
namespace Win32 {

//************************************************************************************************
// D2DBlitter
//************************************************************************************************

class D2DBlitter: public Blitter
{
public:
	D2DBlitter (D2DRenderTarget& target, ID2D1Bitmap* d2dBitmap)
	: target (target),
	  d2dBitmap (d2dBitmap)
	{}

	// Blitter
	void blit (RectRef src, RectRef dst) override
	{
		target->DrawBitmap (d2dBitmap, D2DInterop::toRectF (dst), 1.f, D2D1_BITMAP_INTERPOLATION_MODE_LINEAR, D2DInterop::toRectF (src));
	}

	D2DRenderTarget& target;
	ID2D1Bitmap* d2dBitmap;
};

//************************************************************************************************
// D2DBitmapSource
//************************************************************************************************

class D2DBitmapSource: public Object,
					   public IWICBitmapSource
{
public:
	D2DBitmapSource (ID2D1Bitmap1* bitmap)
	: bitmap (bitmap)
	{
		ASSERT (bitmap != nullptr)
		bitmap->AddRef ();
	}

	// IUnknown
	DELEGATE_COM_IUNKNOWN
	tresult CCL_API queryInterface (UIDRef iid, void** ptr) override
	{
		QUERY_COM_INTERFACE (IWICBitmapSource)
		return Object::queryInterface (iid, ptr);
	}

	// IWICBitmapSource
	HRESULT STDMETHODCALLTYPE GetSize (UINT *puiWidth, UINT *puiHeight) override
	{
		D2D1_SIZE_U size = bitmap->GetPixelSize ();
		*puiWidth = size.width;
		*puiHeight = size.height;
		return S_OK;
	}

	HRESULT STDMETHODCALLTYPE GetPixelFormat (WICPixelFormatGUID *pPixelFormat) override
	{
		*pPixelFormat = GUID_WICPixelFormat32bppPBGRA;
		return S_OK;
	}

	HRESULT STDMETHODCALLTYPE GetResolution (double *pDpiX, double *pDpiY) override
	{
		FLOAT dpiX = 0.f, dpiY = 0.f;
		bitmap->GetDpi (&dpiX, &dpiY);
		*pDpiX = dpiX;
		*pDpiY = dpiY;
		return S_OK;
	}

	HRESULT STDMETHODCALLTYPE CopyPalette (IWICPalette *pIPalette) override
	{
		return E_NOTIMPL;
	}

	HRESULT STDMETHODCALLTYPE CopyPixels (const WICRect *prc, UINT cbStride, UINT cbBufferSize, BYTE *pbBuffer) override
	{
		D2D1_SIZE_U size = bitmap->GetPixelSize ();
		WICRect srcRect = {0, 0, (INT)size.width, (INT)size.height};
		if(prc)
		{
			bool invalid = prc->X + prc->Width > size.width || prc->Y + prc->Height > size.height;
			ASSERT (invalid == false)
			if(invalid)
				return E_INVALIDARG;

			srcRect = *prc;
		}

		D2D1_MAPPED_RECT mappedRect = {0};
		HRESULT hr = bitmap->Map (D2D1_MAP_OPTIONS_READ, &mappedRect);
		if(FAILED (hr))
			return hr;

		UINT bytesPerPixel = 4;
		UINT32 bytesToCopy = srcRect.Width * bytesPerPixel;
		for(int y = 0; y < srcRect.Height; y++)
		{
			BYTE* srcLine = mappedRect.bits + (y + srcRect.Y) * mappedRect.pitch;
			BYTE* dstLine = pbBuffer + y * cbStride;

			srcLine += srcRect.X * bytesPerPixel;
			::memcpy (dstLine, srcLine, bytesToCopy);
		}

		hr = bitmap->Unmap ();
		ASSERT (SUCCEEDED (hr))
		return S_OK;
	}

protected:
	ComPtr<ID2D1Bitmap1> bitmap;
};

} // namespace Win32
} // namespace CCL

using namespace CCL;
using namespace Win32;

//************************************************************************************************
// D2DBitmap
//************************************************************************************************

D2DBitmap* D2DBitmap::createFromExistingBitmap (ID2D1Bitmap1* d2dBitmap)
{
	if(ComPtr<ID2D1Bitmap1> softwareBitmap = DXGIEngine::instance ().createBitmapForCPUReadAccess (d2dBitmap))
	{
		IWICBitmapSource* bitmapSource = NEW D2DBitmapSource (softwareBitmap);
		bool alphaChannelUsed = softwareBitmap->GetPixelFormat ().alphaMode != D2D1_ALPHA_MODE_IGNORE;
		return NEW D2DBitmap (bitmapSource, alphaChannelUsed);
	}
	else
		return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_CLASS_HIDDEN (D2DBitmap, NativeBitmap)

//////////////////////////////////////////////////////////////////////////////////////////////////

D2DBitmap::D2DBitmap (IWICBitmapSource* bitmapSource, bool alphaChannelUsed)
: NativeBitmap (Point (0, 0)),
  engine (DXGIEngine::instance ()),
  alphaChannelUsed (alphaChannelUsed),
  encodedBitmap (bitmapSource),
  hCachedGdiBitmap (nullptr),
  lastRenderOperation (kSoftwareRendering)
{
	ASSERT (encodedBitmap != 0)

	UINT width = 0, height = 0;
	HRESULT hr = encodedBitmap->GetSize (&width, &height);
	ASSERT (SUCCEEDED (hr))
	this->sizeInPixel ((int)width, (int)height);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

D2DBitmap::D2DBitmap (IBitmapDecoder* customDecoder, bool alphaChannelUsed)
: NativeBitmap (Point (0, 0)),
  engine (DXGIEngine::instance ()),
  alphaChannelUsed (alphaChannelUsed),
  customDecoder (customDecoder),
  hCachedGdiBitmap (nullptr),
  lastRenderOperation (kSoftwareRendering)
{
	ASSERT (customDecoder != nullptr)

	tresult result = customDecoder->getPixelSize (sizeInPixel);
	ASSERT (result == kResultOk)
}

//////////////////////////////////////////////////////////////////////////////////////////////////

D2DBitmap::D2DBitmap (PointRef sizeInPixel, bool alphaChannelUsed, float contentScaleFactor)
: NativeBitmap (sizeInPixel, contentScaleFactor),
  engine (DXGIEngine::instance ()),
  alphaChannelUsed (alphaChannelUsed),
  hCachedGdiBitmap (nullptr),
  lastRenderOperation (kNoRendering)
{
	// PLEASE NOTE: Bitmap allocation is postponed to a later stage. Depending on how this instance
	// is being used, either a WIC bitmap is created for CPU write access inside lockBits(),
	// or a Direc2D bitmap is created for offscreen rendering when beginUpdate() is called.
}

//////////////////////////////////////////////////////////////////////////////////////////////////

D2DBitmap::~D2DBitmap ()
{
#if DEBUG
	if(wicBitmap.isValid ())
	{
		ASSERT (WICBitmapHandler::peekInstance ())		
	}
#endif	
	discardCachedGdiBitmap ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void D2DBitmap::discardDirect2dResource (bool isShutdown)
{
	if(isShutdown)
	{
		wicBitmap.release ();
		discardCachedGdiBitmap ();
	}

	if(d2dBitmap)
	{
		d2dBitmap.release ();

		if(isShutdown == false)
		{
			// Reset state to software rendering during error recovery.
			// Bitmap content created via D2D is lost, though.
			if(lastRenderOperation == kBitmapsSynced && wicBitmap)
				lastRenderOperation = kSoftwareRendering;
		}
	}

	if(wicBitmap.isValid () == false && d2dBitmap.isValid () == false)
		D2DResource::setRegistered (false);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IWICBitmapSource* D2DBitmap::createWICBitmapSource ()
{
	if(lastRenderOperation == kDirect2DRendering)
	{
		ComPtr<ID2D1Bitmap1> softwareBitmap = engine.createBitmapForCPUReadAccess (getDirect2DBitmap ());
		if(softwareBitmap)
			return NEW D2DBitmapSource (softwareBitmap);
	}

	IWICBitmapSource* bitmapSource = encodedBitmap;
	if(bitmapSource == nullptr)
		bitmapSource = getWICBitmap ();

	ASSERT (bitmapSource)
	if(bitmapSource)
		bitmapSource->AddRef ();
	return bitmapSource;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ID2D1Bitmap1* D2DBitmap::beginUpdate ()
{
	return getDirect2DBitmap (true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void D2DBitmap::endUpdate ()
{
	lastRenderOperation = kDirect2DRendering;
	discardCachedGdiBitmap ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool D2DBitmap::isAlphaChannelUsed () const
{
	return alphaChannelUsed;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ID2D1Bitmap1* D2DBitmap::getDirect2DBitmap (bool targetOptionEnabled)
{
	if(d2dBitmap && targetOptionEnabled == true)
	{
		bool hasTargetOption = (d2dBitmap->GetOptions () & D2D1_BITMAP_OPTIONS_TARGET) != 0;
		if(hasTargetOption == false)
		{
			CCL_DEBUGGER ("Recreating Direct2D bitmap with target option enabled!!!\n")
			d2dBitmap.release ();
		}
	}

	if(!d2dBitmap)
	{
		// Hmm... to clear the bitmap we have to enable the target option
		if(lastRenderOperation != kSoftwareRendering)
			targetOptionEnabled = true;

		d2dBitmap = engine.createBitmap (sizeInPixel, alphaChannelUsed, targetOptionEnabled, getContentScaleFactor ());
		if(d2dBitmap)
		{
			D2DResource::setRegistered (true); // register for cleanup on shutdown or error handling

			if(lastRenderOperation == kSoftwareRendering)
			{
				engine.copyFromWICBitmap (d2dBitmap, getWICBitmap ());
				lastRenderOperation = kBitmapsSynced;
			}
			else
				engine.clearBitmap (d2dBitmap);
		}
		else
			engine.reportError (MutableCString ().appendFormat ("Create bitmap failed (%d x %d px)",
								sizeInPixel.x, sizeInPixel.y), engine.getLastError (), true);
	}
	else
	{
		if(lastRenderOperation == kSoftwareRendering)
		{
			engine.copyFromWICBitmap (d2dBitmap, getWICBitmap ());
			lastRenderOperation = kBitmapsSynced;
		}
	}

	return d2dBitmap;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IWICBitmap* D2DBitmap::getWICBitmap ()
{	
	// create or decode bitmap on demand
	if(!wicBitmap)
	{
		if(encodedBitmap)
		{
			wicBitmap = WICBitmapHandler::instance ().createBitmapFromSource (encodedBitmap);
			if(!wicBitmap)
				engine.reportError ("Create bitmap from encoded bitmap failed", 0, true);
			encodedBitmap.release ();
		}
		else
		{
			wicBitmap = WICBitmapHandler::instance ().createBitmap (sizeInPixel.x, sizeInPixel.y);
			if(!wicBitmap)
				engine.reportError (MutableCString ().appendFormat ("Create bitmap of %ix%ipx failed", sizeInPixel.x, sizeInPixel.y), 0, true);
		}

		ASSERT (wicBitmap)
		if(wicBitmap)
		{
			D2DResource::setRegistered (true); // register for cleanup on shutdown or error handling

			// decode with custom decoder
			if(customDecoder)
			{
				BitmapLockData data;
				if(WICBitmapHandler::instance ().lockBitmap (data, wicBitmap, kLockWrite))
				{
					tresult result = customDecoder->getPixelData (data);
					ASSERT (result == kResultOk)
					WICBitmapHandler::instance ().unlockBitmap (data);
				}
				customDecoder.release ();
			}
		}
	}

	if(lastRenderOperation == kDirect2DRendering)
	{
		if(wicBitmap)
		{
			if(ComPtr<ID2D1Bitmap1> softwareBitmap = engine.createBitmapForCPUReadAccess (d2dBitmap))
			{
				D2DBitmapSource bitmapSource (softwareBitmap);
				WICBitmapHandler::instance ().copyBitmap (wicBitmap, &bitmapSource);
			}
			else
				engine.reportError ("Create bitmap for CPU read access failed", engine.getLastError (), true);
		}

		lastRenderOperation = kBitmapsSynced;
	}

	return wicBitmap;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult D2DBitmap::draw (NativeGraphicsDevice& device, PointRef pos, const ImageMode* mode)
{
	Rect src (0, 0, getWidth (), getHeight ());
	Rect dst (src);
	dst.offset (pos);
	return draw (device, src, dst, mode);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult D2DBitmap::draw (NativeGraphicsDevice& device, PointFRef pos, const ImageMode* mode)
{
	RectF src (0, 0, (CoordF)getWidth (), (CoordF)getHeight ());
	RectF dst (src);
	dst.offset (pos);
	return draw (device, src, dst, mode);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult D2DBitmap::draw (NativeGraphicsDevice& device, RectRef src, RectRef dst, const ImageMode* mode)
{
	return draw (device, rectIntToF (src), rectIntToF (dst), mode);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult D2DBitmap::draw (NativeGraphicsDevice& device, RectFRef src, RectFRef dst, const ImageMode* mode)
{
	D2DGraphicsDevice* d2dDevice = ccl_cast<D2DGraphicsDevice> (&device);
	if(!d2dDevice)
		return kResultUnexpected;

	D2DRenderTarget& target = d2dDevice->getTarget ();
	if(target.isValid () == false)
		return kResultUnexpected;

	ID2D1Bitmap* d2dBitmap = getDirect2DBitmap ();
	ASSERT (d2dBitmap != nullptr)
	if(d2dBitmap == nullptr)
		return kResultFailed;

	FLOAT opacity = 1.f;
	D2D1_INTERPOLATION_MODE interpolationMode = D2D1_INTERPOLATION_MODE_LINEAR;
	if(mode != nullptr)
	{
		opacity = mode->getAlphaF ();
		if(mode->getInterpolationMode () == ImageMode::kInterpolationHighQuality)
			interpolationMode = D2D1_INTERPOLATION_MODE_HIGH_QUALITY_CUBIC;	
		
		else if(mode->getInterpolationMode () == ImageMode::kInterpolationPixelQuality)
			interpolationMode = D2D1_INTERPOLATION_MODE_NEAREST_NEIGHBOR;
	}

	target.getContext ()->DrawBitmap (d2dBitmap, D2DInterop::fromCCL (dst), opacity, interpolationMode, D2DInterop::fromCCL (src));
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult D2DBitmap::tile (NativeGraphicsDevice& device, int method, RectRef src, RectRef dst, RectRef clip, RectRef margins)
{
	D2DGraphicsDevice* d2dDevice = ccl_cast<D2DGraphicsDevice> (&device);
	if(!d2dDevice)
		return kResultUnexpected;

	D2DRenderTarget& target = d2dDevice->getTarget ();
	if(target.isValid () == false)
		return kResultUnexpected;

	ID2D1Bitmap* d2dBitmap = getDirect2DBitmap ();
	ASSERT (d2dBitmap != nullptr)
	if(d2dBitmap == nullptr)
		return kResultFailed;

	D2DBlitter blitter (target, d2dBitmap);
	Tiler::tile (blitter, method, src, dst, clip, margins);
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API D2DBitmap::lockBits (BitmapLockData& data, PixelFormat format, int mode)
{
	ASSERT (format == kAny || format == kRGBAlpha)
	if(!(format == kAny || format == kRGBAlpha))
		return kResultInvalidArgument;

	bool done = false;

	//if(lastRenderOperation == kDirect2DRendering && mode == kLockRead)
	//	done = engine.lockBitmap (data, d2dBitmap, mode);
	//else
		done = WICBitmapHandler::instance ().lockBitmap (data, getWICBitmap (), mode);

	 return done ? kResultOk : kResultFailed;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API D2DBitmap::unlockBits (BitmapLockData& data)
{
	bool done = false;

	//if(lastRenderOperation == kDirect2DRendering && mode == kLockRead)
	//	done = engine.unlockBitmap (data);
	//else
		done = WICBitmapHandler::instance ().unlockBitmap (data);

	if(data.mode == kLockWrite)
	{
		lastRenderOperation = kSoftwareRendering;
		discardCachedGdiBitmap ();
	}

	return done ? kResultOk : kResultFailed;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API D2DBitmap::scrollPixelRect (const Rect& rect, const Point& delta)
{
	bool done = false;

	if(lastRenderOperation == kDirect2DRendering)
	{
		done = engine.scrollBitmap (d2dBitmap, rect, delta);
		if(done == false)
			engine.reportError ("Scroll bitmap failed", engine.getLastError (), true);
	}
	else
	{
		done = WICBitmapHandler::instance ().scrollBitmap (getWICBitmap (), rect, delta);
		if(done == true)
		{
			lastRenderOperation = kSoftwareRendering;
			discardCachedGdiBitmap ();
		}
	}

	return done ? kResultOk : kResultFailed;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

HBITMAP CCL_API D2DBitmap::getHBITMAP ()
{
	if(hCachedGdiBitmap == nullptr)
	{
		ComPtr<IWICBitmapSource> bitmapSource = createWICBitmapSource ();
		HRESULT hr = WICBitmapHandler::instance ().createDIBSectionFromBitmapSource (hCachedGdiBitmap, bitmapSource);
		ASSERT (SUCCEEDED (hr))
	}
	return hCachedGdiBitmap;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

HBITMAP CCL_API D2DBitmap::detachHBITMAP ()
{
	HBITMAP result = getHBITMAP ();
	hCachedGdiBitmap = nullptr;
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void D2DBitmap::discardCachedGdiBitmap ()
{
	if(hCachedGdiBitmap)
	{
		::DeleteObject (hCachedGdiBitmap);
		hCachedGdiBitmap = nullptr;
	}
}

//************************************************************************************************
// D2DBitmapRenderTarget
//************************************************************************************************

D2DBitmapRenderTarget::D2DBitmapRenderTarget (D2DBitmap& nativeBitmap)
: nativeBitmap (nativeBitmap)
{
	nativeBitmap.retain ();
	outputImage.share (nativeBitmap.beginUpdate ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

D2DBitmapRenderTarget::~D2DBitmapRenderTarget ()
{
	nativeBitmap.endUpdate ();
	nativeBitmap.release ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool D2DBitmapRenderTarget::isAlphChannelUsed () const
{
	return nativeBitmap.isAlphaChannelUsed ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

float D2DBitmapRenderTarget::getContentScaleFactor () const
{
	return nativeBitmap.getContentScaleFactor ();
}
