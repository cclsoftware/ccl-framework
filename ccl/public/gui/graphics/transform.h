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
// Filename    : ccl/public/gui/graphics/transform.h
// Description : 2D Transformation Matrix
//
//************************************************************************************************

#ifndef _ccl_transform_h
#define _ccl_transform_h

#include "ccl/public/gui/graphics/rect.h"

namespace CCL {

struct Transform;

/** 2D Transformation Matrix reference. */
typedef const Transform& TransformRef;

//************************************************************************************************
// Transform
/** 2D Transformation Matrix
	\ingroup gui_graphics */
//************************************************************************************************

struct Transform
{
	float a0, a1, b0, b1, t0, t1; // the 6 variables in the 3x3 matrix
	/*	| a0 b0 t0 |
		| a1 b1 t1 |
		|  0  0  1 |  */

	/** Construct with identity matrix. */
	Transform ();

	/** Construct matrix. */
	Transform (float a0, float a1, float b0, float b1, float t0, float t1);

	/** Assign matrix. */
	Transform& operator () (float a0, float a1, float b0, float b1, float t0, float t1);

	/** Assign matrix. */
	Transform& operator = (const Transform& t);
	
	/** Comparison operator */
	bool operator == (TransformRef other) const;

	/** Comparison operator */
	bool operator != (TransformRef other) const;
	
	/** Check if this is an identity matrix. */
	bool isIdentity () const;

	/** Translate matrix. */
	Transform& translate (float tx, float ty);

	/** Scale matrix. */
	Transform& scale (float sx, float sy);

	/** Rotate matrix. */
	Transform& rotate (float angle);

	/** Skew matrix horizontally. */
	Transform& skewX (float angle);

	/** Skew matrix vertically. */
	Transform& skewY (float angle);

	/** Multiply with other matrix */
	Transform& multiply (const Transform& t);

	/** Transform point. */
	template<class TCoord>
	Core::TPoint<TCoord>& transform (Core::TPoint<TCoord>& p) const;

	/** Transform rectangle (bounding box of resulting parallelogram). */
	template<class TCoord>
	Core::TRect<TCoord>& transform (Core::TRect<TCoord>& r) const;
};

//////////////////////////////////////////////////////////////////////////////////////////////////
// Transform inline
//////////////////////////////////////////////////////////////////////////////////////////////////

inline Transform::Transform ()
: a0 (1), a1 (0), b0 (0), b1 (1), t0 (0), t1 (0)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline Transform::Transform (float a0, float a1, float b0, float b1, float t0, float t1)
: a0 (a0), a1 (a1), b0 (b0), b1 (b1), t0 (t0), t1 (t1)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline Transform& Transform::operator () (float _a0, float _a1, float _b0, float _b1, float _t0, float _t1)
{
	a0 = _a0;
	a1 = _a1;
	b0 = _b0;
	b1 = _b1;
	t0 = _t0;
	t1 = _t1;
	return *this;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline Transform& Transform::operator = (const Transform& t)
{
	a0 = t.a0;
	a1 = t.a1;
	b0 = t.b0;
	b1 = t.b1;
	t0 = t.t0;
	t1 = t.t1;
	return *this;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline bool Transform::operator == (TransformRef other) const
{ 
	return a0 == other.a0 
		&& a1 == other.a1 
		&& b0 == other.b0 
		&& b1 == other.b1 
		&& t0 == other.t0 
		&& t1 == other.t1; 
}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline bool Transform::operator != (TransformRef other) const
{ 
	return !(*this == other);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline bool Transform::isIdentity () const
{ return a0 == 1 && a1 == 0 && b0 == 0 && b1 == 1 && t0 == 0 && t1 == 0; }

//////////////////////////////////////////////////////////////////////////////////////////////////

template<class TCoord>
inline TPoint<TCoord>& Transform::transform (TPoint<TCoord>& p) const
{
	TCoord x = p.x;
	p.x = TCoord (x * a0 + p.y * b0 + t0);
	p.y = TCoord (x * a1 + p.y * b1 + t1);
	return p;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<class TCoord>
inline TRect<TCoord>& Transform::transform (TRect<TCoord>& r) const
{
	// transform all 4 points
	TPoint<TCoord> p1 (r.getLeftTop ());
	TPoint<TCoord> p2 (r.getLeftBottom ());
	TPoint<TCoord> p3 (r.getRightTop ());
	TPoint<TCoord> p4 (r.getRightBottom ());

	transform (p1);
	transform (p2);
	transform (p3);
	transform (p4);

	// bounding box of resulting parallelogram
	r.left   = ccl_min (ccl_min (ccl_min (p1.x, p2.x), p3.x), p4.x);
	r.right  = ccl_max (ccl_max (ccl_max (p1.x, p2.x), p3.x), p4.x);
	r.top    = ccl_min (ccl_min (ccl_min (p1.y, p2.y), p3.y), p4.y);
	r.bottom = ccl_max (ccl_max (ccl_max (p1.y, p2.y), p3.y), p4.y);
	return r;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

} // namespace CCL

#endif // _ccl_transform_h
