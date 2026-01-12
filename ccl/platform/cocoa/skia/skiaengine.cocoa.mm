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
// Filename    : ccl/platform/cocoa/skia/skiaengine.cocoa.mm
// Description : Cocoa Skia Engine
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "ccl/platform/cocoa/skia/skiaengine.cocoa.h"
#include "ccl/platform/cocoa/skia/skiabitmap.cocoa.h"
#include "ccl/platform/cocoa/skia/skialayer.cocoa.h"
#include "ccl/platform/cocoa/gui/window.mac.h"
#include "ccl/platform/cocoa/metal/metal3dsupport.h"
#include "ccl/platform/cocoa/metal/metalclient.h"

#if CCL_PLATFORM_MAC
#include "ccl/platform/cocoa/gui/printjob.cocoa.h"
#else
#include "ccl/platform/cocoa/gui/printjob.ios.h"
#endif

#include "ccl/public/base/istream.h"
#include "ccl/public/system/ifileutilities.h"
#include "ccl/public/systemservices.h"

#include "ccl/base/storage/configuration.h"

#include "ccl/public/base/ccldefpush.h"
#include <Metal/Metal.h>
#include "ccl/public/base/ccldefpop.h"

using namespace CCL;
using namespace MacOS;

//************************************************************************************************
// CocoaSkiaEngine
//************************************************************************************************

const SkShaper& CocoaSkiaEngine::getShaper ()
{
	if(!shaper)
		shaper = SkShapers::CT::CoreText ();

	return *shaper;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

GrRecordingContext* CocoaSkiaEngine::getGPUContext ()
{
	if(!context)
	{
		GrMtlBackendContext backend = {};
		backend.fDevice.retain ((GrMTLHandle)MetalClient::instance ().getDevice ());
		backend.fQueue.retain ((GrMTLHandle)MetalClient::instance ().getQueue ());
		context = GrDirectContexts::MakeMetal (backend);
	}
	
	return context.get ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

NativeBitmap* CocoaSkiaEngine::createBitmap (int width, int height, IBitmap::PixelFormat pixelFormat, float contentScaleFactor)
{
	return NEW CocoaSkiaBitmap (PixelPoint (Point (width, height), contentScaleFactor), pixelFormat, contentScaleFactor);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

NativeBitmap* CocoaSkiaEngine::loadBitmap (IStream& stream, const FileType& format)
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
			return NEW CocoaSkiaBitmap (customDecoder);
	}
	else // use built-in codecs otherwise
	{
		CocoaSkiaBitmap* bitmap = NEW CocoaSkiaBitmap (memStream);
		if(bitmap->isValid ())
			return bitmap;
	
		bitmap->release ();
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool CocoaSkiaEngine::hasGraphicsLayers ()
{
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IGraphicsLayer* CocoaSkiaEngine::createGraphicsLayer (UIDRef classID)
{
	return CocoaSkiaLayerFactory::createLayer (classID);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

NativeBitmap* CocoaSkiaEngine::createScreenshotFromWindow (Window* window)
{
	#ifndef CCL_PLATFORM_IOS
	OSXWindow* osxWindow = OSXWindow::cast (window);
	ASSERT (osxWindow)
	return NEW CocoaSkiaBitmap (osxWindow->createScreenshotFromWindow ());
	#else
	return nullptr;
	#endif
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Object* CocoaSkiaEngine::createPrintJob ()
{
	#if CCL_PLATFORM_MAC
	return NEW MacOSSkiaPrintJob ();
	#else
	return NEW IOSSkiaPrintJob ();
	#endif
}

//////////////////////////////////////////////////////////////////////////////////////////////////

INative3DSupport* CocoaSkiaEngine::get3DSupport ()
{
	return &Metal3DSupport::instance ();
}

static Configuration::BoolValue skiaEnabled ("CCL.Skia", "Enabled", false);

//************************************************************************************************
// MetalGraphicsInfo
//************************************************************************************************

DEFINE_SINGLETON_CLASS (MetalGraphicsInfo, Object)
DEFINE_CLASS_UID (MetalGraphicsInfo, 0x6266d1a9, 0xaaf9, 0x48f6, 0x97, 0x6b, 0x42, 0xf8, 0xd0, 0xc4, 0x1e, 0x94)
DEFINE_SINGLETON (MetalGraphicsInfo)

bool MetalGraphicsInfo::isSkiaEnabled () const
{
	#if CCL_PLATFORM_IOS // work in progress
	return false;
	#else
	return skiaEnabled && MetalGraphicsInfo::instance ().isMetalAvailable () && MetalGraphicsInfo::instance ().isMetalEnabled ();
	#endif
}
