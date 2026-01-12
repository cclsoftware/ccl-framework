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
// Filename    : ccl/gui/graphics/shapes/shapeimage.cpp
// Description : Shape Image
//
//************************************************************************************************

#include "ccl/gui/graphics/shapes/shapeimage.h"
#include "ccl/gui/graphics/shapes/shapes.h"

#include "ccl/gui/graphics/graphicsdevice.h"
#include "ccl/public/text/cstring.h"

using namespace CCL;

//************************************************************************************************
// ShapeImage
//************************************************************************************************

DEFINE_CLASS_HIDDEN (ShapeImage, Image)

//////////////////////////////////////////////////////////////////////////////////////////////////

ShapeImage::ShapeImage (Shape* _shape)
: shape (nullptr),
  frameCount (1),
  currentFrame (0),
  filmstrip (false)
{
	setShape (_shape);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ShapeImage::~ShapeImage ()
{
	if(shape)
		shape->release ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ShapeImage::setFilmstrip (bool state)
{
	filmstrip = state;
	updateSize ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ShapeImage::setShape (Shape* _shape)
{
	take_shared<Shape> (shape, _shape);
	setFrameCount (shape ? shape->countShapes () : 1);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Shape* ShapeImage::getShape () const
{
	return shape;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ShapeImage::hasReferences (IColorScheme& scheme) const
{
	return shape && shape->hasReferences (scheme);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Image::ImageType CCL_API ShapeImage::getType () const
{
	return kScalable; // |kMultiple???
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API ShapeImage::getFrameCount () const
{
	return frameCount;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API ShapeImage::getCurrentFrame () const
{
	return currentFrame;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API ShapeImage::setCurrentFrame (int frameIndex)
{
	frameIndex = ccl_bound<int> (frameIndex, 0, getFrameCount () - 1);
	if(frameIndex != currentFrame)
	{
		currentFrame = frameIndex;
		updateSize ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API ShapeImage::getFrameIndex (StringID _name) const
{
	String name;
	name << _name;
	if(shape)
	{
		for(int i = 0; i < shape->countShapes (); ++i)
		{
			Shape* subShape = shape->getShape (i);
			if(subShape && name == subShape->getName ())
				return i;
		}
	}
	return -1;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ShapeImage::setFrameCount (int frames)
{
	frameCount = frames > 0 ? frames : 1;
	setCurrentFrame (currentFrame);
	updateSize ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ShapeImage::updateSize ()
{
	if(isFilmstrip () && shape)
	{
		// move all subshapes to origin
		for(int i = 0; i < shape->countShapes (); ++i)
		{
			ComplexShape* subShape = ccl_cast<ComplexShape>(shape->getShape (i));
			if(subShape)
			{
				Rect size (subShape->getSize ());
				size.offset (-size.left, -size.top);
				subShape->setSize (size);
			}
		}
	}

	Shape* s = (shape && isFilmstrip ()) ? shape->getShape (currentFrame) : shape;
	Rect r;
	if(s)
		s->getBounds (r);
	
	size (r.right, r.bottom); // negative coords are clipped
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult ShapeImage::draw (GraphicsDevice& graphics, PointRef pos, const ImageMode* mode)
{
	Shape* s = (shape && isFilmstrip ()) ? shape->getShape (currentFrame) : shape;
	if(s)
	{
		TransformSetter t (graphics, Transform ().translate ((float)pos.x, (float)pos.y));
				
		s->drawShape (graphics, mode);
	}
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult ShapeImage::draw (GraphicsDevice& graphics, PointFRef pos, const ImageMode* mode)
{
	// todo
	return draw (graphics, pointFToInt (pos), mode);
}

//////////////////////////////////////////////////////////////////////////////////////////////////
	
tresult ShapeImage::draw (GraphicsDevice& graphics, RectRef src, RectRef dst, const ImageMode* mode)
{
	Coord srcWidth  = src.getWidth ();
	Coord srcHeight = src.getHeight ();
	Coord dstWidth  = dst.getWidth ();
	Coord dstHeight = dst.getHeight ();
	
	if(srcWidth == 0 || srcHeight == 0)
		return kResultOk;
	
	bool mustRestoreContext = false;
	
	auto needsClipping = [&]()
	{
		// clip if only a portion of the shape should be drawn!
		if(src.left != 0 || src.top != 0)
			return true;
		
		Rect fullsrc;
		if(ComplexShape* complex = ccl_cast<ComplexShape> (shape))
			complex->getJoinedBounds (fullsrc);
		else if(shape)
			shape->getBounds (fullsrc);
		
		if(fullsrc.getWidth () != src.getWidth () || fullsrc.getHeight () != src.getHeight ())
			return true;
		
		return false;
	};
	
	if(needsClipping ())
	{
		graphics.saveState ();
		graphics.addClip (dst);
		mustRestoreContext = true;
	}

	AntiAliasSetter smoother (graphics, shape && shape->shouldAntiAlias () ? true : ((graphics.getMode () & IGraphics::kAntiAlias) != 0));
	
	// Scaling
	if(srcWidth == dstWidth && srcHeight == dstHeight)
	{
		draw (graphics, Point (dst.left - src.left, dst.top - src.top), mode);
	}
	else
	{
		float sx = (float)dstWidth  / (float)srcWidth;
		float sy = (float)dstHeight / (float)srcHeight;
		
		if(shape && shape->shouldScale ())
		{
			Point pos (dst.left - src.left, dst.top - src.top);

			Shape* s = isFilmstrip () ? shape->getShape (currentFrame) : shape;
			if(s)
			{
				TransformSetter t (graphics, Transform ().translate ((float)pos.x, (float)pos.y));
				s->drawShape (graphics, sx, sy, mode);
			}
		}
		else
		{
			// Note: Transformation order is important!
			// The new matrix is translated before it is scaled,
			// then the shape is drawn at the new origin without further offset!

			if(!mustRestoreContext)
			{
				graphics.saveState ();
				mustRestoreContext = true;
			}
			
			Transform newMatrix;
			newMatrix.translate ((float)(dst.left - src.left), (float)(dst.top - src.top));
			newMatrix.scale (sx, sy);
			graphics.addTransform (newMatrix);

			draw (graphics, Point (), mode);
		}
	}

	if(mustRestoreContext)
		graphics.restoreState ();
		
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
	
tresult ShapeImage::draw (GraphicsDevice& graphics, RectFRef src, RectFRef dst, const ImageMode* mode)
{
	// todo
	return draw (graphics, rectFToInt (src), rectFToInt (dst), mode);
}

