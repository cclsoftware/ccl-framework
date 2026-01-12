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
// Filename    : ccl/platform/cocoa/quartz/quartzbitmap.mm
// Description : Quartz Bitmap
//
//************************************************************************************************

#define DEBUG_LOG 0
#define DEBUG_REDRAWS 0

#import <CoreGraphics/CGBitmapContext.h>

#include "ccl/base/singleton.h"

#include "ccl/platform/cocoa/quartz/quartzbitmap.h"
#include "ccl/platform/cocoa/quartz/device.h"
#include "ccl/platform/cocoa/quartz/cgdataprovider.h"
#include "ccl/platform/cocoa/macutils.h"
#include "ccl/platform/cocoa/quartz/cghelper.h"

#include "ccl/gui/graphics/imaging/tiler.h"

#include "ccl/public/gui/graphics/dpiscale.h"
#include "ccl/public/gui/framework/idleclient.h"
#include "ccl/public/systemservices.h"

#include "core/gui/corebitmapprimitives.h"

using namespace CCL;
using namespace MacOS;

#define DELAY_DECODING 0
#define ONDEMAND_DECODING 1

#if DELAY_DECODING || ONDEMAND_DECODING
#define CHECK_DECODE if(mustDecode) decode ();
#else
#define CHECK_DECODE
#endif

#if DELAY_DECODING
namespace CCL {
namespace MacOS
{

//************************************************************************************************
// QuartzBitmapDecoder
/** Decodes bitmaps on idle. */
//************************************************************************************************

class QuartzBitmapDecoder: public Object,
						   public Singleton<QuartzBitmapDecoder>,
						   public IdleClient
{
public:
	~QuartzBitmapDecoder ()
	{
		bitmaps.objectCleanup ();
	}

	void addBitmap (QuartzBitmap* bitmap)
	{
		bitmap->retain ();
		bitmaps.add (bitmap);
		
		if(!isTimerEnabled ())
		{
			startTimer (kInitialDelay);
			CCL_PRINTF ("QuartzBitmapDecoder: start (%d)\n", (int)System::GetSystemTicks () / 1000)
		}
	}
	
	// IdleClient
	void onIdleTimer ()
	{
		// decode one bitmap per idle
		QuartzBitmap* bitmap = (QuartzBitmap*)bitmaps.removeFirst ();
		if(bitmap)
		{
			bitmap->decode ();
			bitmap->release ();
			delay = kRegularDelay;
		}
		else
		{
			stopTimer ();
			CCL_PRINTF ("QuartzBitmapDecoder: done (%d)\n", (int)System::GetSystemTicks () / 1000)
		}
	}
	
	CLASS_INTERFACE (ITimerTask, Object)
	
private:
	ObjectList bitmaps;
		
	enum { kInitialDelay = 1000, kRegularDelay = 10 };
};

}} //namespace MacOS, CCL

DEFINE_SINGLETON (QuartzBitmapDecoder)

#endif

//************************************************************************************************
// QuartzBitmap
//************************************************************************************************

