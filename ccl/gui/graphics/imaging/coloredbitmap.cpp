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
// Filename    : ccl/gui/graphics/imaging/coloredbitmap.cpp
// Description : Colored Bitmap
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "ccl/gui/graphics/imaging/coloredbitmap.h"
#include "ccl/gui/graphics/imaging/bitmappainter.h"
#include "ccl/gui/graphics/imaging/imagecache.h"

#include "ccl/gui/theme/colorscheme.h"

namespace CCL {

//************************************************************************************************
// ColoredSchemeBitmap::WrappedFilter
//************************************************************************************************

class ColoredSchemeBitmap::WrappedFilter: public BitmapFilter
{
public:
	DECLARE_CLASS_ABSTRACT (WrappedFilter, BitmapFilter)

	WrappedFilter (ColoredSchemeBitmap& bitmap, IBitmapFilter* filter, ColorScheme* scheme, StringID nameInScheme);
	~WrappedFilter ();

	ColorScheme* getScheme () const { return scheme; }

	// BitmapFilter
	void CCL_API notify (ISubject* subject, MessageRef msg) override;
	tresult CCL_API processData (BitmapData& dstData, const BitmapData& srcData) override;

protected:
	ColoredSchemeBitmap& bitmap;
	ColorScheme* scheme;
	MutableCString nameInScheme;
	AutoPtr<IBitmapFilter> filter;

	void updateColor ();
};

} // namespace CCL

using namespace CCL;
	
//************************************************************************************************
// ModifiedBitmap
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (ModifiedBitmap, Image)
	
//////////////////////////////////////////////////////////////////////////////////////////////////

