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
// Filename    : ccl/gui/graphics/mutableregion.cpp
// Description : Mutable Region
//
//************************************************************************************************

#include "ccl/gui/graphics/mutableregion.h"

using namespace CCL;

//************************************************************************************************
// MutableRegion
//************************************************************************************************

DEFINE_CLASS (MutableRegion, Object)
DEFINE_CLASS_UID (MutableRegion, 0xB3FD9505, 0x1594, 0x42D2, 0xB0, 0x15, 0xA4, 0x1D, 0xC0, 0x3E, 0x44, 0x9D) // ClassID::MutableRegion

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API MutableRegion::addRect (RectRef rect)
{
	rects.join (rect);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API MutableRegion::rectVisible (RectRef rect) const
{
	VectorForEach (rects.getRects (), Rect&, r)
		if(rect.intersect (r))
			return true;
	EndFor
	
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API MutableRegion::setEmpty ()
{
	rects.setEmpty ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Rect CCL_API MutableRegion::getBoundingBox () const
{
	return rects.getBoundingBox ();
}

//************************************************************************************************
// SelectionRegion
//************************************************************************************************

DEFINE_CLASS (SelectionRegion, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API SelectionRegion::addRect (RectRef rect)
{
	rects.add (rect);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API SelectionRegion::setEmpty ()
{
	rects.removeAll ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API SelectionRegion::rectVisible (RectRef rect) const
{
	for(RectRef r : rects)
		if(r.intersect (rect))
			return true;

	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Rect CCL_API SelectionRegion::getBoundingBox () const
{
	Rect bounds;
	bounds.setReallyEmpty ();

	for(RectRef r : rects)
		bounds.join (r);

	return bounds;
}
