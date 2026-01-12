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
// Filename    : ccl/public/gui/graphics/3d/ray3d.cpp
// Description : 3D ray class
//
//************************************************************************************************

#include "ccl/public/gui/graphics/3d/ray3d.h"

#include "ccl/public/math/mathprimitives.h"

using namespace CCL;

//************************************************************************************************
// Ray3D
//************************************************************************************************

PointF3D Ray3D::operator * (float t) const
{
	return origin + direction * t;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Ray3D::intersectsSphere (float& hitDistance, PointF3DRef center, float radius) const
{
	// translate ray origin to be relative to the center of the sphere
	PointF3D relativeOrigin = origin - center;

	// check if origin is inside the sphere
	const float sphereRadiusSquared = radius * radius;
	if(relativeOrigin.lengthSquared () <= sphereRadiusSquared)
	{
		hitDistance = 0.f;
		return true;
	}

	// definitions:
	//  O = relativeOrigin
	//  D = direction
	//  r = sphere.radius
	//  t = hitDistance

	// solve for t:
	//  |O + tD|^2 - r^2 = 0
	//  O^2 + 2OtD + (tD)^2 - r^2 = 0
	//  (D^2)t^2 + (2OD)t + (O^2-r^2) = 0

	// use a quadratic solver:
	//  a = D^2
	//  b = OD
	//  c = O^2 - r^2

	const float b = relativeOrigin.dot (direction.normal ());
	const float c = relativeOrigin.lengthSquared () - sphereRadiusSquared;

	const float determinant = b * b - c;
	if(determinant < 0)
	{
		// the ray doesn't hit the sphere at all
		hitDistance = -NumericLimits::kMaximumFloat;
		return false;
	}

	const float sqrtDet = ::sqrtf (determinant);
	const float t1 = -b - sqrtDet;
	const float t2 = -b + sqrtDet;
	if(t1 < 0 && t2 < 0)
	{
		// the ray enters the sphere behind the origin
		// since we already excluded the case where the origin is inside the sphere, the sphere is entirely opposite to the direction of the ray
		hitDistance = -NumericLimits::kMaximumFloat;
		return false;
	}

	// the ray hits the sphere
	hitDistance = t1 < 0 ? t2 : (t2 < 0 ? t1 : ccl_min (t1, t2));
	return true;
}
