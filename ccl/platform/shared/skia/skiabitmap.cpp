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
// Filename    : ccl/platform/shared/skia/skiabitmap.cpp
// Description : Skia Engine
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "ccl/platform/shared/skia/skiabitmap.h"
#include "ccl/platform/shared/skia/skiastream.h"
#include "ccl/platform/shared/skia/skiadevice.h"
#include "ccl/platform/shared/skia/skiaengine.h"

#include "ccl/base/singleton.h"

#include "ccl/gui/graphics/imaging/tiler.h"

#include "ccl/public/storage/filetype.h"
#include "ccl/public/systemservices.h"
#include "ccl/public/gui/graphics/dpiscale.h"

namespace CCL {
	
//************************************************************************************************
// SkiaBitmapDecoder
//************************************************************************************************

class SkiaBitmapDecoder: public BitmapDecoder
{
public:
	SkiaBitmapDecoder (IMemoryStream& stream);
	
	// BitmapDecoder
	tresult CCL_API getPixelSize (Point& size);
	tresult CCL_API getPixelData (BitmapData& data);
	
private:
	std::unique_ptr<SkCodec> generator;
};
	
} // namespace CCL

using namespace CCL;

//************************************************************************************************
// SkiaBitmap
//************************************************************************************************

DEFINE_CLASS_HIDDEN (SkiaBitmap, NativeBitmap)

//////////////////////////////////////////////////////////////////////////////////////////////////

