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
// Filename    : ccl/platform/cocoa/quartz/engine.mm
// Description : Quartz Engine
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "engine.h"

#include "ccl/platform/cocoa/macutils.h"
#include "ccl/platform/cocoa/cclcocoa.h"

#include "ccl/public/base/ccldefpush.h"

#include <CoreGraphics/CGBitmapContext.h>
#include <CoreGraphics/CGContext.h>
#include <CoreGraphics/CGDataConsumer.h>
#include <ImageIO/CGImageDestination.h>
#include <ImageIO/CGImageProperties.h>
#include "ccl/public/base/ccldefpop.h"

#if CCL_PLATFORM_IOS
	#include "ccl/platform/cocoa/gui/printjob.ios.h"
#else
	#include "ccl/platform/cocoa/gui/printjob.cocoa.h"
#endif

#include "ccl/platform/cocoa/quartz/device.h"
#include "ccl/platform/cocoa/quartz/quartzbitmap.h"
#include "ccl/platform/cocoa/quartz/path.h"
#include "ccl/platform/cocoa/quartz/gradient.h"
#include "ccl/platform/cocoa/quartz/quartzlayer.h"
#include "ccl/platform/cocoa/quartz/fontcache.h"
#include "ccl/platform/cocoa/quartz/quartztextlayout.h"

#include "ccl/platform/cocoa/metal/metal3dsupport.h"

#include "ccl/public/gui/graphics/dpiscale.h"
#include "ccl/public/storage/filetype.h"
#include "ccl/public/base/istream.h"
#include "ccl/public/system/ifileutilities.h"
#include "ccl/public/systemservices.h"

#include "ccl/gui/windows/window.h"

static const CFStringRef cclUTTypePNG = CFSTR ("public.png");
static const CFStringRef cclUTTypeJPEG = CFSTR ("public.jpeg");

using namespace CCL;
using namespace MacOS;

//************************************************************************************************
// QuartzEngine
//************************************************************************************************

