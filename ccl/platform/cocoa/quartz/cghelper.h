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
// Filename    : ccl/platform/cocoa/quartz/helper.h
// Description : Quartz Helper
//
//************************************************************************************************

#ifndef _ccl_quartz_helper_h
#define _ccl_quartz_helper_h

#include "ccl/public/gui/graphics/types.h"

#include <CoreGraphics/cggeometry.h>

namespace CCL {
namespace MacOS {

//////////////////////////////////////////////////////////////////////////////////////////////////
	
#define PRINT_CGPOINT(s,p) CCL_PRINTF ("%s(%d, %d)\n", s, (int)p.x, (int)p.y)

#define PRINT_CGRECT(s,r) CCL_PRINTF ("%s(%d, %d) (%d x %d)\n", s, (int)r.origin.x, (int)r.origin.y, (int)r.size.width, (int)r.size.height)

//////////////////////////////////////////////////////////////////////////////////////////////////

inline CCL::Rect& fromCGRect (CCL::Rect& dst, const ::CGRect& src)
{
	dst.left = (Coord)src.origin.x;
	dst.top = (Coord)src.origin.y;
	dst.right = (Coord)(src.origin.x + src.size.width);
	dst.bottom = (Coord)(src.origin.y + src.size.height);
	return dst;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline ::CGRect& toCGRect (::CGRect& dst, const CCL::Rect& src)
{
	dst.origin.x = src.left;
	dst.origin.y = src.top;
	dst.size.width = src.right - src.left;
	dst.size.height = src.bottom - src.top;
	return dst;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline CCL::RectF& fromCGRect (CCL::RectF& dst, const ::CGRect& src)
{
	dst.left = (CoordF)src.origin.x;
	dst.top = (CoordF)src.origin.y;
	dst.right = (CoordF)(src.origin.x + src.size.width);
	dst.bottom = (CoordF)(src.origin.y + src.size.height);
	return dst;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline ::CGRect& toCGRect (::CGRect& dst, const CCL::RectF& src)
{
	dst.origin.x = src.left;
	dst.origin.y = src.top;
	dst.size.width = src.right - src.left;
	dst.size.height = src.bottom - src.top;
	return dst;
}
	
//////////////////////////////////////////////////////////////////////////////////////////////////

} // namespace MacOS
} // namespace CCL

#endif // _ccl_quartz_helper_h
