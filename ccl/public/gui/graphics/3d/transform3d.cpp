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
// Filename    : ccl/public/gui/graphics/3d/transform3d.cpp
// Description : 3D Transformation Matrix
//
//************************************************************************************************

#include "ccl/public/gui/graphics/3d/transform3d.h"

#include "ccl/public/math/mathprimitives.h"

using namespace CCL;

//************************************************************************************************
// Transform3D
// note: for typical transformations, the last row is 0 0 0 1. If we were sure we don't need other cases,
// we could optimize this later by turning v41 .. v44 into constants (constexpr) and removing their assignments
//************************************************************************************************

Transform3D& Transform3D::translate (float tx, float ty, float tz)
{
//	| v11 v12 v13 v14 |   | 1  0  0  tx |   | v11  v12  v13  (v11 * tx + v12 * ty + v13 * tz + v14) |
//	| v21 v22 v23 v24 | x | 0  1  0  ty | = | v21  v22  v23  (v21 * tx + v22 * ty + v23 * tz + v24) |
//	| v31 v32 v33 v34 |   | 0  0  1  tz |   | v31  v32  v33  (v31 * tx + v32 * ty + v33 * tz + v34) |
//	| v41 v42 v43 v44 |   | 0  0  0   1 |   | v41  v42  v43  (v41 * tx + v42 * ty + v43 * tz + v44) |
	v14 += v11 * tx + v12 * ty + v13 * tz;
	v24 += v21 * tx + v22 * ty + v23 * tz;
	v34 += v31 * tx + v32 * ty + v33 * tz;
	v44 += v41 * tx + v42 * ty + v43 * tz;
	return *this;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Transform3D& Transform3D::scale (float sx, float sy, float sz)
{
//	| v11 v12 v13 v14 |   | sx  0  0   0 |   | (v11 * sx)  (v12 * sy)  (v13 * sz)  v14 |
//	| v21 v22 v23 v24 | x | 0  sy  0   0 | = | (v21 * sx)  (v22 * sy)  (v23 * sz)  v24 |
//	| v31 v32 v33 v34 |   | 0   0  sz  0 |   | (v31 * sx)  (v32 * sy)  (v33 * sz)  v34 |
//	| v41 v42 v43 v44 |   | 0   0  0   1 |   | (v41 * sx)  (v42 * sy)  (v43 * sz)  v44 |
	v11 *= sx;
	v12 *= sy;
	v13 *= sz;

	v21 *= sx;
	v22 *= sy;
	v23 *= sz;

	v31 *= sx;
	v32 *= sy;
	v33 *= sz;

	v41 *= sx;
	v42 *= sy;
	v43 *= sz;
	return *this;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Transform3D& Transform3D::rotateX (float angle)
{
//	| v11 v12 v13 v14 |   | 1   0         0     0 |   | v11  (v12*cos(a) + v13*sin(a))  (-v12*sin(a) + v13*cos(a))  v14 |
//	| v21 v22 v23 v24 | x | 0  cos(a)  -sin(a)  0 | = | v21  (v22*cos(a) + v23*sin(a))  (-v22*sin(a) + v23*cos(a))  v24 |
//	| v31 v32 v33 v34 |   | 0  sin(a)   cos(a)  0 |   | v31  (v32*cos(a) + v33*sin(a))  (-v32*sin(a) + v33*cos(a))  v34 |
//	| v41 v42 v43 v44 |   | 0    0        0     1 |   | v41  (v42*cos(a) + v43*sin(a))  (-v42*sin(a) + v43*cos(a))  v44 |
	float cosa = cosf (angle);
	float sina = sinf (angle);

	float v12old = v12;
	v12 = v12 * cosa + v13 * sina;
	v13 = -v12old * sina + v13 * cosa;

	float v22old = v22;
	v22 = v22 * cosa + v23 * sina;
	v23 = -v22old * sina + v23 * cosa;

	float v32old = v32;
	v32 = v32 * cosa + v33 * sina;
	v33 = -v32old * sina + v33 * cosa;

	float v42old = v42;
	v42 = v42 * cosa + v43 * sina;
	v43 = -v42old * sina + v43 * cosa;
	return *this;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Transform3D& Transform3D::rotateY (float angle)
{
//	| v11 v12 v13 v14 |   |  cos(a)  0  sin(a)  0 |   | (v11*cos(a) - v13*sin(a))  v12  (v11*sin(a) + v13*cos(a))  v14 |
//	| v21 v22 v23 v24 | x |   0      1    0     0 | = | (v21*cos(a) - v23*sin(a))  v22  (v21*sin(a) + v23*cos(a))  v24 |
//	| v31 v32 v33 v34 |   | -sin(a)  0  cos(a)  0 |   | (v31*cos(a) - v33*sin(a))  v32  (v31*sin(a) + v33*cos(a))  v34 |
//	| v41 v42 v43 v44 |   |   0      0    0     1 |   | (v41*cos(a) - v43*sin(a))  v42  (v41*sin(a) + v43*cos(a))  v44 |
	float cosa = cosf (angle);
	float sina = sinf (angle);

	float v11old = v11;
	v11 = v11 * cosa - v13 * sina;
	v13= v11old * sina + v13 * cosa;

	float v21old = v21;
	v21 = v21 * cosa - v23 * sina;
	v23 = v21old * sina + v23 * cosa;

	float v31old = v31;
	v31 = v31 * cosa - v33 * sina;
	v33 = v31old * sina + v33 * cosa;

	float v41old = v41;
	v41 = v41 * cosa - v43 * sina;
	v43 = v41old * sina + v43 * cosa;
	return *this;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Transform3D& Transform3D::rotateZ (float angle)
{
//	| v11 v12 v13 v14 |   | cos(a)  -sin(a)  0  0 |   | (v11*cos(a) + v12*sin(a))  (-v11*sin(a) + v12*cos(a))  v13  v14 |
//	| v21 v22 v23 v24 | x | sin(a)  cos(a)   0  0 | = | (v21*cos(a) + v22*sin(a))  (-v21*sin(a) + v22*cos(a))  v23  v24 |
//	| v31 v32 v33 v34 |   |   0       0      1  0 |   | (v31*cos(a) + v32*sin(a))  (-v31*sin(a) + v32*cos(a))  v33  v34 |
//	| v41 v42 v43 v44 |   |   0       0      0  1 |   | (v41*cos(a) + v42*sin(a))  (-v41*sin(a) + v42*cos(a))  v43  v44 |
	float cosa = cosf (angle);
	float sina = sinf (angle);

	float v11old = v11;
	v11 = v11 * cosa + v12 * sina;
	v12 = -v11old * sina + v12 * cosa;

	float v21old = v21;
	v21 = v21 * cosa + v22 * sina;
	v22 = -v21old * sina + v22 * cosa;

	float v31old = v31;
	v31 = v31 * cosa + v32 * sina;
	v32 = -v31old * sina + v32 * cosa;

	float v41old = v41;
	v41 = v41 * cosa + v42 * sina;
	v42 = -v41old * sina + v42 * cosa;
	return *this;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Transform3D Transform3D::operator * (Transform3DRef rhs) const
{
//	| v11 v12 v13 v14 |   | r11 r12 r13 r14 |   | (v11*r11 + v12*r21 + v13*r31 + v14*r41)  (v11*r12 + v12*r22 + v13*r32 + v14*r42)  (v11*r13 + v12*r23 + v13*r33 + v14*r43)  (v11*r14 + v12*r24 + v13*r34 + v14*r44) |
//	| v21 v22 v23 v24 | x | r21 r22 r23 r24 | = | (v21*r11 + v22*r21 + v23*r31 + v24*r41)  (v21*r12 + v22*r22 + v23*r32 + v24*r42)  (v21*r13 + v22*r23 + v23*r33 + v24*r43)  (v21*r14 + v22*r24 + v23*r34 + v24*r44) |
//	| v31 v32 v33 v34 |   | r31 r32 r33 r34 |   | (v31*r11 + v32*r21 + v33*r31 + v34*r41)  (v31*r12 + v32*r22 + v33*r32 + v34*r42)  (v31*r13 + v32*r23 + v33*r33 + v34*r43)  (v31*r14 + v32*r24 + v33*r34 + v34*r44) |
//	| v41 v42 v43 v44 |   | r41 r42 r43 r44 |   | (v41*r11 + v42*r21 + v43*r31 + v44*r41)  (v41*r12 + v42*r22 + v43*r32 + v44*r42)  (v41*r13 + v42*r23 + v43*r33 + v44*r43)  (v41*r14 + v42*r24 + v43*r34 + v44*r44) |
	Transform3D result;
	result.v11 = v11 * rhs.v11 + v12 * rhs.v21 + v13 * rhs.v31 + v14 * rhs.v41;
	result.v12 = v11 * rhs.v12 + v12 * rhs.v22 + v13 * rhs.v32 + v14 * rhs.v42;
	result.v13 = v11 * rhs.v13 + v12 * rhs.v23 + v13 * rhs.v33 + v14 * rhs.v43;
	result.v14 = v11 * rhs.v14 + v12 * rhs.v24 + v13 * rhs.v34 + v14 * rhs.v44;

	result.v21 = v21 * rhs.v11 + v22 * rhs.v21 + v23 * rhs.v31 + v24 * rhs.v41;
	result.v22 = v21 * rhs.v12 + v22 * rhs.v22 + v23 * rhs.v32 + v24 * rhs.v42;
	result.v23 = v21 * rhs.v13 + v22 * rhs.v23 + v23 * rhs.v33 + v24 * rhs.v43;
	result.v24 = v21 * rhs.v14 + v22 * rhs.v24 + v23 * rhs.v34 + v24 * rhs.v44;

	result.v31 = v31 * rhs.v11 + v32 * rhs.v21 + v33 * rhs.v31 + v34 * rhs.v41;
	result.v32 = v31 * rhs.v12 + v32 * rhs.v22 + v33 * rhs.v32 + v34 * rhs.v42;
	result.v33 = v31 * rhs.v13 + v32 * rhs.v23 + v33 * rhs.v33 + v34 * rhs.v43;
	result.v34 = v31 * rhs.v14 + v32 * rhs.v24 + v33 * rhs.v34 + v34 * rhs.v44;

	result.v41 = v41 * rhs.v11 + v42 * rhs.v21 + v43 * rhs.v31 + v44 * rhs.v41;
	result.v42 = v41 * rhs.v12 + v42 * rhs.v22 + v43 * rhs.v32 + v44 * rhs.v42;
	result.v43 = v41 * rhs.v13 + v42 * rhs.v23 + v43 * rhs.v33 + v44 * rhs.v43;
	result.v44 = v41 * rhs.v14 + v42 * rhs.v24 + v43 * rhs.v34 + v44 * rhs.v44;
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Transform3D Transform3D::getInverseTransform () const
{
	ASSERT (v41 == 0 && v42 == 0 && v43 == 0 && v44 == 1)

	float det = v11 * v22 * v33
			  + v21 * v32 * v13
			  + v31 * v12 * v23
			  - v31 * v22 * v13
			  - v21 * v12 * v33
			  - v11 * v32 * v23;

	if(det == 0.f)
		return Transform3D ();
	
	float invDet = 1.f / det;

	Transform3D result;
	result.v11 = (v22 * v33 - v32 * v23) * invDet;
	result.v12 = -(v12 * v33 - v32 * v13) * invDet;
	result.v13 = (v12 * v23 - v22 * v13) * invDet;
	result.v14 = -(v14 * result.v11 + v24 * result.v12 + v34 * result.v13);
	result.v21 = -(v21 * v33 - v31 * v23) * invDet;
	result.v22 = (v11 * v33 - v31 * v13) * invDet;
	result.v23 = -(v11 * v23 - v21 * v13) * invDet;
	result.v24 = -(v14 * result.v21 + v24 * result.v22 + v34 * result.v23);
	result.v31 = (v21 * v32 - v31 * v22) * invDet;
	result.v32 = -(v11 * v32 - v31 * v12) * invDet;
	result.v33 = (v11 * v22 - v21 * v12) * invDet;
	result.v34 = -(v14 * result.v31 + v24 * result.v32 + v34 * result.v33);

	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Transform3D::resetTranslation ()
{
	v14 = 0.f;
	v24 = 0.f;
	v34 = 0.f;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Transform3D::resetRotation ()
{
	PointF3D s;
	getScale (s);

	v11 = s.x;
	v12 = 0.f;
	v13 = 0.f;

	v21 = 0.f;
	v22 = s.y;
	v23 = 0.f;

	v31 = 0.f;
	v32 = 0.f;
	v33 = s.z;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Transform3D::resetScale ()
{
	PointF3D s;
	getScale (s);
	scale (s.x != 0.f ? 1.f / s.x : 1.f, s.y != 0.f ? 1.f / s.y : 1.f, s.z != 0.f ? 1.f / s.z : 1.f);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Transform3D::getTranslation (PointF3D& translation) const
{
	translation.x = v14;
	translation.y = v24;
	translation.z = v34;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Transform3D::getRotation (PointF3D& rotation) const
{
	PointF3D scale;
	getScale (scale);

	if(scale.x == 0 || scale.y == 0 || scale.z == 0)
	{
		rotation = {0, 0, 0};
		return;
	}

	Transform3D unscaled (*this);
	unscaled.scale (1.f / scale.x, 1.f / scale.y, 1.f / scale.z);

	rotation.y = ::atan2f (-unscaled.v31, ::sqrtf (unscaled.v32 * unscaled.v32 + unscaled.v33 * unscaled.v33));
	if(ccl_abs (ccl_abs (rotation.y) - Math::Constants<float>::kHalfPi) > 0.00001f)
	{
		rotation.x = ::atan2f (unscaled.v32, unscaled.v33);
		rotation.z = ::atan2f (unscaled.v21, unscaled.v11);
	}
	else
	{
		rotation.x = 0;
		rotation.z = ::atan2f (unscaled.v12, unscaled.v22);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Transform3D::getScale (PointF3D& scale) const
{
	scale.x = ::sqrtf (v11 * v11 + v21 * v21 + v31 * v31);
	scale.y = ::sqrtf (v12 * v12 + v22 * v22 + v32 * v32);
	scale.z = ::sqrtf (v13 * v13 + v23 * v23 + v33 * v33);
}

//************************************************************************************************
// TransformUtils3D
//************************************************************************************************

Transform3D TransformUtils3D::perspectiveFovLH (float fieldOfViewY, float aspectRatio, float nearClipDistance, float farClipDistance)
{
	float h = 1.0f / ::tanf (fieldOfViewY * 0.5f);
	float w = h / aspectRatio;
	float Q = farClipDistance / (farClipDistance - nearClipDistance);

	Transform3D result;
	result.v11 = w, result.v12 = 0, result.v13 = 0, result.v14 = 0;
	result.v21 = 0, result.v22 = h, result.v23 = 0, result.v24 = 0;
	result.v31 = 0, result.v32 = 0, result.v33 = Q, result.v34 = -Q * nearClipDistance;
	result.v41 = 0, result.v42 = 0, result.v43 = 1, result.v44 = 0;

	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TransformUtils3D::screenSpaceToCameraSpace (PointF& coordinates, float fieldOfViewY, float aspectRatio)
{
	float viewAngle = ::tanf (fieldOfViewY * 0.5f);
	coordinates.x = coordinates.x * aspectRatio * viewAngle;
	coordinates.y = coordinates.y * viewAngle;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Transform3D TransformUtils3D::lookAt (PointF3DRef eye, PointF3DRef at, PointF3DRef up)
{
	PointF3D forward = (at - eye).normal ();
	PointF3D right = up.cross (forward).normal ();
	PointF3D newUp = forward.cross (right);

	Transform3D result;
	result.v11 = right.x;
	result.v12 = right.y;
	result.v13 = right.z;
	result.v14 = -right.dot (eye);
	result.v21 = newUp.x;
	result.v22 = newUp.y;
	result.v23 = newUp.z;
	result.v24 = -newUp.dot (eye);
	result.v31 = forward.x;
	result.v32 = forward.y;
	result.v33 = forward.z;
	result.v34 = -forward.dot (eye);

	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Transform3D TransformUtils3D::rotateYawPitchRoll (float yaw, float pitch, float roll)
{
	return Transform3D ().rotateY (yaw).rotateX (-pitch).rotateZ (-roll);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TransformUtils3D::getYawPitchRollAngles (float& yaw, float& pitch, float& roll, Transform3DRef transform)
{
	PointF3D scale;
	transform.getScale (scale);

	if(scale.x == 0 || scale.y == 0 || scale.z == 0)
	{
		yaw = 0.f;
		pitch = 0.f;
		roll = 0.f;
		return;
	}

	Transform3D unscaled (transform);
	unscaled.scale (1.f / scale.x, 1.f / scale.y, 1.f / scale.z);

	// v11 == sin(pitch)
	pitch = ::asinf (unscaled.v23);

	if(ccl_abs (ccl_abs (pitch) - Math::Constants<float>::kHalfPi) > 0.00001f)
	{
		// v33 == cos(pitch) * cos(yaw)
		// v13 == cos(pitch) * sin(yaw)
		yaw = ::atan2f (unscaled.v13, unscaled.v33);

		// v22 == cos(pitch) * cos(roll)
		// v21 == -cos(pitch) * sin(roll)
		roll = ::atan2f (-unscaled.v21, unscaled.v22);
	}
	else
	{
		// v11 = cos(yaw + roll)
		yaw = 0.f;
		roll = ::acosf (unscaled.v11);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Transform3D TransformUtils3D::rotateAroundAxis (PointF3DRef axis, float angle)
{
	float s = ::sinf (angle);
	float c = ::cosf (angle);

	Transform3D result;
	result.v11 = axis.x * axis.x + (1.f - axis.x * axis.x) * c;
	result.v12 = axis.x * axis.y * (1.f - c) - axis.z * s;
	result.v13 = axis.x * axis.z * (1.f - c) + axis.y * s;
	result.v21 = axis.y * axis.x * (1.f - c) + axis.z * s;
	result.v22 = axis.y * axis.y + (1.f - axis.y * axis.y) * c;
	result.v23 = axis.y * axis.z * (1.f - c) - axis.x * s;
	result.v31 = axis.z * axis.x * (1.f - c) - axis.y * s;
	result.v32 = axis.z * axis.y * (1.f - c) + axis.x * s;
	result.v33 = axis.z * axis.z + (1.f - axis.z * axis.z) * c;

	return result;
}