bool QuartzEngine::startup ()
{
	//...
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void QuartzEngine::shutdown ()
{
	// cleanup objects first!
	NativeGraphicsEngine::shutdown ();
	//...
}

//////////////////////////////////////////////////////////////////////////////////////////////////

NativeWindowRenderTarget* QuartzEngine::createRenderTarget (Window* window)
{
	ASSERT (window != nullptr)
	#if CCL_PLATFORM_MAC
	return NEW QuartzOSXWindowRenderTarget (*window);
	#elif CCL_PLATFORM_IOS
	return NEW QuartzIOSWindowRenderTarget (*window);
	#endif
}


//////////////////////////////////////////////////////////////////////////////////////////////////

NativeGraphicsPath* QuartzEngine::createPath (IGraphicsPath::TypeHint type)
{
	return NEW QuartzPath;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

NativeGradient* QuartzEngine::createGradient (IGradient::TypeHint type)
{
	switch(type)
	{
	case IGradient::kLinearGradient : return NEW QuartzLinearGradient;
	case IGradient::kRadialGradient : return NEW QuartzRadialGradient;
	}
	return nullptr;	
}

//////////////////////////////////////////////////////////////////////////////////////////////////

NativeBitmap* QuartzEngine::createBitmap (int _width, int _height, IBitmap::PixelFormat pixelFormat, float contentScaleFactor)
{
    int width = ccl_max (_width, 1);
    int height = ccl_max (_height, 1);
	PixelPoint sizeInPixel (Point (width, height), contentScaleFactor);
	return NEW QuartzBitmap (sizeInPixel, pixelFormat, contentScaleFactor);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

NativeBitmap* QuartzEngine::createOffscreen (int width, int height, IBitmap::PixelFormat pixelFormat, bool global, Window* window)
{
	float contentScaleFactor = window ? window->getContentScaleFactor () : 1.f;
	return createBitmap (width, height, pixelFormat, contentScaleFactor);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

NativeBitmap* QuartzEngine::loadBitmap (IStream& stream, const FileType& format)
{
	// ensure that bitmap doesn't access original stream for delayed/on-demand decoding
	AutoPtr<IMemoryStream> memStream = System::GetFileUtilities ().createStreamCopyInMemory (stream);
	if(!memStream)
		return nullptr;
	
	IBitmapCodec* customCodec = CustomBitmapCodecs::instance ().findCodec (format);
	if(customCodec)
	{
		AutoPtr<IBitmapDecoder> customDecoder = customCodec->createBitmapDecoder (*memStream);
		if(customDecoder)
			return NEW QuartzBitmap (customDecoder.detach ());
	}
	else
	{
		QuartzBitmap* bitmap = NEW QuartzBitmap (memStream);
		if(bitmap->getCGImage ()) // success
			return bitmap;
		bitmap->release ();
	}

	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

static size_t ConsumerPutBytesCallback (void* stream, const void* buffer, size_t count)
{
	return ((IStream*)stream)->write (buffer, (int)count);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

static void ConsumerReleaseInfoCallback (void* stream) {}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool QuartzEngine::saveBitmap (IStream& stream, NativeBitmap& bitmap, const FileType& format, 
							   const IAttributeList* encoderOptions)
{
	// check for custom codec first
	if(CustomBitmapCodecs::instance ().encodeBitmap (stream, bitmap, format, encoderOptions))
		return true;

	QuartzBitmap* quartzBitmap = ccl_cast<QuartzBitmap> (&bitmap);
	ASSERT (quartzBitmap != nullptr)
	if(quartzBitmap == nullptr)
		return false;
	
	CFStringRef utiType = NULL;
	if(format.getMimeType () == "image/png")
		utiType = cclUTTypePNG;
	else if(format.getMimeType() == "image/jpeg")
		utiType = cclUTTypeJPEG;
	else
		return false;
	
	CGDataConsumerCallbacks consumerCallbacks = { ConsumerPutBytesCallback, ConsumerReleaseInfoCallback };
	CGDataConsumerRef consumerRef = CGDataConsumerCreate (&stream, &consumerCallbacks);
	
	CGImageDestinationRef destRef = CGImageDestinationCreateWithDataConsumer (consumerRef, utiType, 1, NULL);
	CFRelease (consumerRef);

	int dpi = 72;
	CFStringRef keys[2]   = { kCGImagePropertyDPIHeight, kCGImagePropertyDPIWidth };
	CFNumberRef values[2] = { CFNumberCreate (NULL, kCFNumberSInt32Type, &dpi), CFNumberCreate (NULL, kCFNumberSInt32Type, &dpi) };
	
	CFDictionaryRef dictRef = CFDictionaryCreate (NULL, (const void**)&keys, (const void**)&values, 1, &kCFCopyStringDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
	CFRelease (values[0]);
	CFRelease (values[1]);
	
	CGImageDestinationSetProperties (destRef, dictRef);
	
	CGImageDestinationAddImage (destRef, quartzBitmap->getCGImage (), dictRef);
	CFRelease (dictRef);
	CGImageDestinationFinalize (destRef);
	CFRelease (destRef);

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

NativeGraphicsDevice* QuartzEngine::createWindowDevice (Window* window, void* systemDevice)
{
	#if CCL_PLATFORM_MAC
	ASSERT (window != nullptr)
	QuartzOSXWindowRenderTarget* renderTarget = ccl_cast<QuartzOSXWindowRenderTarget> (window->getRenderTarget ());
	ASSERT (renderTarget != nullptr)
	if(renderTarget)
		return NEW QuartzScopedGraphicsDevice (*renderTarget, static_cast<Unknown&> (*renderTarget));
	#elif CCL_PLATFORM_IOS
	ASSERT (window != nullptr)
	QuartzIOSWindowRenderTarget* renderTarget = ccl_cast<QuartzIOSWindowRenderTarget> (window->getRenderTarget ());
	ASSERT (renderTarget != nullptr)
	if(renderTarget)
		return NEW QuartzScopedGraphicsDevice (*renderTarget, static_cast<Unknown&> (*renderTarget));
	#endif
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

NativeGraphicsDevice* QuartzEngine::createBitmapDevice (NativeBitmap* bitmap)
{
	if(QuartzBitmap* quartzBitmap = ccl_cast<QuartzBitmap> (bitmap))
	{
		AutoPtr<QuartzBitmapRenderTarget> renderTarget = NEW QuartzBitmapRenderTarget (*quartzBitmap);
		return NEW QuartzScopedGraphicsDevice (*renderTarget, static_cast<Unknown&> (*renderTarget));
	}
	
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool QuartzEngine::hasGraphicsLayers ()
{
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IGraphicsLayer* QuartzEngine::createGraphicsLayer (UIDRef classID)
{
	return CocoaQuartzLayerFactory::createLayer (classID);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ITextLayout* QuartzEngine::createTextLayout ()
{
	return NEW QuartzTextLayout ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

NativeBitmap* QuartzEngine::createScreenshotFromWindow (Window* window)
{
	#ifndef CCL_PLATFORM_IOS
	OSXWindow* osxWindow = OSXWindow::cast (window);
	ASSERT (osxWindow)

	return osxWindow->createScreenshotFromWindow ();
	#else
	return nullptr;
	#endif
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Object* QuartzEngine::createPrintJob ()
{
	#if CCL_PLATFORM_MAC
	return NEW MacOSQuartzPrintJob ();
	#else
	return NEW IOSQuartzPrintJob ();
	#endif
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IFontTable* QuartzEngine::collectFonts (int flags)
{
	AutoPtr<SimpleFontTable> result = NEW SimpleFontTable;

	auto addFamily = [&] (NSString* fontFamily)
	{
		String familyName;
		familyName.appendNativeString (fontFamily);
		
		if(!(flags & Font::kCollectAppFonts) && FontCache::instance ().isUserFont (familyName))
			return;
		
		AutoPtr<SimpleFontTable::FontFamily> resultFamily = NEW SimpleFontTable::FontFamily;
		resultFamily->name = familyName;
		NSDictionary* familyAttributes = @{ (id)kCTFontFamilyNameAttribute : fontFamily };
		CTFontDescriptorRef familyDescriptor = CTFontDescriptorCreateWithAttributes ((CFDictionaryRef)familyAttributes);
		NSArray* fontDescriptors = @[ (id)familyDescriptor ];
		CTFontCollectionRef familyCollection = CTFontCollectionCreateWithFontDescriptors ((CFArrayRef)fontDescriptors, 0);
		NSArray* familyMembers = [(NSArray*)CTFontCollectionCreateMatchingFontDescriptors (familyCollection) autorelease];
		CFRelease (familyDescriptor);
		CFRelease (familyCollection);
		for(id familyMemberDesc in familyMembers)
			if(CTFontRef font = CTFontCreateWithFontDescriptor ((CTFontDescriptorRef) familyMemberDesc, 0, 0))
			{
				NSDictionary* traits = [(NSDictionary*)CTFontCopyAttribute (font, kCTFontTraitsAttribute) autorelease];
				NSNumber* symbolic = [traits valueForKey:(NSString*)kCTFontSymbolicTrait];
				CTFontStylisticClass fontClass = [symbolic integerValue] & kCTFontTraitClassMask;
				if(!(flags & Font::kCollectSymbolicFonts) && fontClass == kCTFontClassSymbolic)
					continue;
				
				if(CFStringRef styleName = CTFontCopyName (font, kCTFontStyleNameKey))
				{
					String style;
					style.appendNativeString (styleName);
					resultFamily->styles.add (style);
					CFRelease (styleName);
					if(resultFamily->exampleText.isEmpty ())
						if(CFStringRef sampleText = CTFontCopyName (font, kCTFontSampleTextNameKey))
						{
							resultFamily->exampleText.appendNativeString (sampleText);
							CFRelease (sampleText);
						}
				}
				CFRelease (font);
			}
		if(resultFamily->styles.count () > 0)
			result->addFamilySorted (resultFamily.detach ());
	};
	
	NSArray* fontFamilies = [(NSArray*)CTFontManagerCopyAvailableFontFamilyNames () autorelease];
	for(NSString* fontFamily in fontFamilies)
	{
		if([fontFamily hasPrefix:@"."]) // these are usually hidden for UI
			continue;
		addFamily (fontFamily);
	}
		
	return result.detach ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

INative3DSupport* QuartzEngine::get3DSupport ()
{
	return &Metal3DSupport::instance ();
}

