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
// Filename    : ccl/platform/cocoa/skia/skiabitmap.cocoa.mm
// Description : Cocoa Skia Bitmap
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "ccl/platform/cocoa/skia/skiabitmap.cocoa.h"

#include "ccl/platform/cocoa/quartz/quartzbitmap.h"
#include "ccl/platform/cocoa/quartz/cgdataprovider.h"
#include "ccl/platform/cocoa/macutils.h"
  
using namespace CCL;

//************************************************************************************************
// CocoaSkiaBitmap
//************************************************************************************************

DEFINE_CLASS_HIDDEN (CocoaSkiaBitmap, SkiaBitmap)

CocoaSkiaBitmap::CocoaSkiaBitmap (PointRef sizeInPixel, PixelFormat format, float contentScaleFactor)
: SkiaBitmap (sizeInPixel, format, contentScaleFactor)
{
	valid = sizeInPixel.x > 0 && sizeInPixel.y > 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CocoaSkiaBitmap::CocoaSkiaBitmap (IBitmapDecoder* customDecoder, bool alphaChannelUsed)
: SkiaBitmap (customDecoder, alphaChannelUsed)
{
	valid = sizeInPixel.x > 0 && sizeInPixel.y > 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CocoaSkiaBitmap::CocoaSkiaBitmap (QuartzBitmap* _quartzBitmap)
: valid (false)
{
	initWithQuartzBitmap (_quartzBitmap);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CocoaSkiaBitmap::CocoaSkiaBitmap (IMemoryStream* stream)
: SkiaBitmap (stream),
  valid (false)
{
	valid = sizeInPixel.x > 0 && sizeInPixel.y > 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CocoaSkiaBitmap::initWithQuartzBitmap (QuartzBitmap* quartzBitmap)
{
	if(!quartzBitmap)
		return;

	CGImageRef image = quartzBitmap->getCGImage ();
	sizeInPixel.x = (int)CGImageGetWidth (image);
	sizeInPixel.y = (int)CGImageGetHeight (image);
    if(sizeInPixel.x <= 0 || sizeInPixel.y <= 0)
        return;

	imageInfo = SkImageInfo::Make (sizeInPixel.x, sizeInPixel.y, colorType, kOpaque_SkAlphaType);
	contentScaleFactor = quartzBitmap->getContentScaleFactor ();
	IBitmap::PixelFormat pixelFormat = IBitmap::kRGBAlpha;

	BitmapLockData source;
	quartzBitmap->lockBits (source, pixelFormat, IBitmap::kLockRead);
	int bitsPerComponent = 8;
	int bytesPerPixel = 4;
	int bytesPerRow = bytesPerPixel * sizeInPixel.x;
	CFObj<CGColorSpaceRef> colorSpace = CGColorSpaceCreateDeviceRGB ();
	CGContextRef context = CGBitmapContextCreate (getBits (), sizeInPixel.x,  sizeInPixel.y, bitsPerComponent, bytesPerRow, colorSpace, kCGImageAlphaPremultipliedFirst|kCGBitmapByteOrder32Host);
	CGContextDrawImage (context, CGRectMake (0, 0, static_cast<CGFloat>(sizeInPixel.x), static_cast<CGFloat>(sizeInPixel.y)), image);
	unlockBits (source);
	valid = true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CGImageRef CocoaSkiaBitmap::getCGImage ()
{
	if(!cachedBitmap)
	{
		IBitmap::PixelFormat pixelFormat = IBitmap::kRGBAlpha;
		cachedBitmap = NEW QuartzBitmap (sizeInPixel, pixelFormat, getContentScaleFactor ());

		BitmapLockData destination;
		BitmapLockData source;
		lockBits (source, pixelFormat, IBitmap::kLockRead);
		cachedBitmap->lockBits (destination, pixelFormat, IBitmap::kLockWrite);
		memcpy (destination.scan0, source.scan0, destination.height * destination.rowBytes);
		cachedBitmap->unlockBits (destination);
		unlockBits (source);
	}
	
	return cachedBitmap->getCGImage ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API CocoaSkiaBitmap::unlockBits (BitmapLockData& data)
{
	if(data.mode == IBitmap::kLockWrite)
		cachedBitmap = nullptr;
		
	return SkiaBitmap::unlockBits (data);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API CocoaSkiaBitmap::scrollPixelRect (const Rect& rect, const Point& delta)
{
	cachedBitmap = nullptr;
	
	return SkiaBitmap::scrollPixelRect (rect, delta);
}
