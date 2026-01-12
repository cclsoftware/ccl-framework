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
// Filename    : ccl/platform/win/direct2d/d2dengine.cpp
// Description : Direct2D Engine
//
//************************************************************************************************

#include "ccl/platform/win/direct2d/d2dengine.h"
#include "ccl/platform/win/direct2d/d2dwindow.h"
#include "ccl/platform/win/direct2d/d2ddevice.h"
#include "ccl/platform/win/direct2d/d2dpath.h"
#include "ccl/platform/win/direct2d/d2dgradient.h"
#include "ccl/platform/win/direct2d/d2dbitmap.h"
#include "ccl/platform/win/direct2d/d2dprintjob.h"
#include "ccl/platform/win/direct2d/d2dtextlayout.h"
#include "ccl/platform/win/direct2d/wicbitmaphandler.h"

#include "ccl/platform/win/direct2d/dxgiengine.h"
#include "ccl/platform/win/direct2d/dwriteengine.h"
#include "ccl/platform/win/direct2d/dcompengine.h"

#include "ccl/platform/win/gui/win32graphics.h"

#include "ccl/gui/windows/window.h"

#include "ccl/base/signalsource.h"
#include "ccl/base/message.h"

#include "ccl/public/text/translation.h"
#include "ccl/public/system/ifileutilities.h"

#include "ccl/public/gui/graphics/dpiscale.h"
#include "ccl/public/gui/framework/ialert.h"
#include "ccl/public/gui/framework/controlsignals.h"

using namespace CCL;
using namespace Win32;

//////////////////////////////////////////////////////////////////////////////////////////////////
// Strings
//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_XSTRINGS ("Win32")
	XSTRING (Windows10OrLater, "This application requires Windows 10 or later.")
END_XSTRINGS

//************************************************************************************************
// Direct2DEngine
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (Direct2DEngine, NativeGraphicsEngine)

//////////////////////////////////////////////////////////////////////////////////////////////////

