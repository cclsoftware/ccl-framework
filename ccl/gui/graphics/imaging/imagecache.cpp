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
// Filename    : ccl/gui/graphics/imaging/imagecache.cpp
// Description : Image cache
//
//************************************************************************************************

#include "ccl/gui/graphics/imaging/imagecache.h"

#include "ccl/gui/graphics/imaging/bitmap.h"
#include "ccl/gui/graphics/imaging/filmstrip.h"
#include "ccl/gui/graphics/imaging/multiimage.h"
#include "ccl/gui/graphics/imaging/bitmapfilter.h"
#include "ccl/gui/graphics/imaging/bitmappainter.h"
#include "ccl/gui/graphics/imaging/tiledimage.h"

#include "ccl/gui/graphics/graphicsdevice.h"
#include "ccl/gui/graphics/nativegraphics.h"

#include "ccl/gui/graphics/shapes/shapeimage.h"
#include "ccl/gui/graphics/shapes/shapes.h"

using namespace CCL;

//************************************************************************************************
// ImageCache
//************************************************************************************************

DEFINE_SINGLETON (ImageCache)
DEFINE_CLASS (ImageCache, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

ImageCache::ImageCache ()
{
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ImageCache::~ImageCache ()
{
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IImage* ImageCache::requestImage (IImage* source, Coord width, Coord height)
{
	ListIterator<CacheEntry> iter (cache);
	while(!iter.done ())
	{
		CacheEntry& entry = iter.next ();
		if(entry.source == source && entry.width == width && entry.height == height)
		{
			entry.useCount++;
			return entry.cached;
		}
	}

	// try to keep alpha channel intact
	bool useAlpha = true;
	Bitmap* bitmap = nullptr;
	int frames = source->getFrameCount ();
		
	auto drawSourceToBitmap = [source, height, width](Bitmap* bitmap)
	{
		Coord w = source->getWidth ();
		Coord h = source->getHeight ();
		int frames = source->getFrameCount ();
	
		BitmapGraphicsDevice device (bitmap);
		Rect sr (0, 0, w, h);
		Rect dr (0, 0, width, 0);

		for(int i = 0; i < frames; i++)
		{
			source->setCurrentFrame (i);
			dr.top = height * i;
			dr.bottom = dr.top + height;

			ImageMode mode (ImageMode::kInterpolationHighQuality);
			device.drawImage (source, sr, dr, &mode);
		}
	};
	
	UnknownPtr<IMultiResolutionBitmap> srcMultiBitmap (source);
	if(srcMultiBitmap || source->getType () == IImage::kScalable)
	{
		AutoPtr<Bitmap> bitmap1 = NEW Bitmap (width, height * frames, useAlpha ? IBitmap::kRGBAlpha : IBitmap::kRGB);
		{
			drawSourceToBitmap (bitmap1);
		}
		AutoPtr<Bitmap> bitmap2 = NEW Bitmap (width, height * frames, useAlpha ? IBitmap::kRGBAlpha : IBitmap::kRGB, 2.f);
		{
			drawSourceToBitmap (bitmap2);
		}
		bitmap = NEW MultiResolutionBitmap (bitmap1->getNativeBitmap (), bitmap2->getNativeBitmap ());
	}
	else
	{
		bitmap = NEW Bitmap (width, height * frames, useAlpha ? IBitmap::kRGBAlpha : IBitmap::kRGB);
		drawSourceToBitmap (bitmap);
	}
	
	IImage* image = nullptr;
	
	if(frames > 1)
	{
		Filmstrip* newStrip = NEW Filmstrip (bitmap, frames);

		if(Filmstrip* sourceStrip = unknown_cast<Filmstrip> (source))
		{
			for(int i = 0; i < frames; i++)
				newStrip->setFrameName (i, sourceStrip->getFrameName (i));
		}

		image = newStrip;
		bitmap->release (); // Is now owned by filmstrip
	}
	else
		image = bitmap;

	CacheEntry entry;
	entry.useCount = 1;
	entry.source = source;
	entry.cached = image;
	entry.width = width;
	entry.height = height;
	cache.append (entry);

	return image;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ImageCache::releaseImage (IImage* cached)
{
	ListIterator<CacheEntry> iter (cache);
	while(!iter.done ())
	{
		CacheEntry& entry = iter.next ();
		if(entry.cached == cached)
		{
			if(--entry.useCount == 0)
			{
				entry.cached->release ();
				cache.remove (entry);
			}
			break;
		}
	}
}

//************************************************************************************************
// CachedImage
//************************************************************************************************

CachedImage::CachedImage ()
: cached (nullptr)
{
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CachedImage::~CachedImage ()
{
	if(cached)
		ImageCache::instance ().releaseImage (cached);
}

//////////////////////////////////////////////////////////////////////////////////////////////////
	
IImage* CachedImage::operator = (IImage* image)
{
	if(cached)
		ImageCache::instance ().releaseImage (cached);
	source = image;
	return image;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CachedImage::operator IImage* ()
{
	return cached != nullptr ? cached : (IImage*)source;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IImage* CachedImage::operator -> ()
{
	return cached != nullptr ? cached : (IImage*)source;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool CachedImage::operator == (IImage* image)
{
	return source == image;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CachedImage::operator bool ()
{
	return source != nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CachedImage::update (Coord width, Coord height)
{
	if(source == nullptr)
		return;
	if(source->getWidth () == width && source->getHeight () == height)
	{
		if(cached)
		{
			ImageCache::instance ().releaseImage (cached);
			cached = nullptr;
		}
		return;
	}

	if(cached == nullptr || cached->getWidth () != width || cached->getHeight () != height)
	{
		if(cached)
			ImageCache::instance ().releaseImage (cached);
		cached = ImageCache::instance ().requestImage (source, width, height);
	}
}


//************************************************************************************************
// ModifiedImageCache
//************************************************************************************************

DEFINE_SINGLETON (ModifiedImageCache)
DEFINE_SINGLETON_CLASS (ModifiedImageCache, Object)
DEFINE_CLASS_UID (ModifiedImageCache, 0x9bc5ef85, 0x43cd, 0x4b45, 0xaf, 0x42, 0x48, 0x3c, 0x24, 0xae, 0x41, 0x8a)

//////////////////////////////////////////////////////////////////////////////////////////////////

ModifiedImageCache::ModifiedImageCache ()
{
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ModifiedImageCache::~ModifiedImageCache ()
{
	removeAll ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ModifiedImageCache::removeAll ()
{
	ListIterator<CacheEntry> iter (cache);
	while(!iter.done ())
	{
		CacheEntry& entry = iter.next ();

		for(int i = 0; i < entry.modifications.count (); i++)
			entry.modifications[i].image->release ();
	}
	cache.removeAll ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IImage* CCL_API ModifiedImageCache::lookup (IImage* source, ColorRef color, tbool drawAsTemplate)
{
	if(source == nullptr)
		return nullptr;
	
	if(Image* sourceImage = unknown_cast<Image> (source))
		if(sourceImage->getIsTemplate ())
			drawAsTemplate = true;
	
	if(MultiImage* multiImage = unknown_cast<MultiImage> (source)) 		// special multiImage case...
		source = multiImage->getFrame (multiImage->getCurrentFrame ());	// ...use current frame as source

	if(source == nullptr)
		return nullptr;
	
	IImage* modifiedImage = nullptr;
	
	ListIterator<CacheEntry> iter (cache);
	while(!iter.done ())
	{
		CacheEntry& entry = iter.next ();
		
		if(entry.source == source)
		{
			for(int i = 0; i < entry.modifications.count (); i++)
			{
				const CachedModification& modification = entry.modifications[i];
				if(modification.color == color)
				{
					modifiedImage = modification.image;	//modification found
					break;
				}
			}
			
			// create new modification in existing entry (dispose oldest modification)
			if(!modifiedImage)
			{
				if(entry.modifications.isFull ())
				{
					entry.modifications[0].image->release ();
					entry.modifications.removeFirst ();
				}
				modifiedImage = addModification (entry, color, drawAsTemplate);
			}
			
			if(modifiedImage)
				break;
		}
	}

	if(modifiedImage)
	{
		modifiedImage->setCurrentFrame (source->getCurrentFrame ());
	}
	else // create new entry with modification
	{
		CacheEntry entry (source);

		if(modifiedImage = addModification (entry, color, drawAsTemplate))
		{
			cache.append (entry);
			modifiedImage->setCurrentFrame (source->getCurrentFrame ());
		}
	}
	
	return modifiedImage;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IImage* ModifiedImageCache::addModification (CacheEntry& entry, ColorRef color, bool colorizeTemplate)
{
	if(entry.source->getType () == IImage::kScalable)
	{
		IImage* image = createModifiedShape (entry.source, color, colorizeTemplate);
		
		if(image)
			entry.modifications.add (CachedModification (image, color));
		
		return image;
	}
		
	BitmapFilter* filter = nullptr;
	if(colorizeTemplate)
	{
		BitmapFilters::Colorizer* colorizer = NEW BitmapFilters::Colorizer;
		colorizer->setColor (color);
		filter = colorizer;
	}
	else
	{
		BitmapFilters::LightAdapter* lightAdapter = NEW BitmapFilters::LightAdapter;
		lightAdapter->setColor (color);
		filter = lightAdapter;
	}
	
	IImage* image = createModifiedImage (entry.source, filter);
	
	if(image)
		entry.modifications.add (CachedModification (image, color));
	
	return image;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IImage* ModifiedImageCache::createModifiedImage (IImage* source, BitmapFilter* ownedFilter)
{
	BitmapProcessor processor;
	
	int filmstripFrameCount = 0;
	Rect originalRect;
	
	Filmstrip* filmstrip = nullptr;
	Filmstrip::FrameMode filmstripFrameMode = Filmstrip::kVerticalMode; 
	
	if(Image* image = unknown_cast<Image> (source))
	{
		if(filmstrip = ccl_cast<Filmstrip> (image->getOriginalImage (originalRect)))
		{
			filmstripFrameMode = filmstrip->getFrameMode ();
			filmstripFrameCount = filmstrip->getFrameCount ();
		}	
		else if(filmstrip = ccl_cast<Filmstrip> (image))
		{
			filmstripFrameMode = filmstrip->getFrameMode ();
			filmstripFrameCount = filmstrip->getFrameCount ();
		}
	}
	
	processor.setup (filmstrip ? filmstrip->getOriginalImage (originalRect) : source, Colors::kWhite);
	
	AutoPtr<IBitmapFilterList> filterList = NEW BitmapFilterList;
	filterList->addFilter (NEW BitmapFilters::RevertPremultipliedAlpha);
	
	filterList->addFilter (ownedFilter);
	
	filterList->addFilter (NEW BitmapFilters::PremultipliedAlpha);
	processor.process (*filterList);
	
	if(TiledImage* tiledImage = unknown_cast<TiledImage> (source))
	{
		if(filmstripFrameCount)
		{
			AutoPtr<Filmstrip> filmstrip = NEW Filmstrip (unknown_cast<Image> (processor.getOutput ()), filmstripFrameCount);
			return NEW TiledImage (filmstrip, tiledImage->getMethod (), tiledImage->getMargins ());
		}
		else
		{
			return NEW TiledImage (unknown_cast<Image> (processor.getOutput ()), tiledImage->getMethod (), tiledImage->getMargins ());
		}
	}
	else
	{
		if(filmstripFrameCount)
		{
			return NEW Filmstrip (unknown_cast<Image> (processor.getOutput ()), filmstripFrameCount, filmstripFrameMode);
		}
		else
		{
			Image* processed = unknown_cast<Image> (processor.getOutput ());
			if(processed)
				processed->retain ();
			return processed;
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IImage* ModifiedImageCache::createModifiedShape (IImage* source, ColorRef color, bool colorizeTemplate)
{
	ShapeImage* shapeImage = unknown_cast<ShapeImage> (source);
	Shape* shape = shapeImage ? shapeImage->getShape () : nullptr;
	
	AutoPtr<Shape> newShape (shape ? static_cast<Shape*> (shape->clone ()) : nullptr);
	if(newShape)
	{
		applyShapeModificationDeep (color, newShape, colorizeTemplate);
		return NEW ShapeImage (newShape);
	}

	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ModifiedImageCache::applyShapeModificationDeep (ColorRef color, Shape* shape, bool colorizeTemplate)
{
	if(shape->countShapes () == 0)
	{
		auto getModifiedColor = [=](ColorRef srcColor, ColorRef color)
		{
			if(colorizeTemplate)
			{
				Color fillColor (color);
				fillColor.setAlphaF (srcColor.getAlphaF ());
				return fillColor;
			}
				
			Color fillColor (color);
			ColorHSL hslColor (color);
			ColorHSL srcHSLColor (srcColor);
			
			bool isLightColor = (hslColor.l > 0.5f) ? true : false;
	
			if(isLightColor)	// adapt light pixels
			{
				float lumSub = (1.f - hslColor.l);
	
				if(srcHSLColor.l > 0.5f)
					srcHSLColor.l -= (lumSub * ((srcHSLColor.l - 0.5f) * 2));
	
			}
			else				// adapt light pixels and invert
			{
				float lumSub = hslColor.l;

				if(srcHSLColor.l > 0.5f)
					srcHSLColor.l -= (lumSub * ((srcHSLColor.l - 0.5f) * 2));
	
				srcHSLColor.l = 1.f - (srcHSLColor.l);
			}
			
			srcHSLColor.toColor (fillColor);
			
			return fillColor;
		};
		
		Pen strokePen (shape->getStrokePen ());
		strokePen.setColor (getModifiedColor (strokePen.getColor (), color));
		shape->setStrokePen (strokePen);
	
		SolidBrush fillBrush (shape->getFillBrush ().getColor ());
		fillBrush.setColor (getModifiedColor (fillBrush.getColor (), color));
		shape->setFillBrush (fillBrush);
	}
	else
	{
		int subShapesCount = shape->countShapes ();
		for(int i = 0; i < subShapesCount; i++)
			applyShapeModificationDeep (color, shape->getShape (i), colorizeTemplate);
	}
}


