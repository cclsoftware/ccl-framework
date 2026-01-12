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
// Filename    : ccl/gui/graphics/imaging/multiimage.h
// Description : MultiImage class
//
//************************************************************************************************

#ifndef _ccl_multiimage_h
#define _ccl_multiimage_h

#include "ccl/gui/graphics/imaging/image.h"
#include "ccl/public/gui/graphics/iconsetformat.h"

#include "ccl/public/text/cstring.h"
#include "ccl/base/collections/objectarray.h"

namespace CCL {

//************************************************************************************************
// MultiImage
/** Multi-resolution image class. */
//************************************************************************************************

class MultiImage: public Image
{
public:
	DECLARE_CLASS (MultiImage, Image)

	MultiImage ();

	void addFrame (Image* image, StringID name = nullptr);
	Image* getFrame (int frameIndex);

	// Image
	ImageType CCL_API getType () const override;
	int CCL_API getFrameCount () const override;
	int CCL_API getCurrentFrame () const override;
	void CCL_API setCurrentFrame (int frameIndex) override;
	int CCL_API getFrameIndex (StringID name) const override;
	Image* getOriginalImage (Rect& originalRect, bool deep = false) override;
	tresult draw (GraphicsDevice& graphics, PointRef pos, const ImageMode* mode = nullptr) override;
	tresult draw (GraphicsDevice& graphics, PointFRef pos, const ImageMode* mode = nullptr) override;
	tresult draw (GraphicsDevice& graphics, RectRef src, RectRef dst, const ImageMode* mode = nullptr) override;
	tresult draw (GraphicsDevice& graphics, RectFRef src, RectFRef dst, const ImageMode* mode = nullptr) override;
	tresult tile (GraphicsDevice& graphics, int method, RectRef src, RectRef dest, RectRef clip, RectRef margins) override;

protected:
	class FrameEntry;

	ObjectArray frames;
	int currentFrame;
};

//************************************************************************************************
// ImageResolutionSelector::draw
/** Helper function for selecting the frame with the best matching resolution from a MultiImage
	with frames of different sizes.	If there is no exact size match, the next smaller or larger
	frame is used. */
//************************************************************************************************

struct ImageResolutionSelector
{
	Image* bestImage;
	Rect srcRect;
	Rect dstRect;

	ImageResolutionSelector (Image* image, RectRef rect, int flags = 0, int frame = 0);

	enum Flags
	{
		kAllowStretch 	= 1<<0,	///< stretch a smaller image proportionally to fit (by default, it just gets centered)
		kAllowZoom  	= 1<<1	///< zoom image to fill entire space, while preserving its aspect ratio (image will be clipped)
	};

	/** Select the best matching frame of a MultiImage. */
	static Image* selectImage (Image* image, PointRef destSize, int flags = 0);
	static IImage* selectImage (IImage* image, PointRef destSize, int flags = 0);

	/** Draw the best matching frame of a MultiImage. */
	static void draw (IGraphics& port, Image* image, RectRef rect, int flags = 0, int frame = 0, const ImageMode* mode = nullptr);
	static void draw (IGraphics& port, IImage* image, RectRef rect, int flags = 0, int frame = 0, const ImageMode* mode = nullptr);
};

//************************************************************************************************
// IconSetFormat2
//************************************************************************************************

class IconSetFormat2: public IconSetFormat
{
public:
	static bool isValidIconSize (int size);

	static void makeIconName (String& fileName, const IconSize& iconSize);
	static void makeIconName (String& fileName, const Image* image, float scaleFactor = 1.f);
	static bool isValidIconName (StringRef fileName);

protected:
	static const CStringPtr fileNamePattern;
	static const CStringPtr fileNamePatternHiRes;
};

} // namespace CCL

#endif // _ccl_multiimage_h
