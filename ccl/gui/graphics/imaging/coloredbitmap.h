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
// Filename    : ccl/gui/graphics/imaging/coloredbitmap.h
// Description : Colored Bitmap
//
//************************************************************************************************

#ifndef _ccl_coloredbitmap_h
#define _ccl_coloredbitmap_h

#include "ccl/gui/graphics/imaging/image.h"
#include "ccl/gui/graphics/imaging/bitmapfilter.h"

namespace CCL {

class ColorScheme;
interface IColorScheme;

//************************************************************************************************
// ModifiedBitmap
/** Cached version of a bitmap. */
//************************************************************************************************

class ModifiedBitmap: public Image
{
public:
	DECLARE_CLASS_ABSTRACT (ModifiedBitmap, Image)
	
	ModifiedBitmap (IImage* sourceImage = nullptr);
	
	virtual Image* getModifiedImage () = 0;
	
	// Image
	ImageType CCL_API getType () const override;
	Image* getOriginalImage (Rect& originalRect, bool deep = false) override;
	tresult draw (GraphicsDevice& graphics, PointRef pos, const ImageMode* mode = nullptr) override;
	tresult draw (GraphicsDevice& graphics, PointFRef pos, const ImageMode* mode = nullptr) override;
	tresult draw (GraphicsDevice& graphics, RectRef src, RectRef dst, const ImageMode* mode = nullptr) override;
	tresult draw (GraphicsDevice& graphics, RectFRef src, RectFRef dst, const ImageMode* mode = nullptr) override;
	tresult tile (GraphicsDevice& graphics, int method, RectRef src, RectRef dest, RectRef clip, RectRef margins) override;
	
protected:	
	SharedPtr<IImage> sourceImage;
};

//************************************************************************************************
// ColoredBitmap
/** Cached colorized version of a bitmap. */
//************************************************************************************************

class ColoredBitmap: public ModifiedBitmap
{
public:
	DECLARE_CLASS (ColoredBitmap, ModifiedBitmap)
	
	ColoredBitmap (IImage* sourceImage = nullptr, ColorRef color = Colors::kBlack);
	
	virtual BitmapFilter* createBitmapFilter ();
	
	void setColor (const Color& color);	

	// ModifiedBitmap
	Image* getModifiedImage () override;

protected:
	AutoPtr<Image> modifiedImage;
	
	Color cachedColor;
	Color defaultColor;
};
	
//************************************************************************************************
// TintedBitmap
/** Cached tinted version of a bitmap. */
//************************************************************************************************
	
class TintedBitmap: public ColoredBitmap
{
public:
	DECLARE_CLASS (TintedBitmap, ColoredBitmap)
	
	TintedBitmap (IImage* sourceImage = nullptr, ColorRef color = Colors::kBlack);
	
	// ColoredBitmap
	BitmapFilter* createBitmapFilter () override;
};

//************************************************************************************************
// LightAdaptedBitmap
/** Cached lightAdapted version of a bitmap. */
//************************************************************************************************

class LightAdaptedBitmap: public ColoredBitmap
{
public:
	DECLARE_CLASS (LightAdaptedBitmap, ColoredBitmap)
	
	LightAdaptedBitmap (IImage* sourceImage = nullptr, ColorRef color = Colors::kBlack);
	
	// ColoredBitmap
	BitmapFilter* createBitmapFilter () override;
};
	
//************************************************************************************************
// ColoredSchemeBitmap
/** Bitmap dependent on color scheme. */
//************************************************************************************************

class ColoredSchemeBitmap: public ColoredBitmap
{
public:
	DECLARE_CLASS (ColoredSchemeBitmap, ColoredBitmap)

	ColoredSchemeBitmap (Image* sourceImage = nullptr);
	~ColoredSchemeBitmap ();

	void addFilter (IBitmapFilter* filter); ///< takes filter ownership!
	void addFilter (IBitmapFilter* filter, ColorScheme* scheme, StringID nameInScheme); ///< takes filter ownership!
	bool hasReferences (IColorScheme& scheme) const;

	void setImageUpdateNeeded ();
	
	// ColoredBitmap
	Image* getModifiedImage () override;
	ImageType CCL_API getType () const override;
	Image* getOriginalImage (Rect& originalRect, bool deep = false) override;

	CLASS_INTERFACES (ColoredBitmap)
	
protected:
	class WrappedFilter;
	
	BitmapFilterList filterList;
};

} // namespace CCL

#endif // _ccl_coloredbitmap_h
