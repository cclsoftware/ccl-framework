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
// Filename    : ccl/app/utilities/imagebuilder.cpp
// Description : Image Builder
//
//************************************************************************************************

#include "ccl/app/utilities/imagebuilder.h"

#include "ccl/public/base/iobject.h"
#include "ccl/public/base/variant.h"

#include "ccl/public/gui/graphics/graphicsfactory.h"
#include "ccl/public/gui/graphics/iconsetformat.h"
#include "ccl/public/gui/graphics/igraphics.h"
#include "ccl/public/gui/graphics/dpiscale.h"
#include "ccl/public/gui/graphics/ibitmapfilter.h"

#include "ccl/public/gui/framework/iwindow.h"
#include "ccl/public/guiservices.h"
#include "ccl/public/plugservices.h"

using namespace CCL;

//************************************************************************************************
// ImageBuilder
//************************************************************************************************

bool ImageBuilder::isHighResolutionImageNeeded ()
{
	for(int i = 0; i < System::GetDesktop ().countMonitors (); i++)
		if(System::GetDesktop ().getMonitorScaleFactor (i) > 1.f)
			return true;
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IImage* ImageBuilder::createBitmapCopy (IImage* sourceImage)
{
	IBitmap::PixelFormat pixelFormat = IBitmap::kRGBAlpha;
	float scaleFactor = 1.f;
	if(UnknownPtr<IBitmap> sourceBitmap = sourceImage)
	{
		pixelFormat = sourceBitmap->getPixelFormat ();
		scaleFactor = sourceBitmap->getContentScaleFactor ();
	}

	AutoPtr<IImage> destImage = GraphicsFactory::createBitmap (sourceImage->getWidth (), sourceImage->getHeight (), pixelFormat, scaleFactor);
	AutoPtr<IGraphics> g = GraphicsFactory::createBitmapGraphics (destImage);
	ImageMode mode (1.f, ImageMode::kInterpolationHighQuality);
	g->drawImage (sourceImage, Point (), &mode);

	return destImage.detach ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IImage* ImageBuilder::createSizedImage (IImage* sourceImage, int width, int height, float scaleFactor)
{
	Rect srcRect (0, 0, sourceImage->getWidth (), sourceImage->getHeight ());
	Rect newRect (0, 0, width, height);

	Rect dstRect (srcRect);
	dstRect.fitProportionally (newRect);
	dstRect.center (newRect);

	AutoPtr<IImage> destImage = GraphicsFactory::createBitmap (width, height, IBitmap::kRGBAlpha, scaleFactor);
	AutoPtr<IGraphics> g = GraphicsFactory::createBitmapGraphics (destImage);
	ImageMode mode (1.f, ImageMode::kInterpolationHighQuality);
	g->drawImage (sourceImage, srcRect, dstRect, &mode);
	
	return destImage.detach ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IImage* ImageBuilder::createBlurredImage (IImage* source, float blurFactor, int width, int height, bool saturate)
{
	UnknownPtr<IBitmap> sourceBitmap (source);
	ASSERT (sourceBitmap)
	if(!sourceBitmap)
		return nullptr;

	AutoPtr<IImage> limitedSource = GraphicsFactory::createBitmap (width, height, sourceBitmap->getPixelFormat (), sourceBitmap->getContentScaleFactor ());
	
	if(AutoPtr<IGraphics> limitedSourceGraphics = GraphicsFactory::createBitmapGraphics (limitedSource))
	{
		Rect src (0, 0, source->getWidth (), source->getHeight ());
		Rect dst (Rect (0, 0, width, height));
		ImageMode mode (1.0f, ImageMode::kInterpolationHighQuality);
		limitedSourceGraphics->drawImage (source, src, dst, &mode);
	}
	
	AutoPtr<IBitmapFilter> filter = GraphicsFactory::createBitmapFilter (BitmapFilters::kFilterList);
	UnknownPtr<IBitmapFilterList> filterList (filter);
	ASSERT (filterList.isValid ())
	
	if(saturate)
	{
		IBitmapFilter* saturator = GraphicsFactory::createBitmapFilter (BitmapFilters::kSaturator);
		UnknownPtr<IObject> (saturator)->setProperty (IBitmapFilter::kValueID, 0.6f);
		filterList->addFilter (saturator);
	}
	
	IBitmapFilter* blurX = GraphicsFactory::createBitmapFilter (BitmapFilters::kBlurX);
	UnknownPtr<IObject> (blurX)->setProperty (IBitmapFilter::kValueID, blurFactor);
	filterList->addFilter (blurX);
	
	IBitmapFilter* blurY = GraphicsFactory::createBitmapFilter (BitmapFilters::kBlurY);
	UnknownPtr<IObject> (blurY)->setProperty (IBitmapFilter::kValueID, blurFactor);
	filterList->addFilter (blurY);
	
	AutoPtr<IBitmapProcessor> processor = ccl_new<IBitmapProcessor> (ClassID::BitmapProcessor);
	processor->setup (limitedSource, Colors::kBlack);
	processor->process (*filterList);
	
	SharedPtr<IImage> blurredImage = processor->getOutput ();
	return blurredImage.detach ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IImage* ImageBuilder::createIconSet (IImage* sourceImage, int sizeIDList)
{
	ASSERT (sizeIDList != 0)

	const int kNumScalings = 2;
	float scaleFactors[kNumScalings] = {1.f, 2.f};

	int sourceSize = ccl_max (sourceImage->getWidth (), sourceImage->getHeight ());

	Vector<IImage*> images;
	Vector<CString> frameNames;
	for(int sizeIndex = 0; sizeIndex < IconSetFormat::kIconSizesAll; sizeIndex++)
	{
		if(sizeIDList & (1<<sizeIndex))
		{
			const IconSetFormat::IconSize& iconSize = IconSetFormat::getIconSizeAt (sizeIndex);
			if(iconSize.size > sourceSize)
				continue;

			int hiResPixelSize = DpiScale::coordToPixel (iconSize.size, 2.f);
			if(sourceSize >= hiResPixelSize) // create 2x version
			{
				IImage* bitmaps[kNumScalings] = {nullptr};
				for(int i = 0; i < kNumScalings; i++)
					bitmaps[i] = createSizedImage (sourceImage, iconSize.size, iconSize.size, scaleFactors[i]);

				IImage* multiBitmap = GraphicsFactory::createMultiResolutionBitmap (bitmaps, scaleFactors, kNumScalings);
				ASSERT (multiBitmap != nullptr)
				images.add (multiBitmap);
				frameNames.add (iconSize.name);

				for(int i = 0; i < kNumScalings; i++)
					bitmaps[i]->release ();
			}
			else
			{
				IImage* bitmap = createSizedImage (sourceImage, iconSize.size, iconSize.size, 1.f);
				images.add (bitmap);
				frameNames.add (iconSize.name);
			}
		}
	}

	ASSERT (!images.isEmpty ())
	if(images.isEmpty ())
		return nullptr;
	
	IImage* multiImage = GraphicsFactory::createMultiImage (images, frameNames, images.count ());
	
	VectorForEach (images, IImage*, image)
		image->release ();
	EndFor

	return multiImage;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IImage* ImageBuilder::createThumbnail (IImage* sourceImage, float scaleFactor, int flags)
{
	int width = kThumbnailSize;
	int height = kThumbnailSize;

	if(flags & kKeepAspectRatio)
	{
		if(sourceImage->getWidth () > sourceImage->getHeight ())
			height = kThumbnailSize * sourceImage->getHeight () / sourceImage->getWidth ();
		else if(sourceImage->getHeight () > sourceImage->getWidth ())
			width = kThumbnailSize * sourceImage->getWidth () / sourceImage->getHeight ();
	}

	return createSizedImage (sourceImage, width, height, scaleFactor);
}
