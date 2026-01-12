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
// Filename    : ccl/public/gui/framework/itemviewgeometry.cpp
// Description : Item View Geometry
//
//************************************************************************************************

#include "ccl/public/gui/framework/itemviewgeometry.h"
#include "ccl/public/gui/framework/iview.h"

using namespace CCL;

//************************************************************************************************
// ItemViewGeometry
//************************************************************************************************

ItemViewGeometry::ItemViewGeometry ()
: vertical (true),
  indicatorWidth (4)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

int ItemViewGeometry::getRelation (bool& upperHalf, RectRef itemRect, PointRef p)
{
	Coord x, start, end;
	if(isVertical ())
		x = p.y, start = itemRect.top, end = itemRect.bottom;
	else
		x = p.x, start = itemRect.left, end = itemRect.right;

	static constexpr Coord kItemRelationGap = 4;
	if(x < start + kItemRelationGap)
	{
		upperHalf = true;
		return kBeforeItem;
	}
	else if(x >= end - kItemRelationGap)
	{
		upperHalf = false;
		return kAfterItem;
	}
	else
	{
		upperHalf = (x - start) < (end - start) / 2;
		return kOnItem;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Rect ItemViewGeometry::calcSpriteSize (RectRef containerRect, RectRef itemRect, int relation)
{
	Rect rect;
	Coord& start = isVertical () ? rect.top : rect.left;
	Coord& end   = isVertical () ? rect.bottom : rect.right;

	if(!itemRect.isEmpty ())
	{
		rect = itemRect;

		if(relation == kBeforeItem)
		{
			start -= indicatorWidth / 2;
			ccl_lower_limit (start, 0);
			end = start + indicatorWidth;
		}
		else if(relation == kAfterItem)
		{
			end += indicatorWidth / 2;
			start = end - indicatorWidth;
		}
	}
	else
	{
		// at end of view
		rect = containerRect;
		end += indicatorWidth / 2;
		start = end - indicatorWidth;
	}
	return rect;
}
