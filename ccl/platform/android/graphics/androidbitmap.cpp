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
// Filename    : ccl/platform/android/graphics/androidbitmap.cpp
// Description : Android Bitmap
//
//************************************************************************************************

#include "androidbitmap.h"
#include "frameworkgraphics.h"

#include "ccl/gui/graphics/imaging/tiler.h"
#include "ccl/public/gui/graphics/dpiscale.h"

#include "core/gui/corebitmapprimitives.h"

#include <android/bitmap.h>

using namespace CCL;
using namespace CCL::Android;

//************************************************************************************************
// JavaBitmap
//************************************************************************************************

JavaBitmap::JavaBitmap (JNIEnv* jni, jobject object)
: JniObject (jni, object)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

JavaBitmap::JavaBitmap ()
: JniObject ()
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult JavaBitmap::draw (FrameworkGraphics& androidDevice, PointFRef pos, const ImageMode* mode, float scaleFactor)
{
	// scale bitmap pixels to coords (coords are then scaled to graphics pixel by an outer transform)
	FrameworkGraphics::ScaleHelper scaleHelper (&androidDevice, scaleFactor, pos);

	jobject paint = gGraphicsFactory->getCachedBitmapPaint (mode);
	FrameworkGraphicsClass.drawBitmap (androidDevice, object, pos.x, pos.y, paint);
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult JavaBitmap::draw (FrameworkGraphics& androidDevice, RectRef _src, RectRef dst, const ImageMode* mode, float scaleFactor)
{
	// scale source rect from coords to bitmap pixels (destination coords are scaled to graphics pixels by the outer transfrom)
	PixelRect src (_src, scaleFactor);

	jobject paint = gGraphicsFactory->getCachedBitmapPaint (mode);
	FrameworkGraphicsClass.drawBitmapR (androidDevice, object, src.left, src.top, src.right, src.bottom, dst.left, dst.top, dst.right, dst.bottom, paint);
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void JavaBitmap::drawDirect (FrameworkGraphics& device, RectRef _src, float scaleFactor)
{
	// scale source rect from coords to bitmap pixels (destination coords are scaled to graphics pixels by the outer transfrom)
	PixelRect src (_src, scaleFactor);

	FrameworkGraphicsClass.drawBitmapDirect (device, object, src.left, src.top, src.right, src.bottom);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult JavaBitmap::tile (FrameworkGraphics& androidDevice, int method, RectRef src, RectRef dest, RectRef clip, RectRef margins, float scaleFactor)
{
	class AndroidBlitter: public Blitter
	{
	public:
		AndroidBlitter (FrameworkGraphics* device, jobject bitmap, float contentScaleFactor)
		: device (device),
		  bitmap (bitmap),
		  contentScaleFactor (contentScaleFactor),
		  paint (gGraphicsFactory->getCachedBitmapPaint (255, false)),
		  bounds (device->getUpdateRegion ())
		{
			bounds.offset (device->getOrigin () * -1);
		}

		// Blitter
		void blit (RectRef _src, RectRef dst) override
		{
			// bounds is the untransformed update region, but we can't calculate the transformed bounds here,
			// so don't try to optimize if there was a transform in the current draw event
			if(!device->hasTransform () && !dst.intersect (bounds))
			{
				CCL_PRINTF ("AndroidBlitter: skip %d, %d (%d x %d) bounds: %d, %d (%d x %d)", dst.left, dst.top, dst.getWidth (), dst.getHeight (), bounds.left, bounds.top, bounds.getWidth (), bounds.getHeight ())
				return;
			}

			PixelRect src (_src, contentScaleFactor);
			FrameworkGraphicsClass.drawBitmapR (*device, bitmap, src.left, src.top, src.right, src.bottom, dst.left, dst.top, dst.right, dst.bottom, paint);
		}

		FrameworkGraphics* device;
		jobject bitmap;
		float contentScaleFactor;
		jobject paint;
		JniAccessor jni;
		Rect bounds;
	};

	AndroidBlitter blitter (&androidDevice, object, scaleFactor);
	Tiler::tile (blitter, method, src, dest, clip, margins);
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult JavaBitmap::lockBits (BitmapLockData& data, IBitmap::PixelFormat format, int mode)
{
	ASSERT (format == IBitmap::kAny || format == IBitmap::kRGBAlpha)
	if(!(format == IBitmap::kAny || format == IBitmap::kRGBAlpha))
		return kResultInvalidArgument;

	JniAccessor jni;

	void* bits = 0;
	if(AndroidBitmap_lockPixels (jni, object, &bits) < 0)
		return kResultFailed;

	AndroidBitmapInfo info = {0};
	AndroidBitmap_getInfo (jni, object, &info);

	data.width = info.width;
	data.height = info.height;
	data.format = IBitmap::kRGBAlpha;
	data.scan0 = bits;
	data.rowBytes = info.stride;
	data.bitsPerPixel = 32;
	data.mode = mode;
	data.nativeData = bits;
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult JavaBitmap::unlockBits (BitmapLockData& data)
{
	AndroidBitmap_unlockPixels (JniAccessor (), object);
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult JavaBitmap::scrollPixelRect (const Rect& rect, const Point& delta)
{
	if(delta.x == 0 && delta.y == 0)
		return kResultOk;

	BitmapLockData bitmapData;
	tresult tr = lockBits (bitmapData, IBitmap::kRGBAlpha, IBitmap::kLockWrite);
	if(tr != kResultOk)
		return tr;

	Core::BitmapPrimitives32::scrollRect (bitmapData, rect, delta);

	unlockBits (bitmapData);
	return kResultOk;
}

//************************************************************************************************
// AndroidBitmap
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (AndroidBitmap, NativeBitmap)

/////////////////////////////////////////////////////////////////////////////////////////////////

AndroidBitmap::AndroidBitmap (IBitmapDecoder* customDecoder)
: NativeBitmap (Point ()),
  customDecoder (customDecoder)
{
	customDecoder->getPixelSize (sizeInPixel);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

AndroidBitmap::AndroidBitmap (JNIEnv* jni, jobject object)
: NativeBitmap (Point ()),
  javaBitmap (jni, object)
{
	AndroidBitmapInfo info = {0};
	AndroidBitmap_getInfo (jni, object, &info);

	this->sizeInPixel ((int)info.width, (int)info.height);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

AndroidBitmap::~AndroidBitmap ()
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

JavaBitmap* AndroidBitmap::getJavaBitmap ()
{
	if(javaBitmap.getJObject ())
		return &javaBitmap;

	if(customDecoder)
	{
		BitmapData data;
		data.init (sizeInPixel.x, sizeInPixel.y, Core::kBitmapRGBAlpha, false);
		pixelBuffer.setAlignment (8);
		pixelBuffer.resize (data.height * data.rowBytes);
		data.initScan0 (pixelBuffer.getAddressAligned (), false);
		if(customDecoder->getPixelData (data) != kResultOk)
			return nullptr;

		customDecoder.release ();
	}

	CCL_PRINTF ("getJavaBitmap: create (%d x %d)", sizeInPixel.x, sizeInPixel.y)

	// create new java bitmap
	JniAccessor jni;
	LocalRef object (jni, FrameworkGraphicsFactoryClass.createBitmapRaw (*gGraphicsFactory, sizeInPixel.x, sizeInPixel.y));
	if(jni.checkException () || object == nullptr)
		return nullptr;

	javaBitmap.assign (jni, object);

	BitmapLockData data;
	if(javaBitmap.lockBits (data, IBitmap::kAny, IBitmap::kLockRead) != kResultOk)
		return nullptr;

	// copy pixels from native bitmap
	uint32 bytesToCopy = data.rowBytes * data.height;
	ASSERT (bytesToCopy == pixelBuffer.getSize ())
	::memcpy (data.scan0, pixelBuffer.getAddressAligned (), bytesToCopy);

	unlockBits (data);

	return &javaBitmap;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult AndroidBitmap::draw (NativeGraphicsDevice& device, PointRef pos, const ImageMode* mode)
{
	return draw (device, pointIntToF (pos), mode);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult AndroidBitmap::draw (NativeGraphicsDevice& device, PointFRef pos, const ImageMode* mode)
{
	FrameworkGraphics* androidDevice = ccl_cast<FrameworkGraphics> (&device);
	JavaBitmap* javaBmp = getJavaBitmap ();
	if(!androidDevice || !javaBmp)
		return kResultUnexpected;

	return javaBmp->draw (*androidDevice, pos, mode, getContentScaleFactor ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API AndroidBitmap::draw (NativeGraphicsDevice& device, RectFRef src, RectFRef dst, const ImageMode* mode)
{
	return draw (device, rectFToInt (src), rectFToInt (dst), mode);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API AndroidBitmap::draw (NativeGraphicsDevice& device, RectRef src, RectRef dst, const ImageMode* mode)
{
	FrameworkGraphics* androidDevice = ccl_cast<FrameworkGraphics> (&device);
	JavaBitmap* javaBmp = getJavaBitmap ();
	if(!androidDevice || !javaBmp)
		return kResultUnexpected;

	return javaBmp->draw (*androidDevice, src, dst, mode, getContentScaleFactor ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AndroidBitmap::drawDirect (FrameworkGraphics& device, RectRef src)
{
	JavaBitmap* javaBmp = getJavaBitmap ();
	if(javaBmp)
		javaBmp->drawDirect (device, src, getContentScaleFactor ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult AndroidBitmap::tile (NativeGraphicsDevice& device, int method, RectRef src, RectRef dest, RectRef clip, RectRef margins)
{
	FrameworkGraphics* androidDevice = ccl_cast<FrameworkGraphics> (&device);
	JavaBitmap* javaBmp = getJavaBitmap ();
	if(!androidDevice || !javaBmp)
		return kResultUnexpected;

	return javaBmp->tile (*androidDevice, method, src, dest, clip, margins, getContentScaleFactor ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IBitmap::PixelFormat CCL_API AndroidBitmap::getPixelFormat () const
{
	return kRGBAlpha;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API AndroidBitmap::lockBits (BitmapLockData& data, PixelFormat format, int mode)
{
	JavaBitmap* javaBmp = getJavaBitmap ();
	if(!javaBmp)
		return kResultUnexpected;

	return javaBmp->lockBits (data, format, mode);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API AndroidBitmap::unlockBits (BitmapLockData& data)
{
	JavaBitmap* javaBmp = getJavaBitmap ();
	if(!javaBmp)
		return kResultUnexpected;

	return javaBmp->unlockBits (data);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API AndroidBitmap::scrollPixelRect (const Rect& rect, const Point& delta)
{
	JavaBitmap* javaBmp = getJavaBitmap ();
	if(!javaBmp)
		return kResultUnexpected;

	return javaBmp->scrollPixelRect (rect, delta);
}