Direct2DEngine::Direct2DEngine ()
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Direct2DEngine::startupRequired ()
{	
	if(!DXGIEngine::instance ().startup ())
		return false;

	if(!DWriteEngine::instance ().startup ())
		return false;

	if(!DirectCompositionEngine::instance ().startup ())
		return false;

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Direct2DEngine::startup ()
{
	if(startupRequired () == false)
	{
		if(suppressErrors == false)
			Alert::warn (XSTR (Windows10OrLater));

		return false;
	}

	// force creation of WIC factory at early stage so that it stays alive
	// in case WIC bitmaps still exist after D2D engine shutdown
	WICBitmapHandler::instance ();
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Direct2DEngine::shutdown ()
{
	SuperClass::shutdown ();
	D3DSupport::shutdown3D ();

	DirectCompositionEngine::instance ().shutdown ();

	DWriteEngine::instance ().shutdown ();

	D2DResource::discardAll (true);

	DXGIEngine::instance ().shutdown ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Direct2DEngine::recoverFromError ()
{
	// Device loss affects D3D, D2D, DirectComposition, but not DirectWrite, etc.

	D3DSupport::handleError3D ();

	D2DResource::discardAll (false);

	DXGIEngine::instance ().handleDeviceLost ();

	DirectCompositionEngine::instance ().handleDeviceLost ();

	SignalSource (Signals::kGUI).signal (Message (Signals::kGraphicsEngineReset));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

NativeWindowRenderTarget* Direct2DEngine::createRenderTarget (Window* window)
{
	ASSERT (window != nullptr)
	return NEW D2DWindowRenderTarget (*window);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

NativeGraphicsPath* Direct2DEngine::createPath (IGraphicsPath::TypeHint type)
{
	return NEW D2DPathGeometry (type);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

NativeGradient* Direct2DEngine::createGradient (IGradient::TypeHint type)
{	
	switch(type)
	{
	case IGradient::kLinearGradient : return NEW D2DLinearGradient;
	case IGradient::kRadialGradient : return NEW D2DRadialGradient;
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

NativeBitmap* Direct2DEngine::createBitmap (int width, int height, IBitmap::PixelFormat pixelFormat, float contentScaleFactor)
{
	// ATTENTION: 24 bit RGB pixel format creates a 32 bit RGBA bitmap where the alpha channel is ignored!
	PixelPoint sizeInPixel (Point (width, height), contentScaleFactor);
	return NEW D2DBitmap (sizeInPixel, pixelFormat == IBitmap::kRGBAlpha, contentScaleFactor);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

NativeBitmap* Direct2DEngine::createOffscreen (int width, int height, IBitmap::PixelFormat pixelFormat, bool global, Window* window)
{
	return createBitmap (width, height, pixelFormat, window ? window->getContentScaleFactor () : 1.f);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

NativeBitmap* Direct2DEngine::loadBitmap (CCL::IStream& stream, const FileType& format)
{
	// create copy in memory for later decoding
	AutoPtr<IMemoryStream> memStream = System::GetFileUtilities ().createStreamCopyInMemory (stream);
	if(!memStream)
		return nullptr;

	// check for custom codec first...
	IBitmapCodec* customCodec = CustomBitmapCodecs::instance ().findCodec (format);
	if(customCodec)
	{
		AutoPtr<IBitmapDecoder> customDecoder = customCodec->createBitmapDecoder (*memStream);
		if(customDecoder)
			return NEW D2DBitmap (customDecoder.detach ());
	}	
	else // use built-in WIC codecs otherwise
	{
		ComPtr<IWICBitmapSource> bitmapSource = WICBitmapHandler::instance ().createSourceFromStream (*memStream);
		if(bitmapSource)
			return NEW D2DBitmap (bitmapSource.detach ());
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Direct2DEngine::saveBitmap (CCL::IStream& stream, NativeBitmap& bitmap, const FileType& format,
								 const IAttributeList* encoderOptions)
{
	// check for custom codec first...
	if(CustomBitmapCodecs::instance ().encodeBitmap (stream, bitmap, format, encoderOptions))
		return true;

	// use built-in WIC codecs otherwise
	D2DBitmap* d2dBitmap = ccl_cast<D2DBitmap> (&bitmap);
	ComPtr<IWICBitmapSource> bitmapSource = d2dBitmap->createWICBitmapSource (); 
	return WICBitmapHandler::instance ().saveToStream (stream, bitmapSource, format);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

NativeGraphicsDevice* Direct2DEngine::createWindowDevice (Window* window, void* systemDevice)
{
	ASSERT (window != nullptr)
	D2DWindowRenderTarget* renderTarget = ccl_cast<D2DWindowRenderTarget> (window->getRenderTarget ());
	ASSERT (renderTarget != nullptr)
	if(renderTarget)
	{
		if(renderTarget->hasOutputImage ()) // fails if swap chain doesn't exist, can happen when D3D device is lost
			return NEW D2DScopedGraphicsDevice (*renderTarget, /*static_cast<Unknown*> (renderTarget)*/nullptr);
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

NativeGraphicsDevice* Direct2DEngine::createBitmapDevice (NativeBitmap* bitmap)
{
	if(D2DBitmap* d2dBitmap = ccl_cast<D2DBitmap> (bitmap))
	{
		AutoPtr<D2DBitmapRenderTarget> renderTarget = NEW D2DBitmapRenderTarget (*d2dBitmap);
		if(renderTarget->hasOutputImage ()) // D2DBitmap::beginUpdate() might fail
			return NEW D2DScopedGraphicsDevice (*renderTarget, static_cast<Unknown*> (renderTarget));		
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

NativeBitmap* Direct2DEngine::createScreenshotFromWindow (Window* window)
{
	AutoPtr<D2DBitmap> d2dBitmap;

	#if 0 // use DXGI APIs
	if(D2DWindowRenderTarget* renderTarget = ccl_cast<D2DWindowRenderTarget> (window->getRenderTarget ()))
	{
		ComPtr<ID2D1Bitmap1> dxgiScreenshot = DXGIEngine::instance ().takeScreenshot ((HWND)window->getSystemWindow (), renderTarget->getSwapChain ());
		if(dxgiScreenshot)
			d2dBitmap = D2DBitmap::createFromExistingBitmap (dxgiScreenshot);
	}
	#endif

	if(d2dBitmap == nullptr)
	{		
		HBITMAP hBitmap = Win32::CreateScreenshotFromHWND ((HWND)window->getSystemWindow ());
		if(hBitmap)
		{
			if(IWICBitmap* wicBitmap = WICBitmapHandler::instance ().createBitmapFromHBITMAP (hBitmap))
				d2dBitmap = NEW D2DBitmap (wicBitmap); // takes ownership!
			::DeleteObject (hBitmap);
		}
	}

	if(d2dBitmap)
		d2dBitmap->setContentScaleFactor (window->getContentScaleFactor ());

	return d2dBitmap.detach ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ITextLayout* Direct2DEngine::createTextLayout ()
{
	return NEW D2DTextLayout;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Direct2DEngine::installFontFromMemory (const void* data, uint32 dataSize, StringRef name, int style)
{
	return DWriteEngine::instance ().installFontFromMemory (data, dataSize, name);	
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Direct2DEngine::beginFontInstallation (bool state)
{
	return DWriteEngine::instance ().beginFontInstallation (state);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Direct2DEngine::hasGraphicsLayers ()
{
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IGraphicsLayer* Direct2DEngine::createGraphicsLayer (UIDRef classID)
{
	return DirectCompositionEngine::instance ().createLayer (classID);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Object* Direct2DEngine::createPrintJob ()
{
	return NEW D2DPrintJob ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IFontTable* Direct2DEngine::collectFonts (int flags)
{
	return DWriteEngine::instance ().collectFonts (flags); 
}
