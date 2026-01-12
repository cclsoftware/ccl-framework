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
// Filename    : ccl/public/gui/graphics/graphicsfactory.cpp
// Description : Graphics factory funtions
//
//************************************************************************************************

#include "ccl/public/gui/graphics/graphicsfactory.h"
#include "ccl/public/gui/graphics/igraphicshelper.h"
#include "ccl/public/gui/graphics/igraphics.h"
#include "ccl/public/gui/graphics/igraphicslayer.h"
#include "ccl/public/gui/graphics/ibitmapfilter.h"

#include "ccl/public/base/iobject.h"
#include "ccl/public/base/variant.h"

#include "ccl/public/plugservices.h"

using namespace CCL;

//************************************************************************************************
// GraphicsFactory
//************************************************************************************************

int GraphicsFactory::getNumImageFormats ()
{
	return System::GetGraphicsHelper ().Factory_getNumImageFormats ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const FileType* GraphicsFactory::getImageFormat (int index)
{
	return System::GetGraphicsHelper ().Factory_getImageFormat (index);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IImage* GraphicsFactory::loadImageFile (UrlRef path)
{
	return System::GetGraphicsHelper ().Factory_loadImageFile (path);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool GraphicsFactory::saveImageFile (UrlRef path, IImage* image, const IAttributeList* encoderOptions)
{
	return System::GetGraphicsHelper ().Factory_saveImageFile (path, image, encoderOptions) != 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IImage* GraphicsFactory::loadImageStream (IStream& stream, const FileType& format)
{
	return System::GetGraphicsHelper ().Factory_loadImageStream (stream, format);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool GraphicsFactory::saveImageStream (IStream& stream, IImage* image, const FileType& format,
									   const IAttributeList* encoderOptions)
{
	return System::GetGraphicsHelper ().Factory_saveImageStream (stream, image, format, encoderOptions) != 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IImage* GraphicsFactory::createBitmap (int width, int height, IBitmap::PixelFormat format, float scaleFactor)
{
	return System::GetGraphicsHelper ().Factory_createBitmap (width, height, format, scaleFactor);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IGraphics* GraphicsFactory::createBitmapGraphics (IImage* bitmap)
{
	return System::GetGraphicsHelper ().Factory_createBitmapGraphics (bitmap);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IBitmapFilter* GraphicsFactory::createBitmapFilter (StringID which)
{
	return System::GetGraphicsHelper ().Factory_createBitmapFilter (which);	
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IBitmapFilter* GraphicsFactory::createBitmapColorizationFilter (Color color)
{
	IBitmapFilter* filter = createBitmapFilter (BitmapFilters::kFilterList);
	UnknownPtr<IBitmapFilterList> filterList (filter);
	ASSERT (filterList.isValid ())
	
	IBitmapFilter* colorizer = createBitmapFilter (BitmapFilters::kColorize);
	UnknownPtr<IObject> (colorizer)->setProperty (IBitmapFilter::kColorID, (int)(uint32)color);
	
	filterList->addFilter (createBitmapFilter (BitmapFilters::kRevertPremulAlpha));
	filterList->addFilter (colorizer);
	filterList->addFilter (createBitmapFilter (BitmapFilters::kPremultiplyAlpha));	
	return filter;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IImage* GraphicsFactory::createSolidBitmap (Color color, int width, int height, IBitmap::PixelFormat format, float scaleFactor)
{
	IImage* bitmap = createBitmap (width, height, format, scaleFactor);
	
	AutoPtr<IGraphics> graphics = createBitmapGraphics (bitmap);
	ASSERT (graphics != nullptr)
	Rect rect (Rect (0, 0, width, height));
	graphics->fillRect (rect, SolidBrush (color));

	return bitmap;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IGraphicsPath* GraphicsFactory::createPath (IGraphicsPath::TypeHint type)
{
	return System::GetGraphicsHelper ().Factory_createPath (type);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IGradient* GraphicsFactory::createGradient (IGradient::TypeHint type)
{
	return System::GetGraphicsHelper ().Factory_createGradient (type);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IImage* GraphicsFactory::createShapeImage ()
{
	return System::GetGraphicsHelper ().Factory_createShapeImage ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IGraphics* GraphicsFactory::createShapeBuilder (IImage* shapeImage)
{
	return System::GetGraphicsHelper ().Factory_createShapeBuilder (shapeImage);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IImage* GraphicsFactory::createSolidShapeImage (Color color, int width, int height)
{
	IImage* image = createShapeImage ();
	
	AutoPtr<IGraphics> graphics = createShapeBuilder (image);
	ASSERT (graphics != nullptr)
	Rect rect (Rect (0, 0, width, height));
	graphics->fillRect (rect, SolidBrush (color));
	
	return image;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ITextLayout* GraphicsFactory::createTextLayout ()
{
	return System::GetGraphicsHelper ().Factory_createTextLayout ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool GraphicsFactory::hasGraphicsLayers ()
{
	static int layersChecked = -1;
	if(layersChecked == -1)
	{
		AutoPtr<IGraphicsLayer> layer = createGraphicsLayer (ClassID::GraphicsLayer);
		layersChecked = layer.isValid () ? 1 : 0;
	}
	return layersChecked == 1;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IGraphicsLayer* GraphicsFactory::createGraphicsLayer (UIDRef cid)
{
	return System::GetGraphicsHelper ().Factory_createGraphicsLayer (cid);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IUIValue* GraphicsFactory::createValue ()
{
	return System::GetGraphicsHelper ().Factory_createValue ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IImage* GraphicsFactory::createFilmstrip (IImage* sourceImage, StringID frames)
{
	return System::GetGraphicsHelper ().Factory_createFilmstrip (sourceImage, frames);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IImage* GraphicsFactory::createImagePart (IImage* sourceImage, RectRef partRect)
{
	return System::GetGraphicsHelper ().Factory_createImagePart (sourceImage, partRect);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IImage* GraphicsFactory::createMultiImage (IImage* images[], CString frameNames[], int count)
{
	return System::GetGraphicsHelper ().Factory_createMultiImage (images, frameNames, count);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IImage* GraphicsFactory::createMultiResolutionBitmap (IImage* bitmaps[], float scaleFactors[], int count)
{
	return System::GetGraphicsHelper ().Factory_createMultiResolutionBitmap (bitmaps, scaleFactors, count);
}
