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
// Filename    : ccl/gui/graphics/shapes/shapeimage.h
// Description : Shape Image
//
//************************************************************************************************

#ifndef _ccl_shapeimage_h
#define _ccl_shapeimage_h

#include "ccl/gui/graphics/imaging/image.h"

namespace CCL {

class Shape;
interface IColorScheme;

//************************************************************************************************
// ShapeImage
/** Use Shape object as Image. */
//************************************************************************************************

class ShapeImage: public Image
{
public:
	DECLARE_CLASS (ShapeImage, Image)

	ShapeImage (Shape* shape = nullptr);
	~ShapeImage ();

	Shape* getShape () const;
	void setShape (Shape* shape);
	bool hasReferences (IColorScheme& scheme) const;

	void setFilmstrip (bool state);
	bool isFilmstrip () const { return filmstrip; }

	// Image
	ImageType CCL_API getType () const override;
	int CCL_API getFrameCount () const override;
	int CCL_API getCurrentFrame () const override;
	void CCL_API setCurrentFrame (int frameIndex) override;
	int CCL_API getFrameIndex (StringID name) const override;
	tresult draw (GraphicsDevice& graphics, PointRef pos, const ImageMode* mode = nullptr) override;
	tresult draw (GraphicsDevice& graphics, PointFRef pos, const ImageMode* mode = nullptr) override;
	tresult draw (GraphicsDevice& graphics, RectRef src, RectRef dst, const ImageMode* mode = nullptr) override;
	tresult draw (GraphicsDevice& graphics, RectFRef src, RectFRef dst, const ImageMode* mode = nullptr) override;
	tresult tile (GraphicsDevice& graphics, int method, RectRef src, RectRef dest, RectRef clip, RectRef margins) override { return kResultFailed; }

protected:
	void setFrameCount (int frames);
	void updateSize ();

	Shape* shape;
	int frameCount;
	int currentFrame;
	bool filmstrip;
};

} // namespace CCL

#endif // _ccl_shapeimage_h