SkiaBitmap::SkiaBitmap ()
: NativeBitmap (Point (1,1)),
  mustDecode (false)
{
	imageInfo = SkImageInfo::Make (1, 1, colorType, kOpaque_SkAlphaType);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

SkiaBitmap::SkiaBitmap (PointRef sizeInPixel, PixelFormat format, float contentScaleFactor)
: NativeBitmap (sizeInPixel, contentScaleFactor),
  mustDecode (false)
{
	imageInfo = SkImageInfo::Make (sizeInPixel.x, sizeInPixel.y, colorType, format == kRGBAlpha ? kPremul_SkAlphaType : kOpaque_SkAlphaType);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

SkiaBitmap::SkiaBitmap (IBitmapDecoder* customDecoder, bool alphaChannelUsed)
: NativeBitmap (Point (0, 0)),
  decoder (customDecoder),
  mustDecode (true)
{
	ASSERT (customDecoder != nullptr)
	tresult result = customDecoder->getPixelSize (sizeInPixel);
	ASSERT (result == kResultOk)
	if(result == kResultOk)
		imageInfo = SkImageInfo::Make (sizeInPixel.x, sizeInPixel.y, colorType, alphaChannelUsed ? kPremul_SkAlphaType : kOpaque_SkAlphaType);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

SkiaBitmap::SkiaBitmap (IMemoryStream* stream, bool alphaChannelUsed)
: NativeBitmap (Point (0, 0)),
  decoder (NEW SkiaBitmapDecoder (*stream)),
  mustDecode (true)
{
	tresult result = decoder->getPixelSize (sizeInPixel);
	ASSERT (result == kResultOk)
	if(result == kResultOk)
		imageInfo = SkImageInfo::Make (sizeInPixel.x, sizeInPixel.y, colorType, alphaChannelUsed ? kPremul_SkAlphaType : kOpaque_SkAlphaType);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SkiaBitmap::decode ()
{
	if(!mustDecode || !decoder.isValid ())
		return;

	allocateBitmap ();
	
	BitmapData data;
	data.width = sizeInPixel.x;
	data.height = sizeInPixel.y;
	data.format = kRGBAlpha;
	data.scan0 = bitmap.getPixels ();
	data.bitsPerPixel = bitmap.bytesPerPixel () * 8;
	data.rowBytes = int(bitmap.rowBytes ());
	
	tresult result = decoder->getPixelData (data);
	ASSERT (result == kResultOk)
	if(result != kResultOk)
		return;
		
	mustDecode = false;
	decoder = nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

SkSurface* SkiaBitmap::getSurface ()
{
	if(!surface)
	{
		SkiaEngine* engine = SkiaEngine::getInstance ();
		GrRecordingContext* context = engine ? engine->getGPUContext () : nullptr;
		if(context)
			surface = SkSurfaces::RenderTarget (context, skgpu::Budgeted::kYes, imageInfo);
		else
			surface = SkSurfaces::Raster (imageInfo);
		if(!surface)
			return nullptr;
		surface->getCanvas ()->scale (getContentScaleFactor (), getContentScaleFactor ());
		if(!bitmap.isNull ())
			surface->getCanvas ()->drawImage (SkImages::RasterFromBitmap (bitmap), 0, 0);
	}
	return surface.get ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SkiaBitmap::allocateBitmap ()
{
	if(!bitmap.isNull ())
		return;
	
	bool success = bitmap.tryAllocPixelsFlags (imageInfo, SkBitmap::kZeroPixels_AllocFlag);
	ASSERT (success)
}

//////////////////////////////////////////////////////////////////////////////////////////////////

sk_sp<SkImage> SkiaBitmap::getSkiaImage () const
{
	if(!image)
	{
		if(surface)
			image = surface->makeImageSnapshot ();
		else
			image = SkImages::RasterFromBitmap (bitmap);
	}
	return image;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SkiaBitmap::saveTo (IStream& stream, const FileType& format)
{
	decode ();
	if(getSkiaImage () == nullptr)
		return false;
	
	SkiaEngine* engine = SkiaEngine::getInstance ();
	GrRecordingContext* context = engine ? engine->getGPUContext () : nullptr;
	SkEncodedImageFormat skFormat;
	sk_sp<SkData> imageData;
	if(format.getMimeType () == "image/png")
	{
		SkPngEncoder::Options options;
		imageData = SkPngEncoder::Encode (context->asDirectContext (), image.get (), options);
	}
	else if(format.getMimeType() == "image/jpeg")
	{
		SkJpegEncoder::Options options;
		options.fQuality = 80;
		imageData = SkJpegEncoder::Encode (context->asDirectContext (), image.get (), options);
	}
	
	if(!imageData)
		return false;
	
	stream.write (imageData->data (), (int)imageData->size ());
	
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SkiaBitmap::flush ()
{
	if(surface && surface->recordingContext ())
		surface->recordingContext ()->asDirectContext ()->flushAndSubmit (surface.get ());
	image = nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void* SkiaBitmap::getBits ()
{
	allocateBitmap ();
	decode ();
	return bitmap.getPixels ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

SkCanvas* SkiaBitmap::getCanvas ()
{
	if(SkSurface* surface = getSurface ())
		return surface->getCanvas ();
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult SkiaBitmap::draw (NativeGraphicsDevice& device, PointRef pos, const ImageMode* mode)
{
	decode ();
	return draw (device, pointIntToF (pos), mode);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult SkiaBitmap::draw (NativeGraphicsDevice& device, PointFRef pos, const ImageMode* mode)
{
	decode ();
	RectF size (0, 0, PointF (getWidth (), getHeight ()));
	RectF dst (size);
	dst.moveTo (pos);
	return draw (device, size, dst, mode);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult SkiaBitmap::draw (NativeGraphicsDevice& device, RectRef src, RectRef dst, const ImageMode* mode)
{
	decode ();
	return draw (device, rectIntToF (src), rectIntToF (dst), mode);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult SkiaBitmap::draw (NativeGraphicsDevice& device, RectFRef src, RectFRef dst, const ImageMode* mode)
{
	if(src.isEmpty () || dst.isEmpty ())
		return kResultOk;

	SkiaDevice* skiaDevice = ccl_cast<SkiaDevice> (&device);
	if(!skiaDevice)
		return kResultUnexpected;
		
	SkCanvas* canvas = skiaDevice->getCanvas ();
	if(!canvas)
		return kResultUnexpected;

	decode ();
	
	PixelRectF srcInPixel (src, contentScaleFactor);
	SkRect skSrc;
	SkiaDevice::toSkRect (skSrc, srcInPixel);
	PixelRectF dstInPixel (dst, 1);
	SkRect skDst;
	SkiaDevice::toSkRect (skDst, dstInPixel);
	
	SkSamplingOptions samplingOptions (SkFilterMode::kLinear, SkMipmapMode::kLinear);
	SkCanvas::SrcRectConstraint constraint = SkCanvas::kFast_SrcRectConstraint;
	SkPaint paint;
	if(mode)
	{
		if(mode->getInterpolationMode () == ImageMode::kInterpolationHighQuality)
		{
			// CatmullRom causes rendering issues on some Macs. Using default quality for now
			// samplingOptions = SkSamplingOptions (SkCubicResampler::CatmullRom ());
		}
		else if(mode->getInterpolationMode () == ImageMode::kInterpolationPixelQuality)
		{
			constraint = SkCanvas::kStrict_SrcRectConstraint;
			samplingOptions = SkSamplingOptions (SkFilterMode::kNearest, SkMipmapMode::kNone);
		}
		paint.setAlphaf (mode->getAlphaF ());
	}

	canvas->drawImageRect (getSkiaImage (), skSrc, skDst, samplingOptions, &paint, constraint);

	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult SkiaBitmap::tile (NativeGraphicsDevice& device, int method, RectRef src, RectRef dst, RectRef clip, RectRef margins)
{
	class SkiaBlitter : public Blitter
	{
	public:
		SkiaBlitter (SkCanvas& _canvas, sk_sp<SkImage> _image, const SkPaint& _paint, float _contentScaleFactor)
		: canvas (_canvas),
		  image (_image),
		  paint (_paint),
		  contentScaleFactor (_contentScaleFactor)
		{}
		
		void blit (RectRef src, RectRef dst)
		{
			PixelRectF srcInPixel (src, contentScaleFactor);
			SkRect skSrc;
			SkiaDevice::toSkRect (skSrc, srcInPixel);
			PixelRectF dstInPixel (dst, 1);
			SkRect skDst;
			SkiaDevice::toSkRect (skDst, dstInPixel);
			canvas.drawImageRect (image, skSrc, skDst, SkSamplingOptions (SkFilterMode::kNearest), &paint, SkCanvas::kFast_SrcRectConstraint);
		}
		
	protected:
		SkCanvas& canvas;
		sk_sp<SkImage> image;
		const SkPaint& paint;
		float contentScaleFactor;
	};

	SkiaDevice* skiaDevice = ccl_cast<SkiaDevice> (&device);
	if(!skiaDevice)
		return kResultUnexpected;
		
	SkCanvas* canvas = skiaDevice->getCanvas ();
	if(!canvas)
		return kResultUnexpected;
	
	decode ();
	
	SkPaint paint;
	SkiaBlitter blitter (*canvas, getSkiaImage (), paint, contentScaleFactor);
	Tiler::tile (blitter, method, src, dst, clip, margins);
	
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API SkiaBitmap::lockBits (BitmapLockData& data, PixelFormat format, int mode)
{
	allocateBitmap ();
	decode ();
	
	if(surface)
		surface->getCanvas ()->readPixels (bitmap, 0, 0);

	bool pixelFormatCompatible = (format == kAny || format == kRGBAlpha) && (bitmap.colorType () == SkColorType::kBGRA_8888_SkColorType);
	ASSERT (pixelFormatCompatible)
	if(!pixelFormatCompatible)
		return kResultInvalidArgument;

	data.width = sizeInPixel.x;
	data.height = sizeInPixel.y;
	data.format = kRGBAlpha;
	data.scan0 = bitmap.getPixels ();
	data.bitsPerPixel = bitmap.bytesPerPixel () * 8;
	data.rowBytes = int(bitmap.rowBytes ());
	data.mode = mode;
	data.nativeData = nullptr;

	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API SkiaBitmap::unlockBits (BitmapLockData& data)
{
	if(data.mode == kLockWrite)
	{
		bitmap.notifyPixelsChanged ();
		if(surface)
			surface->getCanvas ()->writePixels (bitmap, 0, 0);
		flush ();
	}
	
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API SkiaBitmap::scrollPixelRect (const Rect& rectInPixel, const Point& deltaInPixel)
{
	if(deltaInPixel.isNull ())
		return kResultOk;

	if(getSkiaImage () == nullptr)
		return kResultFailed;

	PixelRect rectInPoint (rectInPixel, 1.f / contentScaleFactor);
	PixelPoint deltaInPoint (deltaInPixel, 1.f / contentScaleFactor);
	
	SkRect dstRect;
	SkiaDevice::toSkRect (dstRect, rectInPoint);
	dstRect = dstRect.makeOffset (deltaInPoint.x, deltaInPoint.y);

	SkRect srcRect;
	SkiaDevice::toSkRect (srcRect, rectInPixel);

	// srcRect in pixel coordinates, dstRect in point coordinates
	getSurface ()->getCanvas ()->drawImageRect (image.get (), srcRect, dstRect, SkSamplingOptions (), nullptr, SkCanvas::kFast_SrcRectConstraint);

	flush ();

	return kResultOk;
}

//************************************************************************************************
// SkiaBitmapRenderTarget
//************************************************************************************************

SkiaBitmapRenderTarget::SkiaBitmapRenderTarget (SkiaBitmap& nativeBitmap)
: bitmap (nativeBitmap)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

SkiaBitmapRenderTarget::~SkiaBitmapRenderTarget ()
{
	bitmap.flush ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

SkCanvas* SkiaBitmapRenderTarget::getCanvas ()
{
	return bitmap.getCanvas ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

float SkiaBitmapRenderTarget::getContentScaleFactor () const
{
	return bitmap.getContentScaleFactor ();
}

//************************************************************************************************
// SkiaBitmapDecoder
//************************************************************************************************

SkiaBitmapDecoder::SkiaBitmapDecoder (IMemoryStream& stream)
: BitmapDecoder (stream)
{
	SkiaStream skStream (&stream);
	sk_sp<SkData> data = SkData::MakeFromStream (&skStream, skStream.getLength ());
	generator = SkCodec::MakeFromData (data);

	ASSERT (generator != nullptr)
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API SkiaBitmapDecoder::getPixelSize (Point& size)
{
	if(generator == nullptr)
		return kResultFailed;

	const SkImageInfo& srcInfo = generator->getInfo ();
	size.x = srcInfo.dimensions ().width ();
	size.y = srcInfo.dimensions ().height ();
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API SkiaBitmapDecoder::getPixelData (BitmapData& data)
{
	if(generator == nullptr)
		return kResultFailed;

	const SkImageInfo& srcInfo = generator->getInfo ();
	SkImageInfo dstInfo = SkImageInfo::Make (srcInfo.dimensions ().width (), srcInfo.dimensions ().height (), SkiaBitmap::colorType, srcInfo.isOpaque () ? kOpaque_SkAlphaType : kPremul_SkAlphaType);
	if(data.width < dstInfo.dimensions ().width () || data.height < dstInfo.dimensions ().height () || data.rowBytes < dstInfo.minRowBytes ())
		return kResultInvalidArgument;
	SkCodec::Result result = generator->getPixels (dstInfo, data.scan0, data.rowBytes);
	return result == SkCodec::kSuccess ? kResultOk : kResultFailed;
}
