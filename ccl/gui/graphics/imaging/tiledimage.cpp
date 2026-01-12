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
// Filename    : ccl/gui/graphics/imaging/tiledimage.cpp
// Description : TiledImage
//
//************************************************************************************************

#include "ccl/gui/graphics/imaging/tiledimage.h"

using namespace CCL;

//************************************************************************************************
// TiledImage
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (TiledImage, Image)

//////////////////////////////////////////////////////////////////////////////////////////////////

TiledImage::TiledImage (Image* sourceImage, TileMethod method, RectRef _margins)
: sourceImage (sourceImage), method (method), margins (_margins)
{
	if(sourceImage)
	{
		sourceImage->retain ();
		size.x = sourceImage->getWidth ();
		size.y = sourceImage->getHeight ();
		
		if(margins.left == 0 && margins.right == 0 && margins.top == 0 && margins.bottom == 0)
		{
			margins.left = size.x / 4;
			margins.right = size.x / 4;
			margins.top = size.y / 4;
			margins.bottom = size.y / 4;
		}
		else
			checkMargins ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

TiledImage::~TiledImage ()
{
	if(sourceImage)
		sourceImage->release ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TiledImage::setMargins (RectRef rect)
{
	this->margins = rect;

	checkMargins ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TiledImage::checkMargins ()
{
	// margins must not be larger than image: prevent empty or negative size of center part
	ASSERT (margins.left + margins.right < size.x && margins.top + margins.bottom < size.y)

	// fix it
	if(margins.left + margins.right >= size.x)
	{
		margins.right = size.x - margins.left - 1;
		if(margins.right < 0)
		{
			margins.left = size.x - 1;
			margins.right = 0;
		}
	}

	if(margins.top + margins.bottom  >= size.y)
	{
		margins.bottom = size.y - margins.top - 1;
		if(margins.bottom < 0)
		{
			margins.top = size.y - 1;
			margins.bottom = 0;
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TiledImage::setMethod (TileMethod method)
{
	this->method = method;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Image::ImageType CCL_API TiledImage::getType () const
{
	return kMultiple;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API TiledImage::getFrameCount () const
{
	if(sourceImage)
		return sourceImage->getFrameCount ();
	return 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API TiledImage::getCurrentFrame () const
{
	if(sourceImage)
		return sourceImage->getCurrentFrame ();
	return 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API TiledImage::setCurrentFrame (int frameIndex)
{
	if(sourceImage)
		return sourceImage->setCurrentFrame (frameIndex);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API TiledImage::getFrameIndex (StringID name) const
{
	if(sourceImage)
		return sourceImage->getFrameIndex (name);
	return -1;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Image* TiledImage::getOriginalImage (Rect& originalRect, bool deep)
{
	getSize (originalRect);
	return resolveOriginal (sourceImage, originalRect, deep);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult TiledImage::draw (GraphicsDevice& graphics, PointRef pos, const ImageMode* mode)
{
	if(sourceImage)
		return sourceImage->draw (graphics, pos, mode);
	return kResultFalse;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult TiledImage::draw (GraphicsDevice& graphics, PointFRef pos, const ImageMode* mode)
{
	return draw (graphics, pointFToInt (pos), mode);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult TiledImage::tile (GraphicsDevice& graphics, int method, RectRef src, RectRef dest, RectRef clip, RectRef margins)
{
	if(sourceImage)
		sourceImage->tile (graphics, method, src, dest, clip, margins);
	return kResultFalse;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult TiledImage::draw (GraphicsDevice& graphics, RectRef src, RectRef dst, const ImageMode* mode)
{
	if(sourceImage)
		return sourceImage->tile (graphics, method, src, dst, dst, margins);
	return kResultTrue;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult TiledImage::draw (GraphicsDevice& graphics, RectFRef src, RectFRef dst, const ImageMode* mode)
{
	return draw (graphics, rectFToInt (src), rectFToInt (dst), mode);
}
