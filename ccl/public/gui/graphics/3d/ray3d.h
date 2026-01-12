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
// Filename    : ccl/public/gui/graphics/3d/ray3d.h
// Description : 3D ray class
//
//************************************************************************************************

#ifndef _ccl_ray3d_h
#define _ccl_ray3d_h

#include "ccl/public/gui/graphics/3d/point3d.h"

namespace CCL {

struct Ray3D;

/** 3D Ray reference. */
typedef const Ray3D& Ray3DRef;

//************************************************************************************************
// Ray3D
/** 3D Ray with an origin and direction in 3D space.
	\ingroup gui_graphics3d */
//************************************************************************************************

struct Ray3D
{
	PointF3D origin;
	PointF3D direction;

	Ray3D (PointF3DRef origin = {}, PointF3DRef direction = {})
	: origin (origin),
	  direction (direction)
	{}

	/** Get the point on the ray at a given distance. */
	PointF3D operator* (float t) const;

	/**	Test whether this ray intersects a sphere.
		The return value indicates whether an intersection occurs.
		The hitDistance out-parameter is the distance to the first intersection, or 0 if the ray's origin lies inside the sphere. */
	bool intersectsSphere (float& hitDistance, PointF3DRef center, float radius) const;
};

} // namespace CCL

#endif // _ccl_ray3d_h
