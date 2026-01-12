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
// Filename    : ccl/public/gui/graphics/3d/point3d.cpp
// Description : 3D Point class
//
//************************************************************************************************

#include "ccl/public/gui/graphics/3d/point3d.h"

#include "ccl/public/math/mathprimitives.h"

using namespace CCL;

//************************************************************************************************
// PointF3D
//************************************************************************************************

float PointF3D::lengthSquared () const
{
	return x * x + y * y + z * z;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

float PointF3D::length () const
{
	return ::sqrtf (lengthSquared ());
}

///////////////////////////////////////////////////////////////////////////////////////////////////

float PointF3D::distanceToSquared (PointF3DRef p) const
{
	const float dx = p.x - x;
	const float dy = p.y - y;
	const float dz = p.z - z;
	return dx * dx + dy * dy + dz * dz;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

float PointF3D::distanceTo (PointF3DRef p) const
{
	return ::sqrtf (distanceToSquared (p));
}

///////////////////////////////////////////////////////////////////////////////////////////////////

float PointF3D::dot (PointF3DRef p) const
{
	return x * p.x + y * p.y + z * p.z;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

PointF3D PointF3D::normal () const
{
	float n = length ();
	return PointF3D (x / n, y / n, z / n);
}

///////////////////////////////////////////////////////////////////////////////////////////////////

PointF3D PointF3D::cross (PointF3DRef p) const
{
	return PointF3D (
		y * p.z - z * p.y,
		z * p.x - x * p.z,
		x * p.y - y * p.x);
}

///////////////////////////////////////////////////////////////////////////////////////////////////

PointF3D& PointF3D::bound (PointF3DRef min, PointF3DRef max)
{
	x = ccl_bound (x, min.x, max.x);
	y = ccl_bound (y, min.y, max.y);
	z = ccl_bound (z, min.z, max.z);
	return *this;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

PointF3D& PointF3D::operator *= (float n)
{
	*this = *this * n;
	return *this;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

PointF3D& PointF3D::operator += (PointF3DRef rhs)
{
	*this = *this + rhs;
	return *this;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

PointF3D& PointF3D::operator -= (PointF3DRef rhs)
{
	*this = *this - rhs;
	return *this;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

PointF3D PointF3D::operator * (float n) const
{
	return PointF3D (x * n, y * n, z * n);
}

///////////////////////////////////////////////////////////////////////////////////////////////////

PointF3D PointF3D::operator + (PointF3DRef rhs) const
{
	return PointF3D (x + rhs.x, y + rhs.y, z + rhs.z);
}

///////////////////////////////////////////////////////////////////////////////////////////////////

PointF3D PointF3D::operator - (PointF3DRef rhs) const
{
	return PointF3D (x - rhs.x, y - rhs.y, z - rhs.z);
}

///////////////////////////////////////////////////////////////////////////////////////////////////

PointF3D PointF3D::operator - () const
{
	return PointF3D (-x, -y, -z);
}

///////////////////////////////////////////////////////////////////////////////////////////////////

bool PointF3D::operator == (PointF3DRef rhs) const
{
	return x == rhs.x && y == rhs.y && z == rhs.x;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

bool PointF3D::operator != (PointF3DRef rhs) const
{
	return !(*this == rhs);
}
