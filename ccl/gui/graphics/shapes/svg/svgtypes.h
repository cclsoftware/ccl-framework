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
// Filename    : ccl/gui/graphics/shapes/svg/svgtypes.h
// Description : SVG Types & Utilities
//
//************************************************************************************************

#ifndef _ccl_svgtypes_h
#define _ccl_svgtypes_h

#include "ccl/public/gui/graphics/types.h"

namespace CCL {
namespace SVG {

//////////////////////////////////////////////////////////////////////////////////////////////////

typedef CoordF Length;

//////////////////////////////////////////////////////////////////////////////////////////////////

inline CoordF makeCoordF (Length c)								{ return c; }
inline PointF makePointF (Length x, Length y)					{ return PointF (x, y); }
inline RectF makeRectF (Length l, Length t, Length r, Length b)	{ return RectF (l, t, r, b); }

/* not used anymore:
inline Coord makeCoord (Length c)								{ return (Coord)c; }
inline Point makePoint (Length x, Length y)						{ return Point ((Coord)x, (Coord)y); }
inline Rect makeRect (Length l, Length t, Length r, Length b)	{ return Rect ((Coord)l, (Coord)t, (Coord)r, (Coord)b); }
*/
//////////////////////////////////////////////////////////////////////////////////////////////////

} // namespace SVG
} // namespace CCL

#endif // _ccl_svgtypes_h
