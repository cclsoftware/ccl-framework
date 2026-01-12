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
// Filename    : ccl/public/gui/graphics/ibitmapfilter.h
// Description : Bitmap Filter Interfaces
//
//************************************************************************************************

#ifndef _ccl_ibitmapfilter_h
#define _ccl_ibitmapfilter_h

#include "ccl/public/gui/graphics/types.h"
#include "ccl/public/gui/graphics/ibitmap.h"

namespace CCL {

//////////////////////////////////////////////////////////////////////////////////////////////////

namespace ClassID
{
	DEFINE_CID (BitmapPainter, 0x421579be, 0x7d53, 0x4716, 0xae, 0x8a, 0xa7, 0x8f, 0xad, 0x0, 0xf8, 0xb);
	DEFINE_CID (BitmapProcessor, 0x2aec6ea5, 0xbe3f, 0x43b7, 0x8d, 0x43, 0x27, 0x23, 0x23, 0xad, 0x69, 0x43);
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// Bitmap Filter Classes
//////////////////////////////////////////////////////////////////////////////////////////////////
/**
	\ingroup gui_graphics */
namespace BitmapFilters
{
	DEFINE_STRINGID (kFilterList, "filterlist");          ///< List of filter [IBitmapFilterList]
	DEFINE_STRINGID (kClear, "clear");					  ///< Clear bitmap
	DEFINE_STRINGID (kPremultiplyAlpha, "premulalpha");	  ///< Premultiply RGB with alpha channel
	DEFINE_STRINGID (kRevertPremulAlpha, "revertalpha");  ///< Revert premultiplied alpha
	DEFINE_STRINGID (kByteSwapRGB, "byteswaprgb");        ///< Swap BGR to RGB and vice versa
	DEFINE_STRINGID (kInvert, "invert");                  ///< Invert
	DEFINE_STRINGID (kGrayScale, "grayscale");            ///< Grayscale
	DEFINE_STRINGID (kAlpha, "alpha");                    ///< Set alpha channel (properties: "value")
	DEFINE_STRINGID (kBlend, "blend");                    ///< Scale alpha channel (properties: "value")
	DEFINE_STRINGID (kLighten, "lighten");                ///< Add brightness (properties: "value")
	DEFINE_STRINGID (kNoise, "noise");					  ///< Add noise (properties: "value")
	DEFINE_STRINGID (kTint, "tint");                      ///< Use src intensity and alpha but replace hue (properties: "color")
	DEFINE_STRINGID (kColorize, "colorize");              ///< Use src alpha as mask for color (properties: "color")
	DEFINE_STRINGID (kLightAdapt, "lightAdapt");	 	  ///< Use src alpha but adapt luminance of dark/light pixels (bitmap is dark/light) that extreme pixels match color intensity (properties: "color")
	DEFINE_STRINGID (kFill, "fill");					  ///< Fill bitmap (properties: "color")
	DEFINE_STRINGID (kSaturator, "saturate");			  ///< saturates the bitmap (properties: "value")
	DEFINE_STRINGID (kAnalyze, "analyze");                ///< Analysis filter (multiple output properties)
	DEFINE_STRINGID (kBlurX, "blurX");					  ///< blurs the bitmap horizontally (properties: "value")
	DEFINE_STRINGID (kBlurY, "blurY");					  ///< blurs the bitmap vertically (properties: "value")
}

//************************************************************************************************
// IBitmapFilter
/**
	\ingroup gui_graphics */
//************************************************************************************************

interface IBitmapFilter: IUnknown
{
	/** Process bitmap data, src and dst could be equal when inplace processing. */
	virtual tresult CCL_API processData (BitmapData& dstData, const BitmapData& srcData) = 0;

	// Property identifiers
	DECLARE_STRINGID_MEMBER (kColorID)
	DECLARE_STRINGID_MEMBER (kValueID)

	DECLARE_IID (IBitmapFilter)
};

DEFINE_IID (IBitmapFilter, 0x331e295, 0x5b82, 0x44f8, 0xac, 0x11, 0x53, 0x23, 0x37, 0x4c, 0x47, 0x3f)
DEFINE_STRINGID_MEMBER (IBitmapFilter, kColorID, "color")
DEFINE_STRINGID_MEMBER (IBitmapFilter, kValueID, "value")

//************************************************************************************************
// IBitmapFilterList
/**
	\ingroup gui_graphics */
//************************************************************************************************

interface IBitmapFilterList: IBitmapFilter
{
	/** Add filter to list. */
	virtual tresult CCL_API addFilter (IBitmapFilter* filter, tbool share = false) = 0;

	DECLARE_IID (IBitmapFilterList)
};

DEFINE_IID (IBitmapFilterList, 0x7bfd164f, 0x77cf, 0x4e54, 0x82, 0x21, 0x7c, 0x9f, 0x60, 0x8c, 0xf8, 0xb1)

//************************************************************************************************
// IBitmapPainter
/**
	\ingroup gui_graphics */
//************************************************************************************************

interface IBitmapPainter: IUnknown
{
	/** Set back color for image conversion. */
	virtual void CCL_API setBackColor (Color color) = 0;
	
	/** Assign bitmap filter. */
	virtual void CCL_API setFilter (IBitmapFilter* filter, tbool share = false) = 0;

	/** Draw image with current filter. */
	virtual tresult CCL_API drawImage (IGraphics& graphics, IImage* image, RectRef src, RectRef dst) = 0;

	/** Draw image inverted. */
	virtual tresult CCL_API drawInverted (IGraphics& graphics, IImage* image, RectRef src, RectRef dst) = 0;
	
	/** Draw image with source alpha channel and new color. */
	virtual tresult CCL_API drawColorized (IGraphics& graphics, IImage* image, RectRef src, RectRef dst, Color color) = 0;

	/** Draw image with with new color. */
	virtual tresult CCL_API drawTinted (IGraphics& graphics, IImage* image, RectRef src, RectRef dst, Color color) = 0;

	DECLARE_IID (IBitmapPainter)
};

DEFINE_IID (IBitmapPainter, 0xe598f431, 0x5c33, 0x4156, 0xb1, 0x73, 0xd5, 0x7d, 0xf0, 0x59, 0x54, 0x8e)

//************************************************************************************************
// IBitmapProcessor
/**
	\ingroup gui_graphics */
//************************************************************************************************

interface IBitmapProcessor: IUnknown
{
	enum Options
	{
		kInplace = 1<<0	///< try to process image without copying
	};

	/** Set up with source image + back color. */
	virtual tresult CCL_API setup (IImage* srcImage, Color backColor, int options = 0, 
								   const Point* size = nullptr, float defaultScaleFactor = 1.f) = 0;

	/** Get output bitmap. */
	virtual IImage* CCL_API getOutput () = 0;

	/** Perform filter processing. */
	virtual tresult CCL_API process (IBitmapFilter& filter) = 0;

	/** Reset state. */
	virtual void CCL_API reset () = 0;

	DECLARE_IID (IBitmapProcessor)
};

DEFINE_IID (IBitmapProcessor, 0x6583aa2f, 0x7bf2, 0x470f, 0x94, 0x9e, 0xd2, 0xe, 0x89, 0x77, 0xb7, 0x58)

} // namespace CCL

#endif // _ccl_ibitmapfilter_h
