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
// Filename    : ccl/gui/graphics/imaging/tiledimage.h
// Description : tiledimage
//
//************************************************************************************************

#ifndef _ccl_tiledimage_h
#define _ccl_tiledimage_h

#include "ccl/gui/graphics/imaging/image.h"
#include "ccl/public/collections/vector.h"

#include "ccl/public/text/cstring.h"

namespace CCL {

//************************************************************************************************
// TiledImage
/** Image that tiles itself when drawn. */
//************************************************************************************************

class TiledImage: public Image
{
public:
	DECLARE_CLASS_ABSTRACT (TiledImage, Image)

	TiledImage (Image* sourceImage, TileMethod method, RectRef margins);
	~TiledImage ();

	void setMargins (RectRef rect);
	RectRef getMargins () const;

	TileMethod getMethod () const;
	void setMethod (TileMethod method);

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
	Image* sourceImage;
	Rect margins;
	TileMethod method;

	void checkMargins ();
};

//////////////////////////////////////////////////////////////////////////////////////////////////
// inline
//////////////////////////////////////////////////////////////////////////////////////////////////

inline RectRef TiledImage::getMargins () const { return margins; }
inline TiledImage::TileMethod TiledImage::getMethod () const { return method; }

} // namespace CCL

#endif // _ccl_tiledimage_h
