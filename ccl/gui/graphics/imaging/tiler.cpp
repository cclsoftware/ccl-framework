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
// Filename    : ccl/gui/graphics/imaging/tiler.cpp
// Description : Tiler
//
//************************************************************************************************

#include "ccl/gui/graphics/imaging/tiler.h"

using namespace CCL;

//////////////////////////////////////////////////////////////////////////////////////////////////

inline void RepeatX (Blitter& blitter, RectRef src, RectRef dst)
{
	Rect tileSrc (src);
	Rect tileDst (dst.left, dst.top, dst.left + src.getWidth (), dst.top + src.getHeight ());
	while(tileDst.bound (dst))
	{
		tileSrc.setWidth (tileDst.getWidth ());
		blitter.blit (tileSrc, tileDst);
		tileDst.offset (src.getWidth (), 0);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline void RepeatY (Blitter& blitter, RectRef src, RectRef dst)
{
	Rect tileSrc (src);
	Rect tileDst (dst.left, dst.top, dst.left + src.getWidth (), dst.top + src.getHeight ());
	while(tileDst.bound (dst))
	{
		tileSrc.setHeight (tileDst.getHeight ());
		blitter.blit (tileSrc, tileDst);
		tileDst.offset (0, src.getHeight ());
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline void RepeatXY (Blitter& blitter, RectRef src, RectRef dst)
{
	Rect tileSrc (src);
	Rect tileDst (dst.left, dst.top, dst.left + src.getWidth (), dst.top + src.getHeight ());
	while(true)
	{
		if(tileDst.left >= dst.right)
		{
			tileDst.offset (0, tileSrc.getHeight ());
			if(tileDst.top >= dst.bottom)
				break;
			tileDst.left = dst.left;
			tileDst.setWidth (src.getWidth ());
			tileDst.setHeight (src.getHeight ());
			tileSrc = src;
		}
		if(tileDst.right > dst.right)
		{
			tileDst.right = dst.right;
			tileSrc.setWidth (tileDst.getWidth ());
		}
		if(tileDst.bottom > dst.bottom)
		{
			tileDst.bottom = dst.bottom;
			tileSrc.setHeight (tileDst.getHeight ());
		}
		if(!tileSrc.isEmpty ())
			blitter.blit (tileSrc, tileDst);
		tileDst.offset (src.getWidth (), 0);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline void StretchX (Blitter& blitter, RectRef src, RectRef dst)
{
	Rect tileSrc (src);
	Rect tileDst (dst.left, dst.top, dst.right, dst.top + src.getHeight ());
	while(tileDst.bound (dst))
	{
		tileSrc.setHeight (tileDst.getHeight ());
		blitter.blit (tileSrc, tileDst);
		tileDst.offset (0, src.getHeight ());
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline void StretchY (Blitter& blitter, RectRef src, RectRef dst)
{
	Rect tileSrc (src);
	Rect tileDst (dst.left, dst.top, dst.left + src.getWidth (), dst.bottom);
	while(tileDst.bound (dst))
	{
		tileSrc.setWidth (tileDst.getWidth ());
		blitter.blit (tileSrc, tileDst);
		tileDst.offset (src.getWidth (), 0);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline void TileXY (Blitter& blitter, RectRef _margins, RectRef src, RectRef dst, bool stretchedX = false, bool stretchedY = false)
{
	Coord imageHeight = src.getHeight ();
	Coord imageWidth = src.getWidth ();
	Rect margins (_margins);

	if(dst.getWidth () < margins.left + margins.right)
	{
		margins.left = dst.getWidth () / 2;
		margins.right = dst.getWidth () - margins.left;
		//SOFT_ASSERT (false, "tile-xy - margins wider than dst");
	}

	if(dst.getHeight () < margins.top + margins.bottom)
	{
		margins.top = dst.getHeight () / 2;
		margins.bottom = dst.getHeight () - margins.top;
		//SOFT_ASSERT (false, "tile-xy - margins higher than dst");
	}

	// upper left corner
	Rect tileSrc (src.left, src.top, src.left + margins.left, src.top + margins.top);
	Rect tileDst (dst.left, dst.top, dst.left + margins.left, dst.top + margins.top);
	blitter.blit (tileSrc, tileDst);
	
	// upper center
	tileSrc (src.left + margins.left, src.top, src.left + imageWidth - margins.right, src.top + margins.top);
	tileDst (dst.left + margins.left, dst.top, dst.right - margins.right, dst.top + margins.top);
	if(stretchedX)
		blitter.blit (tileSrc, tileDst);
	else
		RepeatX (blitter, tileSrc, tileDst);

	// upper right corner
	tileSrc (src.left + src.getWidth () - margins.right, src.top, src.left + imageWidth, src.top + margins.top);
	tileDst (dst.right - margins.right, dst.top, dst.right, dst.top + margins.top);
	blitter.blit (tileSrc, tileDst);

	// right center
	tileSrc (src.left + imageWidth - margins.right, src.top + margins.top, src.left + imageWidth, src.top + imageHeight - margins.bottom);
	tileDst (dst.right - margins.right, dst.top + margins.top, dst.right, dst.bottom - margins.bottom);
	if(stretchedY)
		blitter.blit (tileSrc, tileDst);
	else
		RepeatY (blitter, tileSrc, tileDst);

	// lower left corner
	tileSrc (src.left, src.top + imageHeight - margins.bottom, src.left + margins.left, src.top + imageHeight);
	tileDst (dst.left, dst.bottom - margins.bottom, dst.left + margins.left, dst.bottom);
	blitter.blit (tileSrc, tileDst);

	// lower center
	tileSrc (src.left + margins.left, src.top + imageHeight - margins.bottom, src.left + imageWidth - margins.right, src.top + imageHeight);
	tileDst (dst.left + margins.left, dst.bottom - margins.bottom, dst.right - margins.right, dst.bottom);
	if(stretchedX)
		blitter.blit (tileSrc, tileDst);
	else
		RepeatX (blitter, tileSrc, tileDst);

	// lower right corner
	tileSrc (src.left + imageWidth - margins.right, src.top + imageHeight - margins.bottom, src.left + imageWidth, src.top + imageHeight);
	tileDst (dst.right - margins.right, dst.bottom - margins.bottom, dst.right, dst.bottom);
	blitter.blit (tileSrc, tileDst);

	// left center
	tileSrc (src.left, src.top + margins.top, src.left + margins.left, src.top + imageHeight - margins.bottom);
	tileDst (dst.left, dst.top + margins.top, dst.left + margins.left, dst.bottom - margins.bottom);
	if(stretchedY)
		blitter.blit (tileSrc, tileDst);
	else
		RepeatY (blitter, tileSrc, tileDst);

	// center
	tileSrc (src.left + margins.left, src.top + margins.top, src.left + imageWidth - margins.right, src.top + imageHeight - margins.bottom);
	tileDst (dst.left + margins.left, dst.top + margins.top, dst.right - margins.right, dst.bottom - margins.bottom);
	if(stretchedX && stretchedY)
		blitter.blit (tileSrc, tileDst);
	else if(stretchedX)
		StretchX (blitter, tileSrc, tileDst);
	else if(stretchedY)
		StretchY (blitter, tileSrc, tileDst);
	else
		RepeatXY (blitter, tileSrc, tileDst);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline void TileX (Blitter& blitter, RectRef _margins, RectRef src, RectRef dst)
{
	int width = dst.getWidth ();
	if(width == src.getWidth ())
	{
		Rect tileDst (dst.left, dst.top, dst.left + width, dst.top + src.getHeight ());
		blitter.blit (src, tileDst);
	}
	else if(width < src.getWidth ())
	{
		int leftWidth = width / 2;
		int rightWidth = width - leftWidth;
		
		Rect tileSrc (src.left, src.top, src.left + leftWidth, src.bottom);
		Rect tileDst (dst.left, dst.top, dst.left + leftWidth, dst.top + src.getHeight ());
		blitter.blit (tileSrc, tileDst);

		tileSrc (src.right - rightWidth, src.top, src.right, src.bottom);
		tileDst.offset (leftWidth, 0);
		tileDst.setWidth (rightWidth);
		blitter.blit (tileSrc, tileDst);
	}
	else
	{
		int leftWidth = src.getWidth () / 3;
		int rightWidth = leftWidth;

		Rect tileSrc (src.left, src.top, src.left + leftWidth, src.bottom);
		Rect tileDst (dst.left, dst.top, dst.left + leftWidth, dst.top + src.getHeight ());
		blitter.blit (tileSrc, tileDst);
		
		tileSrc (src.left + leftWidth, src.top, src.right - rightWidth, src.bottom);
		tileDst (dst.left + leftWidth, 0, dst.left + src.getWidth () - leftWidth, src.getHeight ());
		while (tileDst.getWidth () > 0)
		{
			blitter.blit (tileSrc, tileDst);
			tileDst.offset (tileDst.getWidth ());
			if(tileDst.right > dst.right - rightWidth)
			{
				tileDst.right = dst.right - rightWidth;
				tileSrc.setWidth (tileDst.getWidth ());
			}
		}

		tileSrc (src.right - rightWidth, src.top, src.right, src.bottom);
		tileDst (dst.right - rightWidth, 0, dst.right, dst.top + src.getHeight ());
		blitter.blit (tileSrc, tileDst);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline void TileY (Blitter& blitter, RectRef _margins, RectRef src, RectRef dst)
{
	int height = dst.getHeight ();
	if(height == src.getHeight ())
	{
		Rect tileDst (dst.left, dst.top, dst.left + src.getWidth (), dst.top + height);
		blitter.blit (src, tileDst);
	}
	else if(height < src.getHeight ())
	{
		int upperHeight = height / 2;
		int lowerHeight = height - upperHeight;
		
		Rect tileSrc (src.left, src.top, src.right, src.top + upperHeight);
		Rect tileDst (dst.left, dst.top, dst.left + src.getWidth (), dst.top + upperHeight);
		blitter.blit (tileSrc, tileDst);

		tileSrc (src.left, src.bottom - lowerHeight, src.right, src.bottom);
		tileDst.offset (0, upperHeight);
		tileDst.setHeight (lowerHeight);
		blitter.blit (tileSrc, tileDst);
	}
	else
	{
		int upperHeight = src.getHeight () / 3;
		int lowerHeight = upperHeight;

		Rect tileSrc (src.left, src.top, src.right, src.top + upperHeight);
		Rect tileDst (dst.left, dst.top, dst.left + src.getWidth (), dst.top + upperHeight);
		blitter.blit (tileSrc, tileDst);

		tileSrc (src.left, src.top + upperHeight, src.right, src.bottom - lowerHeight);
		tileDst (0, dst.top + upperHeight, src.getWidth (), dst.top + src.getHeight () - upperHeight);
		while (tileDst.getHeight () > 0)
		{
			blitter.blit (tileSrc, tileDst);
			tileDst.offset (0, tileDst.getHeight ());
			if(tileDst.bottom > dst.bottom - lowerHeight)
			{
				tileDst.bottom = dst.bottom - lowerHeight;
				tileSrc.setHeight (tileDst.getHeight ());
			}
		}

		tileSrc (src.left, src.bottom - lowerHeight, src.right, src.bottom);
		tileDst (0, dst.bottom - lowerHeight, src.getWidth (), dst.bottom);
		blitter.blit (tileSrc, tileDst);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult Tiler::tile (Blitter& blitter, int method, RectRef src, RectRef dst, RectRef clip, RectRef margins)
{
	switch (method)
	{
	case IImage::kTileY:
		TileY (blitter, margins, src, dst);
		break;
	case IImage::kTileX:
		TileX (blitter, margins, src, dst);
		break;
	case IImage::kRepeatX:
		RepeatX (blitter, src, dst);
		break;
	case IImage::kRepeatY:
		RepeatY (blitter, src, dst);
		break;
	case IImage::kTileXY:
		TileXY (blitter, margins, src, dst);
		break;
	case IImage::kRepeatXY:
		RepeatXY (blitter, src, dst);
		break;
	case IImage::kStretchXY:
		TileXY (blitter, margins, src, dst, true, true);
		break;
	case IImage::kStretchX:
		TileXY (blitter, margins, src, dst, true, false);
		break;
	case IImage::kStretchY:
		TileXY (blitter, margins, src, dst, false, true);
		break;
	}
	return kResultTrue;
}
