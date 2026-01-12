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
// Filename    : ccl/public/gui/graphics/graphicsfactory.h
// Description : Graphics factory functions
//
//************************************************************************************************

#ifndef _ccl_graphicsfactory_h
#define _ccl_graphicsfactory_h

#include "ccl/public/gui/graphics/ibitmap.h"
#include "ccl/public/gui/graphics/igraphicspath.h"
#include "ccl/public/gui/graphics/igradient.h"

namespace CCL {

class FileType;
interface IAttributeList;
interface IBitmapFilter;
interface ITextLayout;
interface IGraphicsLayer;
interface IUIValue;

//************************************************************************************************
// GraphicsFactory
/** Functions for creating graphics objects. */
//************************************************************************************************

namespace GraphicsFactory
{
	/** Get number of supported image formats. */
	int getNumImageFormats ();
	
	/** Get image format by index. */
	const FileType* getImageFormat (int index);

	/** Load image from file. */
	IImage* loadImageFile (UrlRef path);

	/** Save image to file. */
	bool saveImageFile (UrlRef path, IImage* image,
						const IAttributeList* encoderOptions = nullptr);

	/** Load image from stream. */
	IImage* loadImageStream (IStream& stream, const FileType& format);

	/** Save image to stream. */
	bool saveImageStream (IStream& stream, IImage* image, const FileType& format,
						  const IAttributeList* encoderOptions = nullptr);

	/** Create a new bitmap. */
	IImage* createBitmap (int width, int height, IBitmap::PixelFormat format = IBitmap::kRGB, float scaleFactor = 1.f);

	/** Create a graphics device for drawing into a bitmap. */
	IGraphics* createBitmapGraphics (IImage* bitmap);
	
	/** Create bitmap filter. */
	IBitmapFilter* createBitmapFilter (StringID which);

	/** Create bitmap colorization filter. */
	IBitmapFilter* createBitmapColorizationFilter (Color color);

	/** Create bitmap of given color. */
	IImage* createSolidBitmap (Color color, int width, int height, IBitmap::PixelFormat format = IBitmap::kRGB, float scaleFactor = 1.f);

	/** Create an empty graphics path. */
	IGraphicsPath* createPath (IGraphicsPath::TypeHint type = IGraphicsPath::kPaintPath);

	/** Create an new gradient. */
	IGradient* createGradient (IGradient::TypeHint type);

	/** Create an empty vector image. */
	IImage* createShapeImage ();

	/** Create a graphics device for drawing a vector image. */
	IGraphics* createShapeBuilder (IImage* shapeImage);
	
	/** Create vector image with a single colored rectangle. */
	IImage* createSolidShapeImage (Color color, int width, int height);

	/** Create text layout object. */
	ITextLayout* createTextLayout ();

	/** Check if graphics layers are available. */
	bool hasGraphicsLayers ();
	
	/** Create graphics layer object. */
	IGraphicsLayer* createGraphicsLayer (UIDRef cid);

	/** Create value object. */
	IUIValue* createValue ();
		
	/** Create filmstrip from source image with given frames. */
	IImage* createFilmstrip (IImage* sourceImage, StringID frames);

	/** Create image part. */
	IImage* createImagePart (IImage* sourceImage, RectRef partRect);

	/** Create container image. */
	IImage* createMultiImage (IImage* images[], CString frameNames[], int count);

	/** Create bitmap with representations for multiple DPI scaling factors. */
	IImage* createMultiResolutionBitmap (IImage* bitmaps[], float scaleFactors[], int count);

} // namespace GraphicsFactory

} // namespace CCL

#endif // _ccl_graphicsfactory_h
