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
// Filename    : ccl/gui/graphics/imaging/imagepart.h
// Description : Image Part
//
//************************************************************************************************

#ifndef _ccl_imagepart_h
#define _ccl_imagepart_h

#include "ccl/gui/graphics/imaging/image.h"

#include "ccl/public/text/cstring.h"
#include "ccl/public/gui/graphics/ibitmap.h"

namespace CCL {

//************************************************************************************************
// ImagePart
//************************************************************************************************

class ImagePart: public Image,
				 protected IBitmap, // (only if supported by sourceImage)
				 protected IMultiResolutionBitmap
{
public:
	DECLARE_CLASS (ImagePart, Image)

	ImagePart (Image* sourceImage = nullptr, RectRef partRect = Rect ());
	~ImagePart ();

	PROPERTY_MUTABLE_CSTRING (partName, PartName)

	// Image
	ImageType CCL_API getType () const override;
	Image* getOriginalImage (Rect& originalRect, bool deep = false) override;
	tresult draw (GraphicsDevice& graphics, PointRef pos, const ImageMode* mode = nullptr) override;
	tresult draw (GraphicsDevice& graphics, PointFRef pos, const ImageMode* mode = nullptr) override;
	tresult draw (GraphicsDevice& graphics, RectRef src, RectRef dst, const ImageMode* mode = nullptr) override;
	tresult draw (GraphicsDevice& graphics, RectFRef src, RectFRef dst, const ImageMode* mode = nullptr) override;
	tresult tile (GraphicsDevice& graphics, int method, RectRef src, RectRef dest, RectRef clip, RectRef margins) override;

	CLASS_INTERFACES (Image)

protected:
	Image* sourceImage;
	Rect partRect;

	IBitmap* getSourceBitmap () const;

	// IBitmap
	tresult CCL_API lockBits (BitmapLockData& data, PixelFormat format, int mode) override;
	tresult CCL_API unlockBits (BitmapLockData& data) override;
	tresult CCL_API scrollPixelRect (const Rect& rect, const Point& delta) override;
	Point CCL_API getPixelSize () const override;
	PixelFormat CCL_API getPixelFormat () const override;
	float CCL_API getContentScaleFactor () const override;

	// IMultiResolutionBitmap
	int CCL_API getRepresentationCount () const override;
	void CCL_API setCurrentRepresentation (int index) override;
	int CCL_API getCurrentRepresentation () const override;
};

} // namespace CCL

#endif // _ccl_imagepart_h