void bitsReleaseCallback (void* info, const void* data, size_t size)
{
	Buffer* bits = (Buffer*)info;
	if(bits)
		bits->release ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_CLASS_HIDDEN (QuartzBitmap, NativeBitmap)

//////////////////////////////////////////////////////////////////////////////////////////////////

QuartzBitmap::QuartzBitmap (PointRef sizeInPixel, PixelFormat format, float contentScaleFactor)
: NativeBitmap (sizeInPixel, contentScaleFactor),
  image (NULL),
  mustDecode (false)
{
	ASSERT (sizeInPixel.x > 0 && sizeInPixel.y > 0)

	int bitsPerComponent = 8;
	int bytesPerPixel = 4;
	int bytesPerRow = bytesPerPixel * sizeInPixel.x;

	bits = NEW Buffer (bytesPerRow * sizeInPixel.y, true);
    if(bits)
    {
    	bits->retain ();
        CFObj<CGDataProviderRef> dataProvider = CGDataProviderCreateWithData (bits, getBits (), bytesPerRow * sizeInPixel.y, bitsReleaseCallback);
        CFObj<CGColorSpaceRef> colorSpace = CGColorSpaceCreateDeviceRGB ();

        CGBitmapInfo pixelFormat = format == kRGBAlpha ? kCGImageAlphaPremultipliedFirst : kCGImageAlphaNoneSkipFirst;
		pixelFormat |= kCGBitmapByteOrder32Host;

        image = CGImageCreate (sizeInPixel.x, sizeInPixel.y, bitsPerComponent, bytesPerPixel * 8, bytesPerRow, colorSpace, pixelFormat, dataProvider, 0, false, kCGRenderingIntentAbsoluteColorimetric);
    }
	ASSERT (image != NULL)
}

//////////////////////////////////////////////////////////////////////////////////////////////////

QuartzBitmap::QuartzBitmap (IStream* stream)
: NativeBitmap (Point (0, 0)),
  image (NULL),
  mustDecode (false)
{
	CGDataProviderRef provider = CGStreamDataProvider::create (stream);

	// try create as PNG...
	CGImageRef imageFromStream = CGImageCreateWithPNGDataProvider (provider, 0, false, kCGRenderingIntentDefault);
	// ...or try as JPEG
	if(!imageFromStream)
		imageFromStream = CGImageCreateWithJPEGDataProvider (provider, 0, false, kCGRenderingIntentDefault);

	CGDataProviderRelease (provider);

	ASSERT (imageFromStream != NULL)
	if(imageFromStream)
	{
		sizeInPixel.x = (int)CGImageGetWidth (imageFromStream);
		sizeInPixel.y = (int)CGImageGetHeight (imageFromStream);
		image = imageFromStream;

		mustDecode = true;
		#if DELAY_DECODING
		QuartzBitmapDecoder::instance ().addBitmap (this);
		#else
		#if !ONDEMAND_DECODING
		decode ();
		#endif
		#endif
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

QuartzBitmap::QuartzBitmap (CGImageRef image)
: NativeBitmap (Point (0, 0)),
  image (image),
  mustDecode (false)
{
	if(image)
	{
		CGImageRetain (image);
		sizeInPixel.x = (int)CGImageGetWidth (image);
		sizeInPixel.y = (int)CGImageGetHeight (image);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

QuartzBitmap::QuartzBitmap (IBitmapDecoder* _customDecoder)
: NativeBitmap (Point (0, 0)),
  image (NULL),
  customDecoder (_customDecoder),
  mustDecode (false)
{
	tresult result = customDecoder->getPixelSize (sizeInPixel);
	ASSERT (result == kResultOk)
	
	mustDecode = true;
	#if DELAY_DECODING
	QuartzBitmapDecoder::instance ().addBitmap (this);
	#else
	#if !ONDEMAND_DECODING
	decode ();
	#endif
	#endif
}

//////////////////////////////////////////////////////////////////////////////////////////////////

QuartzBitmap::~QuartzBitmap ()
{
	CGImageRelease (image);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void QuartzBitmap::decode ()
{
	if(!mustDecode)
		return;
	mustDecode = false;

	int bitsPerComponent = 8;
	int bytesPerPixel = 4;
	int bytesPerRow = bytesPerPixel * sizeInPixel.x;
	
	bits = NEW Buffer (bytesPerRow * sizeInPixel.y, true);

	bits->retain ();
	CFObj<CGDataProviderRef> dataProvider = CGDataProviderCreateWithData (bits, getBits (), bytesPerRow * sizeInPixel.y, bitsReleaseCallback);
	CFObj<CGColorSpaceRef> colorSpace = CGColorSpaceCreateDeviceRGB ();
	CGBitmapInfo pixelFormat = kCGImageAlphaPremultipliedFirst|kCGBitmapByteOrder32Host;
	
	if(customDecoder)
	{
		image = CGImageCreate (sizeInPixel.x, sizeInPixel.y, bitsPerComponent, bytesPerPixel * 8, bytesPerRow, colorSpace, pixelFormat, dataProvider, 0, false, kCGRenderingIntentAbsoluteColorimetric);
		BitmapLockData data;
		if(lockBits (data, kRGBAlpha, kLockWrite) == kResultOk)
		{
			tresult result = customDecoder->getPixelData (data);
			ASSERT (result == kResultOk)
			unlockBits (data);
		}
		customDecoder.release ();
	}
	else
	{
		ASSERT (image)
		if(!image)
			return;
		CGImageRef decodedImage = CGImageCreate (sizeInPixel.x, sizeInPixel.y, bitsPerComponent, bytesPerPixel * 8, bytesPerRow, colorSpace, pixelFormat, dataProvider, 0, false, kCGRenderingIntentAbsoluteColorimetric);
		
		CGRect bounds = {{0,0}, {static_cast<CGFloat>(sizeInPixel.x), static_cast<CGFloat>(sizeInPixel.y)}};
		CGContextRef context = CGBitmapContextCreate (getBits (), sizeInPixel.x,  sizeInPixel.y, bitsPerComponent, bytesPerRow, colorSpace, pixelFormat);
		CGContextDrawImage (context, bounds, image);
		
		CGContextRelease (context);
		CGImageRelease (decodedImage);
	}

	CCL_PRINT (".")
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult QuartzBitmap::draw (NativeGraphicsDevice& device, PointRef pos, const ImageMode* mode)
{
	return draw (device, pointIntToF (pos), mode);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult QuartzBitmap::draw (NativeGraphicsDevice& device, PointFRef pos, const ImageMode* mode)
{
	QuartzDevice* qDevice = ccl_cast<QuartzDevice> (&device);
	if(!qDevice)
		return kResultUnexpected;

	CGRect bounds;
	bounds.origin.x = 0;
	bounds.origin.y = 0;
	bounds.size.width = getWidth ();
	bounds.size.height = getHeight ();
	
	CGContextRef context = qDevice->getTarget ().getContext ();
	ASSERT (context)
	
	CGContextTranslateCTM (context, pos.x, pos.y + bounds.size.height);
	CGContextScaleCTM (context, 1, -1);
	if(mode)
		CGContextSetAlpha (context, mode->getAlphaF ());
	
	CGContextDrawImage (context, bounds, image);
	
	if(mode)
		CGContextSetAlpha (context, 1);
	CGContextScaleCTM (context, 1, -1);
	CGContextTranslateCTM (context, -pos.x, -(pos.y + bounds.size.height));

	#if DEBUG_REDRAWS
	System::ThreadSleep (50);
	CGContextSynchronize (context);
	CGContextFlush (context);
	#endif
	
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult QuartzBitmap::draw (NativeGraphicsDevice& device, RectRef src, RectRef dst, const ImageMode* mode)
{
	return draw (device, rectIntToF (src), rectIntToF (dst), mode);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult QuartzBitmap::draw (CGContextRef context, RectRef src, RectRef dst, const ImageMode* mode)
{
	return draw (context, rectIntToF (src), rectIntToF (dst), mode);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult QuartzBitmap::draw (NativeGraphicsDevice& device, RectFRef src, RectFRef dst, const ImageMode* mode)
{
	QuartzDevice* qDevice = ccl_cast<QuartzDevice> (&device);
	if(!qDevice)
		return kResultUnexpected;
	CGContextRef context = qDevice->getTarget ().getContext ();
	
	return draw (context,  src,  dst,  mode);
}

/////////////////////////////////////////////////////////////////////////////////////////////////

tresult QuartzBitmap::draw (CGContextRef context, RectFRef src, RectFRef dst, const ImageMode* mode)
{
	ASSERT (context)
	
	if(src.isEmpty () || dst.isEmpty ())
		return kResultOk;

	#if DEBUG_REDRAWS
	CGRect _bounds;
	MacOS::toCGRect (_bounds, dst);
	CGContextSetRGBFillColor (qDevice->getCGContext (), 0, 0, 1, 1);
	CGContextFillRect (qDevice->getCGContext (), _bounds);
	CGContextSynchronize (qDevice->getCGContext ());
	CGContextFlush (qDevice->getCGContext ());
	#endif

	CHECK_DECODE

	CGImageRef selectedImage = image;
	CGImageRef subImage = NULL;
	if(src.left != 0 || src.top != 0 || src.right != getWidth () || src.bottom != getHeight ())
	{
		// bounds has to be given in pixels, src is in points
		PixelRectF srcInPixel (src, contentScaleFactor);
		CGRect bounds;
		MacOS::toCGRect (bounds, srcInPixel);
		selectedImage = subImage = CGImageCreateWithImageInRect (image, bounds);
		ASSERT (selectedImage)
	}
	
	CGRect bounds;
	bounds.origin.x = 0;
	bounds.origin.y = 0;
	bounds.size.width = dst.getWidth ();
	bounds.size.height = dst.getHeight ();
	
	CGContextTranslateCTM (context, dst.left, dst.bottom);
    CGContextScaleCTM (context, 1, -1);
	if(mode)
		CGContextSetAlpha (context, mode->getAlphaF ());
	
	#if DEBUG
	CGBitmapInfo info = CGImageGetBitmapInfo (selectedImage);
	CGImageAlphaInfo alphaInfo = (CGImageAlphaInfo)(info & kCGBitmapAlphaInfoMask);
	CGImageByteOrderInfo byteOrder = (CGImageByteOrderInfo)(info & kCGBitmapByteOrderMask);
	#endif

	CGContextDrawImage (context, bounds, selectedImage);
	
	if(mode)
		CGContextSetAlpha (context, 1);
    CGContextScaleCTM (context, 1, -1);
	CGContextTranslateCTM (context, -dst.left, -dst.bottom);

	if(subImage)
		CGImageRelease (subImage);
			
	#if DEBUG_REDRAWS
	System::ThreadSleep (50);
	CGContextSynchronize (context);
	CGContextFlush (context);
	#endif
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult QuartzBitmap::tile (NativeGraphicsDevice& device, int method, RectRef src, RectRef dst, RectRef clip, RectRef margins)
{	
	class QuartzBlitter : public Blitter
	{
	public:
		float contentScaleFactor;
		QuartzBlitter (QuartzDevice* device, CGImageRef image, float contentScaleFactor)
		: context (device->getTarget ().getContext ()), image (image), subImage (NULL), contentScaleFactor (contentScaleFactor)
		{
		}

		~QuartzBlitter ()
		{
			if(subImage)
				CGImageRelease (subImage);
		}
		
		void blit (RectRef src, RectRef dst)
		{
			if(src.getHeight () == 0 || src.getWidth () == 0)
				return;
				
			if(subImage != NULL)
			{
				if(subSrc != src)
				{
					CGImageRelease (subImage);
					
					PixelRect srcInPixel (src, contentScaleFactor);
					CGRect bounds;
					MacOS::toCGRect (bounds, srcInPixel);
					subImage = CGImageCreateWithImageInRect (image, bounds);
					ASSERT (subImage)
					subSrc = src;
				}
			}
			else
			{
				PixelRect srcInPixel (src, contentScaleFactor);
				CGRect bounds;
				MacOS::toCGRect (bounds, srcInPixel);
				subImage = CGImageCreateWithImageInRect (image, bounds);
				ASSERT (subImage)
				subSrc = src;
			}
			
			CGRect bounds;
			bounds.origin.x = 0;
			bounds.origin.y = 0;
			bounds.size.width = dst.getWidth ();
			bounds.size.height = dst.getHeight ();

			CGContextTranslateCTM (context, dst.left, dst.bottom);
			CGContextScaleCTM (context, 1, -1);
			CGContextDrawImage (context, bounds, subImage);
			
			CGContextScaleCTM (context, 1, -1);
			CGContextTranslateCTM (context, -dst.left, -dst.bottom);
		}
		
		CGContextRef context;
		CGImageRef image;
		CGImageRef subImage;
		Rect subSrc;
	};

	QuartzDevice* qDevice = ccl_cast<QuartzDevice> (&device);
	if(!qDevice)
		return kResultUnexpected;

	CHECK_DECODE
	
	#if DEBUG_REDRAWS
	CGRect bounds;
	MacOS::toCGRect (bounds, dst);
	CGContextSetRGBFillColor (qDevice->getCGContext (), 0, 0, 1, 1);
	CGContextFillRect (qDevice->getCGContext (), bounds);
	CGContextSynchronize (qDevice->getCGContext ());
	CGContextFlush (qDevice->getCGContext ());
	#endif
	
	QuartzBlitter blitter (qDevice, image, contentScaleFactor);
	Tiler::tile (blitter, method, src, dst, clip, margins);
	
	#if DEBUG_REDRAWS
	System::ThreadSleep (50);
	CGContextSynchronize (qDevice->getCGContext ());
	CGContextFlush (qDevice->getCGContext ());
	#endif

	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API QuartzBitmap::scrollPixelRect (const Rect& rect, const Point& delta)
{
	if(delta.x == 0 && delta.y == 0)
		return kResultOk;

	CHECK_DECODE

	BitmapLockData bitmapData;
	tresult tr = lockBits (bitmapData, kRGBAlpha, kLockWrite);
	if(tr != kResultOk)
		return tr;

	Core::BitmapPrimitives32::scrollRect (bitmapData, rect, delta);

	unlockBits (bitmapData);
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API QuartzBitmap::lockBits (BitmapLockData& data, PixelFormat format, int mode)
{
	ASSERT (format == kAny || format == kRGBAlpha)
	if(!(format == kAny || format == kRGBAlpha))
		return kResultInvalidArgument;

	CHECK_DECODE

	data.width = sizeInPixel.x;
	data.height = sizeInPixel.y;
	data.format = kRGBAlpha;
	data.scan0 = getBits ();
	data.bitsPerPixel = 32;
	data.rowBytes = data.getBytesPerPixel () * sizeInPixel.x;
	data.mode = mode;
	data.nativeData = NULL;
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API QuartzBitmap::unlockBits (BitmapLockData& data)
{
	if(data.mode == kLockWrite)
		recreate ();
		
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void QuartzBitmap::recreate ()
{
	int bitsPerComponent = 8;
	int bytesPerPixel = 4;
	int bytesPerRow = bytesPerPixel * sizeInPixel.x;

    // Recreate the image
    if(bits)
    {
		CGBitmapInfo pixelFormat = CGImageGetBitmapInfo (image);
        CGImageRelease (image);
		
       	bits->retain ();
        CFObj<CGDataProviderRef> dataProvider = CGDataProviderCreateWithData (bits, getBits (), bytesPerRow * sizeInPixel.y, bitsReleaseCallback);
        CFObj<CGColorSpaceRef> colorSpace = CGColorSpaceCreateDeviceRGB ();
		
        image = CGImageCreate (sizeInPixel.x, sizeInPixel.y, bitsPerComponent, bytesPerPixel * 8, bytesPerRow, colorSpace, pixelFormat, dataProvider, 0, false, kCGRenderingIntentAbsoluteColorimetric);
	}
}

//************************************************************************************************
// QuartzBitmapRenderTarget
//************************************************************************************************

QuartzBitmapRenderTarget::QuartzBitmapRenderTarget (QuartzBitmap& nativeBitmap)
: bitmap (nativeBitmap),
  context (NULL)
{
	bitmap.retain ();
	createContext ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

QuartzBitmapRenderTarget::~QuartzBitmapRenderTarget ()
{
	releaseContext ();
	bitmap.release ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void QuartzBitmapRenderTarget::createContext ()
{
	int bitsPerComponent = 8;
	int bytesPerPixel = 4;
	int bytesPerRow = bytesPerPixel * bitmap.getPixelWidth ();
	
	CFObj<CGColorSpaceRef> colorSpace = CGColorSpaceCreateDeviceRGB ();
	CGBitmapInfo pixelFormat = CGImageGetBitmapInfo (bitmap.getCGImage ());
	context = CGBitmapContextCreate (bitmap.getBits (), bitmap.getPixelWidth (), bitmap.getPixelHeight (), bitsPerComponent, bytesPerRow, colorSpace, pixelFormat);
	ASSERT (context)
	float contentScaleFactor = bitmap.getContentScaleFactor ();
	CGContextConcatCTM (context, CGAffineTransformMake (contentScaleFactor, 0, 0, -contentScaleFactor, 0, bitmap.getPixelHeight ()));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void QuartzBitmapRenderTarget::releaseContext ()
{
	if(context)
	{
		CFRelease (context);
		context = NULL;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

float QuartzBitmapRenderTarget::getContentScaleFactor () const
{
	return bitmap.getContentScaleFactor ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void QuartzBitmapRenderTarget::flush ()
{
	bitmap.recreate ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void QuartzBitmapRenderTarget::onScroll (RectRef rect, PointRef delta)
{
	bitmap.scrollPixelRect (rect, delta);
}
