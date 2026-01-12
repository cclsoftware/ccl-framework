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
// Filename    : ccl/public/gui/framework/itemviewgeometry.h
// Description : Item View Geometry
//
//************************************************************************************************

#ifndef _ccl_itemviewgeometry_h
#define _ccl_itemviewgeometry_h

#include "ccl/public/gui/graphics/rect.h"
#include "ccl/public/base/cclmacros.h"

namespace CCL {

//************************************************************************************************
// ItemViewGeometry
//************************************************************************************************

class ItemViewGeometry
{
public:
	ItemViewGeometry ();

	PROPERTY_BOOL (vertical, Vertical)
	PROPERTY_VARIABLE (Coord, indicatorWidth, IndicatorWidth)

	enum ItemRelation
	{
		kOnItem,
		kBeforeItem,
		kAfterItem
	};

	/** Get relation between point & item rect. */
	int getRelation (bool& upperHalf, RectRef itemRect, PointRef p);

	/** Calculate size of a sprite for the given relation. */
	Rect calcSpriteSize (RectRef containerRect, RectRef itemRect, int relation);
};

} // namespace CCL

#endif // _ccl_itemviewgeometry_h
