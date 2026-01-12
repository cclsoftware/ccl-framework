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
// Filename    : ccl/public/gui/graphics/3d/transform3d.h
// Description : 3D Transformation Matrix
//
//************************************************************************************************

#ifndef _ccl_transform3d_h
#define _ccl_transform3d_h

#include "ccl/public/gui/graphics/3d/point3d.h"

namespace CCL {

struct Transform3D;

/** 3D Transformation Matrix reference. */
typedef const Transform3D& Transform3DRef;

//************************************************************************************************
// PlainTransform3D
/** The 3D transform class below is binary equivalent to this C structure. 
	\ingroup gui_graphics3d */
//************************************************************************************************

struct PlainTransform3D
{
	union
	{
		struct
		{
			float v11, v12, v13, v14;
			float v21, v22, v23, v24;
			float v31, v32, v33, v34;
			float v41, v42, v43, v44;
		};
		float v[4][4];
	};
};

//************************************************************************************************
// Transform3D
/** 3D Transformation Matrix
	\ingroup gui_graphics3d */
//************************************************************************************************

struct Transform3D: PlainTransform3D
{
	/** Construct with identity matrix. */
	Transform3D ();

	/** Assign matrix. */
	Transform3D& operator = (Transform3DRef t);
	
	/** Comparison operator */
	bool operator == (Transform3DRef other) const;

	/** Comparison operator */
	bool operator != (Transform3DRef other) const;
	
	/** Check if this is an identity matrix. */
	bool isIdentity () const;

	/** Transpose matrix */
	Transform3D& transpose ();

	/** Get inverse matrix */
	Transform3D getInverseTransform () const;

	/** Add translation. */
	Transform3D& translate (float tx, float ty, float tz);
	Transform3D& translate (PointF3DRef p);

	/** Add scaling (relative to origin). */
	Transform3D& scale (float sx, float sy, float sz);

	/** Add rotation around a coordinate axis. */
	Transform3D& rotateX (float angle);
	Transform3D& rotateY (float angle);
	Transform3D& rotateZ (float angle);

	/** Multiply with other matrix */
	Transform3D operator * (Transform3DRef rhs) const;
	Transform3D& operator *= (Transform3DRef& t);
	Transform3D& rightMultiply (Transform3DRef t);
	Transform3D& leftMultiply (Transform3DRef t);

	/** Reset translation component. */
	void resetTranslation ();

	/** Reset rotation component. */
	void resetRotation ();

	/** Reset scale component. */
	void resetScale ();

	/** Transform point. */
	PointF3D& transform (PointF3D& p) const;

	/** Get translation component. */
	void getTranslation (PointF3D& translation) const;

	/** Get rotation component. */
	void getRotation (PointF3D& rotation) const;

	/** Get scale component. */
	void getScale (PointF3D& scale) const;
};

//////////////////////////////////////////////////////////////////////////////////////////////////
// Right or left multiply point with matrix
//////////////////////////////////////////////////////////////////////////////////////////////////

PointF3D operator * (PointF3DRef p, Transform3DRef& t);
PointF3D operator * (Transform3DRef& t, PointF3DRef p);

//************************************************************************************************
// TransformUtils3D
/** 3D Transformation Utilities
	\ingroup gui_graphics3d */
//************************************************************************************************

namespace TransformUtils3D
{
	/** Create a left-handed perspective projection matrix based on a field of view. */
	Transform3D perspectiveFovLH (float fovY, float aspect, float nearClipDistance, float farClipDistance);

	/** Transform coordinates in screen space (range -1 to 1) to coordinates in camera space. */
	void screenSpaceToCameraSpace (PointF& coordinates, float fovY, float aspect);

	/** Create a perspective transform of a camera looking at the specified point */
	Transform3D lookAt (PointF3DRef eye, PointF3DRef at, PointF3DRef up);

	/** Create a rotation transform using yaw, pitch, roll coordinate axes. */
	Transform3D rotateYawPitchRoll (float yaw, float pitch, float roll);

	/** Extract yaw, pitch, roll angles form a transform matrix. */
	void getYawPitchRollAngles (float& yaw, float& pitch, float& roll, Transform3DRef transform);

