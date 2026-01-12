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
// Filename    : ccl/platform/android/gui/nativegraphics.android.cpp
// Description : Android Native Graphics Engine
//
//************************************************************************************************

#include "ccl/platform/android/gui/frameworkview.h"
#include "ccl/platform/android/gui/graphicslayer.android.h"
#include "ccl/platform/android/gui/window.android.h"

#include "ccl/platform/android/graphics/android3dsupport.h"
#include "ccl/platform/android/graphics/androidbitmap.h"
#include "ccl/platform/android/graphics/androidgradient.h"
#include "ccl/platform/android/graphics/androidpath.h"
#include "ccl/platform/android/graphics/androidrendertarget.h"
#include "ccl/platform/android/graphics/androidtextlayout.h"

#include "ccl/public/systemservices.h"
#include "ccl/public/system/ifileutilities.h"

#define ENABLE_LAYERS 1

namespace CCL {

//************************************************************************************************
// AndroidGraphicsEngine
//************************************************************************************************

class AndroidGraphicsEngine: public NativeGraphicsEngine
{
public:
	bool startup () override;
	NativeWindowRenderTarget* createRenderTarget (Window* window) override;
	NativeGraphicsPath* createPath (IGraphicsPath::TypeHint type) override;
	NativeBitmap* createBitmap (int width, int height, IBitmap::PixelFormat pixelFormat, float contentScaleFactor = 1.f) override;
	NativeBitmap* loadBitmap (IStream& stream, const FileType& format) override;
	bool saveBitmap (IStream& stream, NativeBitmap& bitmap, const FileType& format, const IAttributeList* encoderOptions = nullptr) override;
	NativeGradient* createGradient (IGradient::TypeHint type) override;
	NativeGraphicsDevice* createWindowDevice (Window* window, void* systemDevice = nullptr) override;
	NativeGraphicsDevice* createBitmapDevice (NativeBitmap* bitmap) override;
	NativeBitmap* createScreenshotFromWindow (Window* window) override;
	ITextLayout* createTextLayout () override;
	bool hasGraphicsLayers () override;
	IGraphicsLayer* createGraphicsLayer (UIDRef classID) override;
	IFontTable* collectFonts (int flags) override;
	INative3DSupport* get3DSupport () override;
};

} // namespace CCL

using namespace CCL;
using namespace CCL::Android;

//************************************************************************************************
// AndroidGraphicsEngine
//************************************************************************************************

NativeGraphicsEngine& NativeGraphicsEngine::instance ()
{
	static AndroidGraphicsEngine theEngine;
	return theEngine;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool AndroidGraphicsEngine::startup ()
{
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

NativeWindowRenderTarget* AndroidGraphicsEngine::createRenderTarget (Window* window)
{
	ASSERT (window)
	return AndroidWindowRenderTarget::create (*window);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

NativeGraphicsPath* AndroidGraphicsEngine::createPath (IGraphicsPath::TypeHint type)
{
	return NEW AndroidGraphicsPath;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

NativeBitmap* AndroidGraphicsEngine::createBitmap (int width, int height, IBitmap::PixelFormat pixelFormat, float contentScaleFactor)
{
	PixelPoint sizeInPixel (Point (width, height), contentScaleFactor);
	AndroidBitmap* bitmap = gGraphicsFactory->createBitmap (sizeInPixel, pixelFormat == IBitmap::kRGBAlpha);
	#if DEBUG
	if(bitmap == nullptr)
		Debugger::printf ("Android bitmap allocation failed: %d x %d px\n", sizeInPixel.x, sizeInPixel.y);
	#endif
	if(bitmap)
		bitmap->setContentScaleFactor (contentScaleFactor);
	return bitmap;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

NativeBitmap* AndroidGraphicsEngine::loadBitmap (IStream& stream, const FileType& format)
{
	// create copy in memory for later decoding
	AutoPtr<IMemoryStream> memStream = System::GetFileUtilities ().createStreamCopyInMemory (stream);
	if(!memStream)
		return nullptr;

	// check for custom codec first...
	IBitmapCodec* customCodec = CustomBitmapCodecs::instance ().findCodec (format);
	if(customCodec)
	{
		IBitmapDecoder* customDecoder = customCodec->createBitmapDecoder (*memStream);
		if(customDecoder)
			return NEW AndroidBitmap (customDecoder);
	}
	else // load bitmap through FrameworkGraphicsFactory
		return gGraphicsFactory->loadBitmap (*memStream);

	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool AndroidGraphicsEngine::saveBitmap (IStream& stream, NativeBitmap& bitmap, const FileType& format, 
										const IAttributeList* encoderOptions)
{
	if(CustomBitmapCodecs::instance ().encodeBitmap (stream, bitmap, format, encoderOptions))
		return true;

	if(auto* androidBitmap = ccl_cast<Android::AndroidBitmap> (&bitmap))
		return gGraphicsFactory->saveBitmap (stream, *androidBitmap, format, encoderOptions);

	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

NativeGradient* AndroidGraphicsEngine::createGradient (IGradient::TypeHint type)
{
	switch(type)
	{
	case IGradient::kLinearGradient: return NEW AndroidLinearGradient;
	case IGradient::kRadialGradient: return NEW AndroidRadialGradient;
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

NativeGraphicsDevice* AndroidGraphicsEngine::createWindowDevice (Window* window, void* systemDevice)
{
	if(FrameworkView::isOffscreenEnabled ())
	{
		AndroidWindow* androidWindow = AndroidWindow::cast (window);
		if(FrameworkView* frameworkView = androidWindow ? androidWindow->getFrameworkView () : 0)
			return frameworkView->createOffscreenDevice ();
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

NativeGraphicsDevice* AndroidGraphicsEngine::createBitmapDevice (NativeBitmap* bitmap)
{
	if(AndroidBitmap* androidBitmap = ccl_cast<Android::AndroidBitmap> (bitmap))
		return gGraphicsFactory->createBitmapGraphics (*androidBitmap);

	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

NativeBitmap* AndroidGraphicsEngine::createScreenshotFromWindow (Window* window)
{
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ITextLayout* AndroidGraphicsEngine::createTextLayout ()
{
	return NEW AndroidTextLayout;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool AndroidGraphicsEngine::hasGraphicsLayers ()
{
#if ENABLE_LAYERS
	return true;
#else
	return false;
#endif
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IGraphicsLayer* AndroidGraphicsEngine::createGraphicsLayer (UIDRef classID)
{
#if ENABLE_LAYERS
	if(classID == ClassID::RootLayer)
		return NEW AndroidRootLayer;
	if(classID == ClassID::GraphicsLayer)
		return NEW AndroidGraphicsLayer;
	if(classID == ClassID::TiledLayer)
		return NEW AndroidGraphicsLayer; // todo: AndroidTiledLayer
#endif
	return 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IFontTable* AndroidGraphicsEngine::collectFonts (int flags)
{
	return gGraphicsFactory->collectFonts (flags);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

INative3DSupport* AndroidGraphicsEngine::get3DSupport ()
{
	return &Android3DSupport::instance ();
}
