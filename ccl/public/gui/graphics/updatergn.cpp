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
// Filename    : ccl/public/gui/graphics/updatergn.cpp
// Description : Update Region
//
//************************************************************************************************

#include "ccl/public/gui/graphics/updatergn.h"

using namespace CCL;

//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_IID_ (IUpdateRegion, 0x70012865, 0x7274, 0x4a4d, 0xac, 0x9d, 0x77, 0xd, 0xac, 0xc7, 0x92, 0x4f)
DEFINE_IID_ (IMutableRegion, 0x2229c9e8, 0xface, 0x4f9d, 0x98, 0x55, 0x53, 0xb8, 0x8, 0x0, 0xe1, 0xc0)

//************************************************************************************************
// UpdateRgn
//************************************************************************************************

UpdateRgn::UpdateRgn (const UpdateRgn& other, RectRef subPart)
: region (other.region)
{
	bounds = other.bounds;
	bounds.bound (subPart);
	bounds.offset (-subPart.left, -subPart.top);

	offset = other.offset;
	offset.offset (subPart.left, subPart.top);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool UpdateRgn::isEmpty () const
{
	return bounds.isEmpty ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool UpdateRgn::rectVisible (RectRef rect) const
{
	if(region)
	{
		Rect rect2 (rect);
		rect2.offset (offset);
		return region->rectVisible (rect2) != 0;
	}
	else
		return bounds.intersect (rect);
}
