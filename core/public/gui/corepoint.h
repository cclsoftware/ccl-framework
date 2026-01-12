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
// Filename    : core/public/gui/corepoint.h
// Description : Point class
//
//************************************************************************************************

#ifndef _corepoint_h
#define _corepoint_h

#include "core/public/coretypes.h"

#include "core/meta/generated/cpp/coregui-constants-generated.h"

namespace Core {

//************************************************************************************************
// Coordinate
//************************************************************************************************

/** Integer coordinate.*/
typedef int Coord;

/** Float coordinate. */
typedef float CoordF;

/** Coordinate limits. */
enum CoordLimits
{
	kMaxCoord = kCoordLimitsMaxCoord,
	kMinCoord = kCoordLimitsMinCoord
};

//************************************************************************************************
// TPoint
/** 2D point template class.
	\ingroup core_gui */
//************************************************************************************************

template <typename Type>
struct TPoint
{
	Type x;	///< horizontal coordinate
	Type y;	///< vertical coordinate

	/** Construct from x/y coordinate. */
	TPoint (Type x, Type y)
	: x (x),
	  y (y)
	{}

	/** Construct with (0,0). */
	TPoint ()
	: x (0),
	  y (0)
	{}

	/** Offset point. */
	TPoint& offset (Type dx, Type dy);

	/** Offset point. */
	TPoint& offset (const TPoint& p);

	/** Set point with x/y. */
	TPoint& operator () (Type x, Type y);

	/** Scale point. */
	TPoint& operator *= (float factor);
	
	/** Scale point. */
	TPoint operator * (float factor) const;

	/** Add two points. */
	TPoint operator + (const TPoint& p) const;

	/** Subtract two points. */
	TPoint operator - (const TPoint& p) const;

	/** Add another point. */
	TPoint& operator += (const TPoint& p);

	/** Subtract another point. */
	TPoint& operator -= (const TPoint& p);

	/** Compare point. */
	bool operator == (const TPoint& p) const;
	
	/** Compare point. */
	bool operator != (const TPoint& p) const;

	/** Check if point is (0,0). */
	bool isNull () const;
};

/** Point with integer coordinates. */
typedef TPoint<Coord> Point;
typedef const Point& PointRef;

/** Point with float coordinates. */
typedef TPoint<CoordF> PointF;
typedef const PointF& PointFRef;

//////////////////////////////////////////////////////////////////////////////////////////////////
// TPoint inline
//////////////////////////////////////////////////////////////////////////////////////////////////

template <typename Type>
inline TPoint<Type>& TPoint<Type>::offset (Type dx, Type dy)
{ x += dx; y += dy; return *this; }

template <typename Type>
inline TPoint<Type>& TPoint<Type>::offset (const TPoint& p)
{ x += p.x; y += p.y; return *this; }

template <typename Type>
inline TPoint<Type>& TPoint<Type>::operator () (Type _x, Type _y)
{ x = _x; y = _y; return *this; }

template <typename Type>
inline TPoint<Type> TPoint<Type>::operator * (float f) const
{ return TPoint (Type (x * f), Type (y * f)); }

template <typename Type>
inline TPoint<Type>& TPoint<Type>::operator *= (float f)
{ x = Type (x * f); y = Type (y * f); return *this; }

template <typename Type>
inline TPoint<Type>& TPoint<Type>::operator += (const TPoint& p)
{ x += p.x; y += p.y; return *this; }

template <typename Type>
inline TPoint<Type>& TPoint<Type>::operator -= (const TPoint& p)
{ x -= p.x; y -= p.y; return *this; }

template <typename Type>
inline TPoint<Type> TPoint<Type>::operator + (const TPoint& p) const
{ return TPoint (x + p.x, y + p.y); }

template <typename Type>
inline TPoint<Type> TPoint<Type>::operator - (const TPoint& p) const
{ return TPoint (x - p.x, y - p.y); }

template <typename Type>
inline bool TPoint<Type>::operator == (const TPoint& p) const
{ return x == p.x && y == p.y; }

template <typename Type>
inline bool TPoint<Type>::operator != (const TPoint& p) const	
{ return x != p.x || y != p.y; }

template <typename Type>
inline bool TPoint<Type>::isNull () const	
{ return x == 0 && y == 0; }

//////////////////////////////////////////////////////////////////////////////////////////////////

} // namespace Core

#endif // _corepoint_h
