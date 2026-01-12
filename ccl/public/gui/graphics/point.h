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
// Filename    : ccl/public/gui/graphics/point.h
// Description : Point class
//
//************************************************************************************************

#ifndef _ccl_point_h
#define _ccl_point_h

#include "ccl/public/base/primitives.h"

#include "core/public/gui/corepoint.h"

namespace CCL {

using Core::Coord;
using Core::CoordF;

using Core::Point;
using Core::PointRef;

using Core::PointF;
using Core::PointFRef;

using Core::TPoint;

using Core::kMaxCoord;
using Core::kMinCoord;

//////////////////////////////////////////////////////////////////////////////////////////////////

/** Bound coordinate to limits. 
	\ingroup gui_graphics */
inline Coord boundCoord (Coord c)
{
	return ccl_bound<Coord> (c, kMinCoord, kMaxCoord);
}

/** Convert float to integer coordinate. */
inline Coord coordFToInt (CoordF c)
{
	return ccl_to_int (c);
}

/** Convert float to integer point. */
inline Point pointFToInt (PointFRef p)
{
	return Point (coordFToInt (p.x), coordFToInt (p.y));
}

/** Convert integer to float point. */
inline PointF pointIntToF (PointRef p)
{
	return PointF ((CoordF)p.x, (CoordF)p.y);
}

/** Get intersection of line segments a and b. (float) */
inline bool getIntersectionPointF (PointF& result, PointFRef a1, PointFRef a2, PointFRef b1, PointFRef b2)
{
	double det = ((a1.x - a2.x) * (b1.y - b2.y)) - ((a1.y - a2.y) * (b1.x - b2.x));
	if(det == 0)
		return false; // a and b collinear
		
	double pre = ((a1.x * a2.y) - (a1.y * a2.x)); 
	double post = ((b1.x * b2.y) - (b1.y * b2.x));
	double x = ((pre * (b1.x - b2.x)) - ((a1.x - a2.x) * post)) / det;
	double y = ((pre * (b1.y - b2.y)) - ((a1.y - a2.y) * post)) / det;
		
	static constexpr double kEpsilon = 0.5; // to exclude rounding errors 
	 	
	if(((x+kEpsilon) < ccl_min<double>(a1.x, a2.x)) || ((x-kEpsilon) > ccl_max<double>(a1.x, a2.x)) || ((x+kEpsilon) < ccl_min<double>(b1.x, b2.x)) || ((x-kEpsilon) > ccl_max<double>(b1.x, b2.x)))
		return false; // x not within line segments
	if(((y+kEpsilon) < ccl_min<double>(a1.y, a2.y)) || ((y-kEpsilon) > ccl_max<double>(a1.y, a2.y)) || ((y+kEpsilon) < ccl_min<double>(b1.y, b2.y)) || ((y-kEpsilon) > ccl_max<double>(b1.y, b2.y)))
		return false; // y not within line segments
	
	result.x = x;
	result.y = y;
	return true;
}
			
/** Get intersection of line segments a and b. (integer) */
inline bool getIntersectionPoint (Point& result, PointRef a1, PointRef a2, PointRef b1, PointRef b2)
{
	PointF resultF;
	if(getIntersectionPointF (resultF, pointIntToF(a1), pointIntToF(a2), pointIntToF(b1), pointIntToF(b2)))
	{
		result.x = ccl_to_int (resultF.x);
		result.y = ccl_to_int (resultF.y);
		return true;
	}
	return false;
}

} // namespace CCL

#endif // _ccl_point_h
