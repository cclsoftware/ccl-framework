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
// Filename    : ccl/public/gui/framework/idrawable.cpp
// Description : Drawable Interface
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "ccl/public/gui/framework/idrawable.h"

#include "ccl/public/gui/graphics/igraphics.h"

using namespace CCL;

//************************************************************************************************
// SolidDrawable
//************************************************************************************************

SolidDrawable::SolidDrawable (const SolidBrush& brush, float opacity)
: brush (brush),
  opacity (opacity)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API SolidDrawable::takeOpacity ()
{
	Color color = brush.getColor ();
	opacity = color.getAlphaF ();
	color.alpha = 0xFF;
	brush.setColor (color);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API SolidDrawable::draw (const DrawArgs& args)
{
	CCL_PRINTF ("SolidDrawable draw %3d %3d %3d %3d  ur %3d %3d %3d %3d  \n",
				args.size.left, args.size.top, args.size.getWidth (), args.size.getHeight (),
				args.updateRgn.bounds.left, args.updateRgn.bounds.top, args.updateRgn.bounds.getWidth (), args.updateRgn.bounds.getHeight
				())
	Rect rect (args.size);
	if(rect.bound (args.updateRgn.bounds))
		args.graphics.fillRect (rect, brush);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SolidDrawable::setOpacity (float _opacity)
{
	opacity = _opacity;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

float CCL_API SolidDrawable::getOpacity () const
{
	return opacity;
}

//************************************************************************************************
// BorderDrawable
//************************************************************************************************

BorderDrawable::BorderDrawable (ColorRef fillColor, ColorRef borderColor, Coord cornerRadius)
: fillBrush (fillColor),
  borderPen (borderColor),
  cornerRadius (cornerRadius)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

Coord BorderDrawable::getSafetyMargin () const
{
	return getCornerRadius () > 0 ? 1 : 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API BorderDrawable::draw (const IDrawable::DrawArgs& args)
{
	// avoid artifacts with fractional scaling when drawing round rect close to sprite edges
	// (to compensate this margin, the sprite should be enlaged by same amout)
	Rect rect (args.size);
	rect.contract (getSafetyMargin ());

	Coord shortEdge = ccl_min (rect.getHeight (), rect.getWidth ());
	Coord radius = ccl_min (cornerRadius, shortEdge / 2);
	Rect clipRect (args.updateRgn.bounds);
	clipRect.expand (radius);
	if(rect.bound (clipRect))
	{
		AntiAliasSetter smoother (args.graphics);

		if(radius > 0)
			args.graphics.fillRoundRect (rect, radius, radius, fillBrush);
		else
			args.graphics.fillRect (rect, fillBrush);

		if(borderPen.getColor ().getAlphaF () > 0.f)
		{
			if(radius > 0)
				args.graphics.drawRoundRect (rect, radius, radius, borderPen);
			else
				args.graphics.drawRect (args.size, borderPen);
		}
	}
}

//************************************************************************************************
// ImageDrawable
//************************************************************************************************

ImageDrawable::ImageDrawable (IImage* image, float alpha)
: image (image),
  imageMode (alpha),
  opacity (1.f)
{
	ASSERT (image != nullptr)
	image->retain ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API ImageDrawable::takeOpacity ()
{
	opacity = imageMode.getAlphaF ();
	imageMode.setAlphaF (1.f);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API ImageDrawable::draw (const DrawArgs& args)
{
	args.graphics.drawImage (image, args.size.getLeftTop (), &imageMode);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

float CCL_API ImageDrawable::getOpacity () const
{
	return opacity;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IImage* CCL_API ImageDrawable::getImage () const
{
	return image;
}
