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
// Filename    : ccl/public/gui/graphics/rect.cpp
// Description : Rectangle class
//
//************************************************************************************************

#include "ccl/public/gui/graphics/rect.h"

#include "ccl/public/base/debug.h"

using namespace CCL;

//////////////////////////////////////////////////////////////////////////////////////////////////

#if DEBUG
void CCL::dumpRect (const Rect& rect, const char* string)
{
	Debugger::printf ("%s (%d, %d)-(%d, %d) %d x %d\n",
					  string ? string : "CCL::Rect",
					  rect.left, rect.top, rect.right, rect.bottom,
					  rect.getWidth (), rect.getHeight ());
}
#endif

//************************************************************************************************
// SizeLimit
//************************************************************************************************

SizeLimit& SizeLimit::include (const SizeLimit& limits)
{
	ccl_lower_limit (minWidth,  limits.minWidth);
	ccl_lower_limit (minHeight, limits.minHeight);
	ccl_upper_limit (maxWidth,  limits.maxWidth);
	ccl_upper_limit (maxHeight, limits.maxHeight);
	return *this;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

SizeLimit& SizeLimit::resolveConflicts ()
{
	if(maxWidth < minWidth)
		maxWidth = minWidth;
	if(maxHeight < minHeight)
		maxHeight = minHeight;
	return *this;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Point& SizeLimit::makeValid (Point& size) const
{
	size.x = ccl_bound (size.x, minWidth, maxWidth);
	size.y = ccl_bound (size.y, minHeight, maxHeight);
	return size;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Rect& SizeLimit::makeValid (Rect& rect) const
{
	Coord w = rect.getWidth ();
	if(w < minWidth)
		rect.setWidth (minWidth);
	else if(w > maxWidth)
		rect.setWidth (maxWidth);

	Coord h = rect.getHeight ();
	if(h < minHeight)
		rect.setHeight (minHeight);
	else if(h > maxHeight)
		rect.setHeight (maxHeight);

	return rect;
}
