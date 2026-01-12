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
// Filename    : ccl/gui/graphics/imaging/bitmappainter.h
// Description : Bitmap Painter
//
//************************************************************************************************

#ifndef _ccl_bitmappainter_h
#define _ccl_bitmappainter_h

#include "ccl/base/object.h"

#include "ccl/public/gui/graphics/ibitmapfilter.h"

namespace CCL {

//************************************************************************************************
// BitmapPainter
//************************************************************************************************

class BitmapPainter: public Object,
					 public IBitmapPainter
{
public:
	DECLARE_CLASS (BitmapPainter, Object)

	BitmapPainter ();

	// IBitmapPainter
	void CCL_API setBackColor (Color color) override;
	void CCL_API setFilter (IBitmapFilter* filter, tbool share = false) override;
	tresult CCL_API drawImage (IGraphics& graphics, IImage* image, RectRef src, RectRef dst) override;
	tresult CCL_API drawInverted (IGraphics& graphics, IImage* image, RectRef src, RectRef dst) override;
	tresult CCL_API drawColorized (IGraphics& graphics, IImage* image, RectRef src, RectRef dst, Color color) override;
	tresult CCL_API drawTinted (IGraphics& graphics, IImage* image, RectRef src, RectRef dst, Color color) override;
	
	CLASS_INTERFACE (IBitmapPainter, Object)

protected:
	Color backColor;
	AutoPtr<IBitmapFilter> filter;

	static tresult drawImage (IGraphics& graphics, IImage* image, RectRef src, RectRef dst, IBitmapFilter* filter, Color backColor);
};

//************************************************************************************************
// BitmapProcessor
//************************************************************************************************

class BitmapProcessor: public Object,
					   public IBitmapProcessor
{
public:
	DECLARE_CLASS (BitmapProcessor, Object)

	void setDestination (IImage* dstImage); ///< set destination image directly, must be compatible!
	void setSource (IImage* srcImage); 		///< set source image directly, must be compatible!
	
	// IBitmapProcessor
	tresult CCL_API setup (IImage* srcImage, Color backColor, int options = 0, 
						   const Point* size = nullptr, float defaultScaleFactor = 1.f) override;
	IImage* CCL_API getOutput () override;
	tresult CCL_API process (IBitmapFilter& filter) override;
	void CCL_API reset () override;

	CLASS_INTERFACE (IBitmapProcessor, Object)

protected:
	AutoPtr<IBitmap> srcBitmap;
	AutoPtr<IBitmap> dstBitmap;

	static IBitmap* convert (IImage* image, IBitmap::PixelFormat format, Color backColor, 
							 float defaultScaleFactor, bool& copied);
};

} // namespace CCL

#endif // _ccl_bitmappainter_h