ModifiedBitmap::ModifiedBitmap (IImage* source)
: sourceImage (source)
{
	ASSERT (sourceImage != nullptr)
	if(sourceImage)
	{
		size.x = sourceImage->getWidth ();
		size.y = sourceImage->getHeight ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IImage::ImageType CCL_API ModifiedBitmap::getType () const
{
	return sourceImage->getType ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Image* ModifiedBitmap::getOriginalImage (Rect& originalRect, bool deep)
{
	getSize (originalRect);
	return resolveOriginal (unknown_cast<Image> (sourceImage), originalRect, deep);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult ModifiedBitmap::draw (GraphicsDevice& graphics, PointRef pos, const ImageMode* mode)
{
	if(Image* modified = getModifiedImage ())
		return modified->draw (graphics, pos, mode);
	return kResultInvalidPointer;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult ModifiedBitmap::draw (GraphicsDevice& graphics, PointFRef pos, const ImageMode* mode)
{
	if(Image* modified = getModifiedImage ())
		return modified->draw (graphics, pos, mode);
	return kResultInvalidPointer;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult ModifiedBitmap::draw (GraphicsDevice& graphics, RectRef src, RectRef dst, const ImageMode* mode)
{
	if(Image* modified = getModifiedImage ())
		return modified->draw (graphics, src, dst, mode);
	return kResultInvalidPointer;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult ModifiedBitmap::draw (GraphicsDevice& graphics, RectFRef src, RectFRef dst, const ImageMode* mode)
{
	if(Image* modified = getModifiedImage ())
		return modified->draw (graphics, src, dst, mode);
	return kResultInvalidPointer;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult ModifiedBitmap::tile (GraphicsDevice& graphics, int method, RectRef src, RectRef dest, RectRef clip, RectRef margins)
{
	if(Image* modified = getModifiedImage ())
		return modified->tile (graphics, method, src, dest, clip, margins);
	return kResultInvalidPointer;
}

//************************************************************************************************
// ColoredBitmap
//************************************************************************************************

DEFINE_CLASS (ColoredBitmap, ModifiedBitmap)

//////////////////////////////////////////////////////////////////////////////////////////////////

ColoredBitmap::ColoredBitmap (IImage* source, ColorRef color)
: ModifiedBitmap (source),
  cachedColor (color),
  defaultColor (color)
{
	if(defaultColor.getAlphaF () == 0)
		defaultColor = Colors::kBlack;
}
	
//////////////////////////////////////////////////////////////////////////////////////////////////

Image* ColoredBitmap::getModifiedImage ()
{
	if(sourceImage == nullptr)
		return nullptr;
	
	if(modifiedImage)
		return modifiedImage;
	
	modifiedImage = unknown_cast<Image> (ModifiedImageCache::createModifiedImage (sourceImage, createBitmapFilter ()));
	
	return modifiedImage;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

BitmapFilter* ColoredBitmap::createBitmapFilter ()
{
	BitmapFilters::Colorizer* colorizer =  NEW BitmapFilters::Colorizer;
	colorizer->setColor (cachedColor);
	return colorizer;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ColoredBitmap::setColor (const Color& color)
{
	if(cachedColor != color)
	{
		modifiedImage = nullptr;
		cachedColor = color;
		if(cachedColor.getAlphaF () == 0)
			cachedColor = defaultColor;
	}
}

//************************************************************************************************
// TintedBitmap
//************************************************************************************************

DEFINE_CLASS (TintedBitmap, ColoredBitmap)

//////////////////////////////////////////////////////////////////////////////////////////////////

TintedBitmap::TintedBitmap (IImage* sourceImage, ColorRef color)
: ColoredBitmap (sourceImage, color)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

BitmapFilter* TintedBitmap::createBitmapFilter ()
{
	BitmapFilters::Tinter* tinter =  NEW BitmapFilters::Tinter;
	tinter->setColor (cachedColor);
	return tinter;
}

//************************************************************************************************
// LightAdaptedBitmap
//************************************************************************************************

DEFINE_CLASS (LightAdaptedBitmap, ColoredBitmap)

//////////////////////////////////////////////////////////////////////////////////////////////////

LightAdaptedBitmap::LightAdaptedBitmap (IImage* sourceImage, ColorRef color)
: ColoredBitmap (sourceImage, color)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

BitmapFilter* LightAdaptedBitmap::createBitmapFilter ()
{
	BitmapFilters::LightAdapter* lightAdapter =  NEW BitmapFilters::LightAdapter;
	lightAdapter->setColor (cachedColor);
	return lightAdapter;
}

//************************************************************************************************
// ColoredSchemeBitmap::WrappedFilter
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (ColoredSchemeBitmap::WrappedFilter, BitmapFilter)

//////////////////////////////////////////////////////////////////////////////////////////////////

ColoredSchemeBitmap::WrappedFilter::WrappedFilter (ColoredSchemeBitmap& bitmap, IBitmapFilter* filter, ColorScheme* scheme, StringID nameInScheme)
: bitmap (bitmap),
  filter (filter),
  scheme (scheme),
  nameInScheme (nameInScheme)
{
	ASSERT (filter != nullptr && scheme != nullptr)
	scheme->retain ();
	scheme->addObserver (this);
	updateColor ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ColoredSchemeBitmap::WrappedFilter::~WrappedFilter ()
{
	scheme->removeObserver (this);
	scheme->release ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ColoredSchemeBitmap::WrappedFilter::updateColor ()
{
	Color color = scheme->getColor (nameInScheme);
	UnknownPtr<IObject> (filter)->setProperty (IBitmapFilter::kColorID, (int)(uint32)color);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API ColoredSchemeBitmap::WrappedFilter::notify (ISubject* subject, MessageRef msg)
{
	updateColor ();
	bitmap.setImageUpdateNeeded ();

	#if (0 && DEBUG) // for testing only
	bitmap.getModifiedImage ();
	#endif
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API ColoredSchemeBitmap::WrappedFilter::processData (BitmapData& dstData, const BitmapData& srcData)
{
	return filter->processData (dstData, srcData);
}

//************************************************************************************************
// ColoredSchemeBitmap
//************************************************************************************************

DEFINE_CLASS_HIDDEN (ColoredSchemeBitmap, ColoredBitmap)

//////////////////////////////////////////////////////////////////////////////////////////////////

ColoredSchemeBitmap::ColoredSchemeBitmap (Image* sourceImage)
: ColoredBitmap (sourceImage)
{
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API ColoredSchemeBitmap::queryInterface (UIDRef iid, void** ptr)
{
	if(iid == ccl_iid<IBitmap> () || iid == ccl_iid<IMultiResolutionBitmap> ())
	{
		return getModifiedImage ()->queryInterface (iid, ptr);
	}
	return SuperClass::queryInterface (iid, ptr);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ColoredSchemeBitmap::~ColoredSchemeBitmap ()
{
	CCL_PRINTLN ("ColoredSchemeBitmap dtor")
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ColoredSchemeBitmap::addFilter (IBitmapFilter* filter)
{
	filterList.addFilter (filter);
	setImageUpdateNeeded ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ColoredSchemeBitmap::addFilter (IBitmapFilter* filter, ColorScheme* scheme, StringID nameInScheme)
{
	filterList.addFilter (NEW BitmapFilters::RevertPremultipliedAlpha);
	filterList.addFilter (NEW WrappedFilter (*this, filter, scheme, nameInScheme));
	filterList.addFilter (NEW BitmapFilters::PremultipliedAlpha);
	setImageUpdateNeeded ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ColoredSchemeBitmap::hasReferences (IColorScheme& scheme) const
{
	for(int i = 0; i < filterList.count (); i++)
		if(auto filter = unknown_cast<WrappedFilter> (filterList.at (i)))
			if(filter->getScheme () == &scheme)
				return true;
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ColoredSchemeBitmap::setImageUpdateNeeded ()
{
	modifiedImage = nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Image* ColoredSchemeBitmap::getModifiedImage ()
{
	if(modifiedImage == nullptr)
	{
		BitmapProcessor processor;
		processor.setup (sourceImage, Colors::kWhite);
		modifiedImage.share (unknown_cast<Image> (processor.getOutput ()));
		ASSERT (modifiedImage != nullptr)

		// apply filters
		processor.process (static_cast<IBitmapFilterList&> (filterList));
	}

	if(modifiedImage == nullptr)
	{
		// should only happen if no filters specified, i.e. source image is used unchanged.
		ASSERT (filterList.count () == 0)
		return unknown_cast<Image> (sourceImage);
	}

	return modifiedImage;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IImage::ImageType CCL_API ColoredSchemeBitmap::getType () const
{
	return kBitmap;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Image* ColoredSchemeBitmap::getOriginalImage (Rect& originalRect, bool deep)
{
	// quick fix: don't resolve to bitmap for ColorScheme::hasReferences() to work correctly.
	return SuperClass::getOriginalImage (originalRect, deep);
	//getSize (originalRect);
	//return getModifiedImage ();
}
