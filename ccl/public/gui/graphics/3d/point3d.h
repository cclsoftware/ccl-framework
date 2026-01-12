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
// Filename    : ccl/public/gui/graphics/3d/point3d.h
// Description : 3D Point class
//
//************************************************************************************************

#ifndef _ccl_point3d_h
#define _ccl_point3d_h

#include "ccl/public/gui/graphics/point.h"

namespace CCL {

struct PointF3D;
struct PointF4D;

/** 3D Point reference. */
typedef const PointF3D& PointF3DRef;

/** 4D Point reference. */
typedef const PointF4D& PointF4DRef;

//************************************************************************************************
// PointF3D
/** 3D Point with float coordinates.
	\ingroup gui_graphics3d */
//************************************************************************************************

struct PointF3D
{
	CoordF x;
	CoordF y;
	CoordF z;

	PointF3D (CoordF x = 0, CoordF y = 0, CoordF z = 0)
	: x (x),
	  y (y),
	  z (z)
	{}

	float lengthSquared () const;
	float length () const;

	float distanceToSquared (PointF3DRef p) const;
	float distanceTo (PointF3DRef p) const;
	
	float dot (PointF3DRef p) const;	
	PointF3D normal () const;
	PointF3D cross (PointF3DRef p) const;

	PointF3D& bound (PointF3DRef min, PointF3DRef max);

	PointF3D operator * (float n) const;
	PointF3D operator + (PointF3DRef rhs) const;
	PointF3D operator - (PointF3DRef rhs) const;
	PointF3D operator - () const;

	PointF3D& operator *= (float n);
	PointF3D& operator += (PointF3DRef rhs);
	PointF3D& operator -= (PointF3DRef rhs);

	bool operator == (PointF3DRef rhs) const;
	bool operator != (PointF3DRef rhs) const;
};

inline PointF3D operator * (float n, PointF3DRef p) { return p * n; }

//************************************************************************************************
// PointF4D
/** 4D Point with float coordinates.
	\ingroup gui_graphics3d */
//************************************************************************************************

struct PointF4D
{
	CoordF x;
	CoordF y;
	CoordF z;
	CoordF w;

	PointF4D (CoordF x = 0, CoordF y = 0, CoordF z = 0, CoordF w = 0)
	: x (x),
	  y (y),
	  z (z),
	  w (w)
	{}

	explicit PointF4D (PointF3DRef rhs, CoordF w = 1)
	: x (rhs.x),
	  y (rhs.y),
	  z (rhs.z),
	  w (w)
	{}
};

} // namespace CCL

#endif // _ccl_point3d_h
