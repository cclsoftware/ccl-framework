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
// Filename    : ccl/public/gui/graphics/iimage.h
// Description : Image Interface
//
//************************************************************************************************

#ifndef _ccl_iimage_h
#define _ccl_iimage_h

#include "ccl/public/base/iunknown.h"
#include "ccl/public/base/cclmacros.h"

namespace CCL {

//************************************************************************************************
// IImage
/** Basic Image interface. 
	\ingroup gui_graphics */
//************************************************************************************************

interface IImage: IUnknown
{
	/** Image type. */
	DEFINE_ENUM (ImageType)
	{
		kScalable,	///< scalable vector image
		kBitmap,	///< rastered bitmap
		kMultiple	///< image is container of multiple images
	};

	/** Tile method. */
	DEFINE_ENUM (TileMethod)
	{
		kNone = 0,
		kTileX,                        ///< tile="tile-x"
		kTileY,                        ///< tile="tile-y"
		kRepeatX,                      ///< tile="repeat-x"
		kRepeatY,                      ///< tile="repeat-y"
		kTileXY,                       ///< tile="tile-xy"
		kRepeatXY,                     ///< tile="repeat-xy"
		kStretchXY,					   ///< tile="stretch-xy" respecting the margin
		kStretchX,				   	   ///< tile="stretch-x" respecting the margin y direction
		kStretchY				       ///< tile="stretch-y" respecting the margin x direction
	};

	// predefined image frame names
	DECLARE_STRINGID_MEMBER (kSmall)		///< 16x16
	DECLARE_STRINGID_MEMBER (kNormal)		///< 32x32
	DECLARE_STRINGID_MEMBER (kLarge)		///< 64x64

	// additional properties (IObject)
	DECLARE_STRINGID_MEMBER (kIsTemplate)	///< template images can be colorized by the framework
	DECLARE_STRINGID_MEMBER (kIsAdaptive)	///< adaptive images can adapt the luminance of a reference color

	/** Helper to select image frame by name. */
	struct Selector
	{
		Selector (IImage* image, StringID frameName)
		{ if(image) image->setCurrentFrame (image->getFrameIndex (frameName)); }
	};

	/** Get image type. */
	virtual ImageType CCL_API getType () const = 0;

	/** Get image width in points. */
	virtual int CCL_API getWidth () const = 0;

	/** Get image height in points. */
	virtual int CCL_API getHeight () const = 0;

	/** Get number of frames. */
	virtual int CCL_API getFrameCount () const = 0;
	
	/** Get current frame index. */
	virtual int CCL_API getCurrentFrame () const = 0;

	/** Select frame by index. */
	virtual void CCL_API setCurrentFrame (int frameIndex) = 0;

	/** Get frame index by name. */
	virtual int CCL_API getFrameIndex (StringID name) const = 0;

	/** Get original image (could be this or a source image). */
	virtual IImage* CCL_API getOriginal () = 0;

	DECLARE_IID (IImage)
};

DEFINE_IID (IImage, 0x184c6791, 0x8392, 0x4569, 0xa1, 0x5d, 0x88, 0x3e, 0xa9, 0x5a, 0x6d, 0xc5)
DEFINE_STRINGID_MEMBER (IImage, kSmall, "small")
DEFINE_STRINGID_MEMBER (IImage, kNormal, "normal")
DEFINE_STRINGID_MEMBER (IImage, kLarge, "large")
DEFINE_STRINGID_MEMBER (IImage, kIsTemplate, "isTemplate")
DEFINE_STRINGID_MEMBER (IImage, kIsAdaptive, "isAdaptive")

//************************************************************************************************
// ImageMode
/** Image mode definition. 
	\ingroup gui_graphics */
//************************************************************************************************

class ImageMode
{
public:
	/** Interpolation mode. */
	DEFINE_ENUM (InterpolationMode)
	{
		kInterpolationDefault = 0,	///< default interpolation mode
		kInterpolationHighQuality,	///< high quality interpolation mode
		kInterpolationPixelQuality	///< sharp interpolation mode
	};

	ImageMode (float alpha = 1.f, InterpolationMode interpolationMode = kInterpolationDefault)
	: alpha (alpha),
	  interpolationMode (interpolationMode)
	{}

	ImageMode (InterpolationMode interpolationMode)
	: alpha (1.f),
	  interpolationMode (interpolationMode)
	{}

	PROPERTY_VARIABLE (float, alpha, AlphaF)
	PROPERTY_VARIABLE (InterpolationMode, interpolationMode, InterpolationMode)
};

//************************************************************************************************
// ImageEncoding
/** Image encoding options.
	\ingroup gui_graphics */
//************************************************************************************************

namespace ImageEncoding
{
	DEFINE_STRINGID (kQuality, "quality")		///< image encoding quality (0..100)
	DEFINE_STRINGID (kLossless, "lossless")		///< lossless encoding (true/false)
}

} // namespace CCL

#endif // _ccl_iimage_h