	/** Create a rotation transform using a fixed axis and an angle around this axis (radians). */
	Transform3D rotateAroundAxis (PointF3DRef axis, float angle);
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// Transform3D inline
//////////////////////////////////////////////////////////////////////////////////////////////////

inline Transform3D::Transform3D ()
{
	v11 = 1; v12 = 0; v13 = 0; v14 = 0;
	v21 = 0; v22 = 1; v23 = 0; v24 = 0;
	v31 = 0; v32 = 0; v33 = 1; v34 = 0;
	v41 = 0; v42 = 0; v43 = 0; v44 = 1;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline Transform3D& Transform3D::operator = (Transform3DRef t)
{
	v11 = t.v11; v12 = t.v12; v13 = t.v13; v14 = t.v14;
	v21 = t.v21; v22 = t.v22; v23 = t.v23; v24 = t.v24;
	v31 = t.v31; v32 = t.v32; v33 = t.v33; v34 = t.v34;
	v41 = t.v41; v42 = t.v42; v43 = t.v43; v44 = t.v44;
	return *this;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline bool Transform3D::operator == (Transform3DRef t) const
{
	return v11 == t.v11 && v12 == t.v12 && v13 == t.v13 && v14 == t.v14
		&& v21 == t.v21 && v22 == t.v22 && v23 == t.v23 && v24 == t.v24
		&& v31 == t.v31 && v32 == t.v32 && v33 == t.v33 && v34 == t.v34
		&& v41 == t.v41 && v42 == t.v42 && v43 == t.v43 && v44 == t.v44;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline bool Transform3D::operator != (Transform3DRef other) const
{
	return !(*this == other);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline bool Transform3D::isIdentity () const
{
	return v11 == 1 && v12 == 0 && v13 == 0 && v14 == 0
		&& v21 == 0 && v22 == 1 && v23 == 0 && v24 == 0
		&& v31 == 0 && v32 == 0 && v33 == 1 && v34 == 0
		&& v41 == 0 && v42 == 0 && v43 == 0 && v44 == 1;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline Transform3D& Transform3D::transpose ()
{
	float t = 0;
	t = v12, v12 = v21, v21 = t;
	t = v13, v13 = v31, v31 = t;
	t = v14, v14 = v41, v41 = t;
	t = v23, v23 = v32, v32 = t;
	t = v24, v24 = v42, v42 = t;
	t = v34, v34 = v43, v43 = t;
	return *this;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline Transform3D& Transform3D::operator *= (Transform3DRef& rhs)
{
	*this = *this * rhs;
	return *this;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline Transform3D& Transform3D::rightMultiply (Transform3DRef rhs)
{
	*this = *this * rhs;
	return *this;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline Transform3D& Transform3D::leftMultiply (Transform3DRef lhs)
{
	*this = lhs * *this;
	return *this;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline Transform3D& Transform3D::translate (PointF3DRef p)
{
	return translate (p.x, p.y, p.z);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline PointF3D& Transform3D::transform (PointF3D& p) const
{
	p = *this * p;
	return p;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// operator * inline
//////////////////////////////////////////////////////////////////////////////////////////////////

inline PointF3D operator * (PointF3DRef p, Transform3DRef& t)
{
//	              | v11 v12 v13 v14 |
//	| x y z 1 | x | v21 v22 v23 v24 | = | x*v11 + y*v21 + z*v31 + v41,  x*v12 + y*v22 + z*v32 + v42,  x*v13 + y*v23 + z*v33 + v43,  x*v14 + y*v24 + z*v34 + v44 |
//	              | v31 v32 v33 v34 |
//	              | v41 v42 v43 v44 |
	PointF3D result;
	result.x = p.x * t.v11 + p.y * t.v21 + p.z * t.v31 * p.z + t.v41;
	result.y = p.x * t.v12 + p.y * t.v22 + p.z * t.v32 * p.z + t.v42;
	result.z = p.x * t.v13 + p.y * t.v23 + p.z * t.v33 * p.z + t.v43;
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline PointF3D operator * (Transform3DRef& t, PointF3DRef p)
{
//	| v11 v12 v13 v14 |   | x |   | v11*x + v12*y + v13*z + v14 |
//	| v21 v22 v23 v24 | x | y | = | v21*x + v22*y + v23*z + v24 |
//	| v31 v32 v33 v34 |   | z |   | v31*x + v32*y + v33*z + v34 |
//	| v41 v42 v43 v44 |   | 1 |   | v41*x + v42*y + v43*z + v44 |
	PointF3D result;
	result.x = t.v11 * p.x + t.v12 * p.y + t.v13 * p.z + t.v14;
	result.y = t.v21 * p.x + t.v22 * p.y + t.v23 * p.z + t.v24;
	result.z = t.v31 * p.x + t.v32 * p.y + t.v33 * p.z + t.v34;
	return result;
}

} // namespace CCL

#endif // _ccl_transform3d_h
