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
// Filename    : ccl/platform/shared/skia/skiaengine.cpp
// Description : Skia Engine
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "ccl/platform/shared/skia/skiaengine.h"

#include "ccl/platform/shared/skia/skiaglue.h"

#include "ccl/platform/shared/skia/skiarendertarget.h"
#include "ccl/platform/shared/skia/skiadevice.h"
#include "ccl/platform/shared/skia/skiabitmap.h"
#include "ccl/platform/shared/skia/skiapath.h"
#include "ccl/platform/shared/skia/skiagradient.h"
#include "ccl/platform/shared/skia/skiatextlayout.h"
#include "ccl/platform/shared/skia/skiafonttable.h"

#include "ccl/gui/graphics/nativegraphics.h"
#include "ccl/gui/windows/window.h"

#include "ccl/public/gui/graphics/dpiscale.h"
#include "ccl/public/storage/filetype.h"
#include "ccl/public/base/istream.h"
#include "ccl/public/system/ifileutilities.h"
#include "ccl/public/systemservices.h"

using namespace CCL;

//************************************************************************************************
// SkiaEngine
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (SkiaEngine, NativeGraphicsEngine)

//////////////////////////////////////////////////////////////////////////////////////////////////

SkiaEngine* SkiaEngine::getInstance ()
{
	return ccl_cast<SkiaEngine> (&NativeGraphicsEngine::instance ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

NativeGraphicsDevice* SkiaEngine::createdScopedDevice (SkiaRenderTarget* target, IUnknown& targetUnknown)
{
	if(!target)
		return nullptr;
		
	return NEW SkiaScopedGraphicsDevice (*target, targetUnknown);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const SkShaper& SkiaEngine::getShaper ()
{
	if(!shaper)
		shaper = SkShaper::Make ();

	return *shaper;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

GrRecordingContext* SkiaEngine::getGPUContext ()
{
	// to be overridden by platform implementations
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SkiaEngine::startup ()
{
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

NativeWindowRenderTarget* SkiaEngine::createRenderTarget (Window* window)
{
	ASSERT (window != nullptr)
	return SkiaWindowRenderTarget::create (*window);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

NativeGraphicsPath* SkiaEngine::createPath (IGraphicsPath::TypeHint type)
{
	return NEW SkiaPath ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

NativeGradient* SkiaEngine::createGradient (IGradient::TypeHint type)
{
	switch(type)
	{
	case IGradient::kLinearGradient : return NEW SkiaLinearGradient;
	case IGradient::kRadialGradient : return NEW SkiaRadialGradient;
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

NativeBitmap* SkiaEngine::createBitmap (int width, int height, IBitmap::PixelFormat pixelFormat, float contentScaleFactor)
{
	return NEW SkiaBitmap (PixelPoint (Point (width, height), contentScaleFactor), pixelFormat, contentScaleFactor);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

NativeBitmap* SkiaEngine::loadBitmap (IStream& stream, const FileType& format)
{
	// ensure that bitmap doesn't access original stream for delayed/on-demand decoding
	AutoPtr<IMemoryStream> memStream = System::GetFileUtilities ().createStreamCopyInMemory (stream);
	if(!memStream)
		return nullptr;
	
	// check for custom codec first...
	IBitmapCodec* customCodec = CustomBitmapCodecs::instance ().findCodec (format);
	if(customCodec)
	{
		IBitmapDecoder* customDecoder = customCodec->createBitmapDecoder (*memStream);
		if(customDecoder)
			return NEW SkiaBitmap (customDecoder);
	}
	else // use built-in codecs otherwise
	{
		return NEW SkiaBitmap (memStream);
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SkiaEngine::saveBitmap (IStream& stream, NativeBitmap& bitmap, const FileType& format,
							 const IAttributeList* encoderOptions)
{
	// check for custom codec first
	if(CustomBitmapCodecs::instance ().encodeBitmap (stream, bitmap, format, encoderOptions))
		return true;

	SkiaBitmap* skiaBitmap = ccl_cast<SkiaBitmap> (&bitmap);
	ASSERT (skiaBitmap != nullptr)
	if(skiaBitmap == nullptr)
		return false;
	
	return skiaBitmap->saveTo (stream, format);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

NativeGraphicsDevice* SkiaEngine::createWindowDevice (Window* window, void* systemDevice)
{
	if(window)
	{
		SkiaWindowRenderTarget* renderTarget = ccl_cast<SkiaWindowRenderTarget> (window->getRenderTarget ());
		ASSERT(renderTarget)
		if(renderTarget)
			return createdScopedDevice (renderTarget, static_cast<Unknown&> (*renderTarget));
	}

	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

NativeGraphicsDevice* SkiaEngine::createBitmapDevice (NativeBitmap* bitmap)
{
	if(SkiaBitmap* skiaBitmap = ccl_cast<SkiaBitmap> (bitmap))
	{
		AutoPtr<SkiaBitmapRenderTarget> renderTarget = NEW SkiaBitmapRenderTarget (*skiaBitmap);
		ASSERT(renderTarget)
		return createdScopedDevice (renderTarget, static_cast<Unknown&> (*renderTarget));
	}
	
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

NativeBitmap* SkiaEngine::createScreenshotFromWindow (Window* window)
{
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ITextLayout* SkiaEngine::createTextLayout ()
{
	return NEW SkiaTextLayout;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SkiaEngine::hasGraphicsLayers ()
{
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IGraphicsLayer* SkiaEngine::createGraphicsLayer (UIDRef classID)
{
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IFontTable* SkiaEngine::collectFonts (int flags)
{
	return NEW SkiaFontTable (flags);
}
