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
// Filename    : ccl/gui/graphics/imaging/bitmappainter.cpp
// Description : Bitmap Painter
//
//************************************************************************************************

#include "ccl/gui/graphics/imaging/bitmappainter.h"
#include "ccl/gui/graphics/imaging/bitmapfilter.h"

#include "ccl/gui/graphics/imaging/bitmap.h"
#include "ccl/gui/graphics/imaging/imagepart.h"
#include "ccl/gui/graphics/imaging/tiledimage.h"
#include "ccl/gui/graphics/imaging/filmstrip.h"
#include "ccl/gui/graphics/imaging/multiimage.h"

#include "ccl/gui/graphics/graphicsdevice.h"
#include "ccl/gui/graphics/nativegraphics.h"

using namespace CCL;

//************************************************************************************************
// BitmapPainter
//************************************************************************************************

tresult BitmapPainter::drawImage (IGraphics& graphics, IImage* image, RectRef src, RectRef dst, IBitmapFilter* filter, Color backColor)
{
	ASSERT (image && filter)
	Rect imageSize (0, 0, image->getWidth (), image->getHeight ());

	if(Filmstrip* filmstrip = unknown_cast<Filmstrip> (image))
	{
		if(Image* subFrame = filmstrip->getSubFrame (filmstrip->getFrameName (filmstrip->getCurrentFrame ())))
			image = subFrame;
	}
	else if(MultiImage* multiImage = unknown_cast<MultiImage> (image))
	{
		if(Image* subFrame = multiImage->getFrame (multiImage->getCurrentFrame ()))
			image = subFrame;
	}

	if(src == imageSize && dst.getWidth () == src.getWidth () && dst.getHeight () == src.getHeight ()) // full source, no scaling
	{
		BitmapProcessor processor;
		tresult tr = processor.setup (image, backColor, 0, nullptr, graphics.getContentScaleFactor ());
		if(tr != kResultOk)
			return tr;

		tr = processor.process (*filter);
		ASSERT (tr == kResultOk)

		graphics.drawImage (processor.getOutput (), src, dst);
	}
	else
	{
		if(TiledImage* tiledImage = unknown_cast<TiledImage> (image)) // special case for tiled images (filter before tile)
		{
			Rect originalRect;
			IImage* original = tiledImage->getOriginalImage (originalRect);

			BitmapProcessor processor;
			tresult tr = processor.setup (original, backColor, 0, nullptr, graphics.getContentScaleFactor ());
			if(tr != kResultOk)
				return tr;

			tr = processor.process (*filter);
			ASSERT (tr == kResultOk)

			TiledImage tiledImage2 (unknown_cast<Image> (processor.getOutput ()), tiledImage->getMethod (), tiledImage->getMargins ());
			graphics.drawImage (&tiledImage2, src, dst);
		}
		else
		{
			CCL_NOT_IMPL ("Scaling + segments not implemented!")
			return kResultNotImplemented;
		}
	}
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_CLASS (BitmapPainter, Object)
DEFINE_CLASS_UID (BitmapPainter, 0x421579be, 0x7d53, 0x4716, 0xae, 0x8a, 0xa7, 0x8f, 0xad, 0x0, 0xf8, 0xb)

//////////////////////////////////////////////////////////////////////////////////////////////////

BitmapPainter::BitmapPainter ()
: backColor (Colors::kWhite)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API BitmapPainter::setBackColor (Color color)
{
	backColor = color;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API BitmapPainter::setFilter (IBitmapFilter* _filter, tbool share)
{
	if(share)
		filter.share (_filter);
	else
		filter = _filter;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API BitmapPainter::drawImage (IGraphics& graphics, IImage* image, RectRef src, RectRef dst)
{
	if(filter)
		return drawImage (graphics, image, src, dst, filter, backColor);
	else
	{
		graphics.drawImage (image, src, dst);
		return kResultOk;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API BitmapPainter::drawInverted (IGraphics& graphics, IImage* image, RectRef src, RectRef dst)
{
	BitmapFilters::RevertPremultipliedAlpha reverter;
	BitmapFilters::Inverter inverter;
	BitmapFilters::PremultipliedAlpha premultiplier;

	BitmapFilterList filterList;
	filterList.addFilter (&reverter, true);
	filterList.addFilter (&inverter, true);
	filterList.addFilter (&premultiplier, true);

	return drawImage (graphics, image, src, dst, filterList, backColor);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API BitmapPainter::drawColorized (IGraphics& graphics, IImage* image, RectRef src, RectRef dst, Color color)
{
	BitmapFilters::RevertPremultipliedAlpha reverter;
	BitmapFilters::Colorizer colorizer;
	BitmapFilters::PremultipliedAlpha premultiplier;

	colorizer.setColor (color);

	BitmapFilterList filterList;
	filterList.addFilter (&reverter, true);
	filterList.addFilter (&colorizer, true);
	filterList.addFilter (&premultiplier, true);

	return drawImage (graphics, image, src, dst, filterList, backColor);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API BitmapPainter::drawTinted (IGraphics& graphics, IImage* image, RectRef src, RectRef dst, Color color)
{
	BitmapFilters::RevertPremultipliedAlpha reverter;
	BitmapFilters::Tinter tinter;
	BitmapFilters::PremultipliedAlpha premultiplier;

	tinter.setColor (color);

	BitmapFilterList filterList;
	filterList.addFilter (&reverter, true);
	filterList.addFilter (&tinter, true);
	filterList.addFilter (&premultiplier, true);

	return drawImage (graphics, image, src, dst, filterList, backColor);
}

//************************************************************************************************
// BitmapProcessor
//************************************************************************************************

inline bool checkFormat (IBitmap* bitmap, IBitmap::PixelFormat format)
{
	return bitmap && (bitmap->getPixelFormat () == format || bitmap->getPixelFormat () == IBitmap::kAny);
					//was: BitmapDataLocker (bitmap, format, IBitmap::kLockRead).result == kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IBitmap* BitmapProcessor::convert (IImage* _image, IBitmap::PixelFormat format, Color backColor,
								   float defaultScaleFactor, bool& copied)
{
	copied = false;
	Image* image = unknown_cast<Image> (_image);
	ASSERT (image != nullptr)
	if(image == nullptr)
		return nullptr;

	#if 1
	// check if image supports IBitmap (classes: Bitmap, ImagePart, etc.)
	if(UnknownPtr<IBitmap> bitmap = image->asUnknown ())
	{
		if(checkFormat (bitmap, format))
			return bitmap.detach ();
	}
	else
	#endif
	{
		// try to resolve original bitmap (classes: MultiImage, Filmstrip, etc.)
		Rect partRect;
		if(Bitmap* original = Bitmap::getOriginalBitmap (partRect, image, true))
		{
			Rect size;
			original->getSize (size);
			if(size == partRect)
			{
				if(checkFormat (original, format))
					return return_shared (original);
			}
			else
			{
				AutoPtr<ImagePart> part = NEW ImagePart (original, partRect);
				UnknownPtr<IBitmap> partBitmap (part->asUnknown ());
				if(checkFormat (partBitmap, format))
					return partBitmap.detach ();
			}
		}
	}

	// copy image to new bitmap
	copied = true;
	Bitmap* bitmap = NEW Bitmap (image->getWidth (), image->getHeight (), format, defaultScaleFactor);
	{
		BitmapGraphicsDevice device (bitmap);
		Rect r;
		image->getSize (r);
		device.drawImage (image, r, r);
	}
	return bitmap;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_CLASS (BitmapProcessor, Object)
DEFINE_CLASS_UID (BitmapProcessor, 0x2aec6ea5, 0xbe3f, 0x43b7, 0x8d, 0x43, 0x27, 0x23, 0x23, 0xad, 0x69, 0x43)

//////////////////////////////////////////////////////////////////////////////////////////////////

void BitmapProcessor::setDestination (IImage* dstImage)
{
	dstBitmap.share (UnknownPtr<IBitmap> (dstImage));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void BitmapProcessor::setSource (IImage* srcImage)
{
	srcBitmap.share (UnknownPtr<IBitmap> (srcImage));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API BitmapProcessor::setup (IImage* srcImage, Color backColor, int options, const Point* size,
										float defaultScaleFactor)
{
	dstBitmap.release ();

	bool copied = false;
	srcBitmap = convert (srcImage, IBitmap::kRGBAlpha, backColor, defaultScaleFactor, copied);
	if(!srcBitmap)
		return kResultInvalidArgument;

	bool inplace = (options & kInplace) != 0;
	bool resized = size && (srcImage->getWidth () != size->x || srcImage->getHeight () != size->y);
	UnknownPtr<IMultiResolutionBitmap> srcMultiBitmap (srcBitmap);

	auto createBitmap = [&] (int width, int height) -> IBitmap*
	{
		if(srcMultiBitmap.isValid ())
			return NEW MultiResolutionBitmap (width, height, IBitmap::kRGBAlpha);
		else
			return NEW Bitmap (width, height, IBitmap::kRGBAlpha, srcBitmap->getContentScaleFactor ());
	};

	if(resized == true) // output size is different from input size
	{
		dstBitmap = createBitmap (size->x, size->y);

		{
			// source will be centered, not scaled
			Rect srcRect (0, 0, srcImage->getWidth (), srcImage->getHeight ());
			Rect dstRect (0, 0, size->x, size->y);
			Rect newRect (dstRect);
			newRect.center (srcRect);

			auto copyPart = [&] () -> tresult
			{
				BitmapDataLocker srcLocker (srcBitmap, IBitmap::kRGBAlpha, IBitmap::kLockRead);
				if(srcLocker.result != kResultOk)
					return srcLocker.result;

				BitmapDataLocker dstLocker (dstBitmap, IBitmap::kRGBAlpha, IBitmap::kLockWrite);
				if(dstLocker.result != kResultOk)
					return dstLocker.result;

				ASSERT (srcBitmap->getContentScaleFactor () == dstBitmap->getContentScaleFactor ())
				float scaleFactor = srcBitmap->getContentScaleFactor ();
				int pixelOffsetX = newRect.left;
				int pixelOffsetY = newRect.top;
				if(scaleFactor != 1.f)
				{
					pixelOffsetX = ccl_to_int (pixelOffsetX * scaleFactor);
					pixelOffsetY = ccl_to_int (pixelOffsetY * scaleFactor);
				}

				BitmapPrimitives32::clear (dstLocker.data);
				BitmapPrimitives32::copyPart (dstLocker.data, srcLocker.data, pixelOffsetX, pixelOffsetY);
				return kResultOk;
			};

			if(srcMultiBitmap)
			{
				UnknownPtr<IMultiResolutionBitmap> dstMultiBitmap (dstBitmap);
				ASSERT (dstMultiBitmap && srcMultiBitmap->getRepresentationCount () == dstMultiBitmap->getRepresentationCount ())
				for(int i = 0; i < dstMultiBitmap->getRepresentationCount (); i++)
				{
					IMultiResolutionBitmap::RepSelector srcSelector (srcMultiBitmap, i);
					IMultiResolutionBitmap::RepSelector dstSelector (dstMultiBitmap, i);
					tresult tr = copyPart ();
					if(tr != kResultOk)
						return tr;
				}
			}
			else
			{
				tresult tr = copyPart ();
				if(tr != kResultOk)
					return tr;
			}
		}

		srcBitmap.share (dstBitmap);
	}
	else if(inplace == false && copied == false) // do not modify original
		dstBitmap = createBitmap (srcImage->getWidth (), srcImage->getHeight ());
	else
		dstBitmap.share (srcBitmap);
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IImage* CCL_API BitmapProcessor::getOutput ()
{
	UnknownPtr<IImage> dstImage (dstBitmap);
	ASSERT (dstImage)
	return dstImage;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API BitmapProcessor::process (IBitmapFilter& filter)
{
	ASSERT (srcBitmap && dstBitmap)
	if(!srcBitmap || !dstBitmap)
		return kResultUnexpected;

	if(srcBitmap == dstBitmap) // inplace processing
	{
		auto processInplace = [&] ()
		{
			BitmapDataLocker locker (srcBitmap, IBitmap::kRGBAlpha, IBitmap::kLockWrite); // read+write
			if(locker.result != kResultOk)
				return locker.result;

			return filter.processData (locker.data, locker.data);
		};

		UnknownPtr<IMultiResolutionBitmap> multiBitmap (srcBitmap);
		if(multiBitmap)
		{
			tresult tr = kResultOk;
			for(int i = 0; i < multiBitmap->getRepresentationCount (); i++)
			{
				IMultiResolutionBitmap::RepSelector selector (multiBitmap, i);
				tr = processInplace ();
				if(tr != kResultOk)
					break;
			}
			return tr;
		}
		else
			return processInplace ();
	}
	else
	{
		auto process = [&] ()
		{
			BitmapDataLocker srcLocker (srcBitmap, IBitmap::kRGBAlpha, IBitmap::kLockRead);
			if(srcLocker.result != kResultOk)
				return srcLocker.result;
		
			BitmapDataLocker dstLocker (dstBitmap, IBitmap::kRGBAlpha, IBitmap::kLockWrite);
			if(dstLocker.result != kResultOk)
				return dstLocker.result;

			return filter.processData (dstLocker.data, srcLocker.data);
		};

		UnknownPtr<IMultiResolutionBitmap> srcMultiBitmap (srcBitmap);
		UnknownPtr<IMultiResolutionBitmap> dstMultiBitmap (dstBitmap);
		if(srcMultiBitmap && dstMultiBitmap)
		{
			tresult tr = kResultOk;
			ASSERT (srcMultiBitmap->getRepresentationCount () == dstMultiBitmap->getRepresentationCount ())
			for(int i = 0; i < srcMultiBitmap->getRepresentationCount (); i++)
			{
				IMultiResolutionBitmap::RepSelector srcSelector (srcMultiBitmap, i);
				IMultiResolutionBitmap::RepSelector dstSelector (dstMultiBitmap, i);
				tr = process ();
				if(tr != kResultOk)
					break;
			}
			return tr;
		}
		else
			return process ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API BitmapProcessor::reset ()
{
	srcBitmap.release ();
	dstBitmap.release ();
}
