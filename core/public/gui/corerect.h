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
// Filename    : core/public/gui/corerect.h
// Description : Rectangle class
//
//************************************************************************************************

#ifndef _corerect_h
#define _corerect_h

#include "core/public/gui/corepoint.h"
#include "core/public/gui/corealignment.h"

namespace Core {

//************************************************************************************************
// TRect
/** Rectangle template class.
	\ingroup core_gui */
//************************************************************************************************

template <typename Type>
struct TRect
{
	Type left;		///< left coordinate
	Type top;		///< top coordinate
	Type right;		///< right coordinate
	Type bottom;	///< bottom coordinate

	/** Construct rectangle from coordinates. */
	TRect (Type l = 0, Type t = 0, Type r = 0, Type b = 0)
	: left (l), top (t), right (r), bottom (b)
	{}

	/** Construct rectangle from points (left/top, right/bottom). */
	TRect (const TPoint<Type>& p1, const TPoint<Type>& p2)
	: left (p1.x), top (p1.y), right (p2.x), bottom (p2.y)
	{}

	/** Construct rectangle from left/top and size. */
	TRect (Type l, Type t, const TPoint<Type>& size)
	: left (l), top (t), right (l + size.x), bottom (t + size.y)
	{}

	/** Construct rectangle at (0,0) with given size. */
	TRect (const TPoint<Type>& size)
	: left (0), top (0), right (size.x), bottom (size.y)
	{}

	/** Assign new coordinates. */
	TRect& operator () (Type l, Type t, Type r, Type b);

	/** Get width. */
	Type getWidth () const;

	/** Get height. */
	Type getHeight () const;

	/** Set width without moving. */
	TRect& setWidth (Type w);
	
	/** Set height without moving. */
	TRect& setHeight (Type w);

	/** Get size of rectangle as a point. */
	TPoint<Type> getSize () const;

	/** Set width and height without moving. */
	TRect& setSize (const TPoint<Type>& size);

	/** Get left/top position. */
	TPoint<Type> getLeftTop () const;

	/** Get right/top position. */
	TPoint<Type> getRightTop () const;
	
	/** Get left/bottom position. */
	TPoint<Type> getLeftBottom () const;
	
	/** Get right/bottom position. */
	TPoint<Type> getRightBottom () const;
	
	/** Get center point of rectangle. */
	TPoint<Type> getCenter () const;

	/** Offset by given delta. */
	TRect& offset (Type dx, Type dy = 0);

	/** Offset by point. */
	TRect& offset (const TPoint<Type>& p);
	
	/** Move origin to new position. */
	TRect& moveTo (const TPoint<Type>& p);
	
	/** Shrink all edges by given value. */
	TRect& contract (Type v);
	
	/** Expand all edges by given value. */
	TRect& expand (Type v);
	
	/** Multiply all edges by given value. */
	TRect& zoom (float factor);

	/** Bound to given rectangle (can cause shrink). Returns false, if result is empty. */
	bool bound (const TRect& r);
	
	/** Bound to given rectangle horizontally (can cause shrink). Returns false, if result is empty. */
	bool boundH (const TRect& r);

	/** Bound to given rectangle vertically (can cause shrink). Returns false, if result is empty. */
	bool boundV (const TRect& r);

	/** Join given rectangle (can cause expansion). */
	TRect& join (const TRect& r);
	
	/** Join with point (can cause expansion). */
	TRect& join (const TPoint<Type>& p);
	
	/** Center in given parent rectangle. */
	TRect& center (const TRect& r);
	
	/** Center horizontally in given parent rectangle. */
	TRect& centerH (const TRect& r);
	
	/** Center vertically in given parent rectangle. */
	TRect& centerV (const TRect& r);

	/** Align in given parent rectangle. */
	TRect& align (const TRect& r, const Alignment& alignment);

	/** Scale proportionally to fit into parent rectangle. (There may be space left in one direction.) */
	TRect& fitProportionally (const TRect& r);

	/** Swap corners if width/height is negative. */
	TRect& normalize ();
	
	/** Set coordinates to (0,0,0,0). */
	TRect& setEmpty ();
	
	/** Set empty using +- kMaxCoord. Use when joining rectangles. */
	TRect& setReallyEmpty ();
	
	/** Check if rectangle is empty. */
	bool isEmpty () const;
	
	/** Check if this rectangle intersects with other rectangle. */
	bool intersect (const TRect& r) const;
	
	/** Check if point is inside. */
	bool pointInside (const TPoint<Type>& p) const;
	
	/** Check if rectangle is inside. */
	bool rectInside (const TRect& r) const;

	/** Compare rectangles. */
	bool operator == (const TRect& r) const;

	/** Compare rectangles. */
	bool operator != (const TRect& r) const;
};

/** Rectangle with integer coordinates. */
typedef TRect<Coord> Rect;
typedef const Rect& RectRef;

/** Rectangle with float coordinates. */
typedef TRect<CoordF> RectF;
typedef const RectF& RectFRef;

//////////////////////////////////////////////////////////////////////////////////////////////////
// TRect inline
//////////////////////////////////////////////////////////////////////////////////////////////////

template <typename Type>
inline Type TRect<Type>::getWidth () const
{ return right - left; }

template <typename Type>
inline Type TRect<Type>::getHeight () const
{ return bottom - top; }

template <typename Type>
inline TRect<Type>& TRect<Type>::setWidth (Type w)
{ right = left + w; return *this; }

template <typename Type>
inline TRect<Type>& TRect<Type>::setHeight (Type h)
{ bottom = top + h; return *this; }

template <typename Type>
inline TPoint<Type> TRect<Type>::getSize () const
{ return TPoint<Type> (getWidth (), getHeight ()); }

template <typename Type>
inline TRect<Type>& TRect<Type>::setSize (const TPoint<Type>& size)
{ right = left + size.x; bottom = top + size.y; return *this; }

template <typename Type>
inline TPoint<Type> TRect<Type>::getLeftTop () const
{ return TPoint<Type> (left, top); }

template <typename Type>
inline TPoint<Type> TRect<Type>::getRightTop () const
{ return TPoint<Type> (right, top); }

template <typename Type>
inline TPoint<Type> TRect<Type>::getLeftBottom () const
{ return TPoint<Type> (left, bottom); }

template <typename Type>
inline TPoint<Type> TRect<Type>::getRightBottom () const
{ return TPoint<Type> (right, bottom); }

template <typename Type>
inline TPoint<Type> TRect<Type>::getCenter () const
{ return TPoint<Type> (left + getWidth ()/2, top + getHeight ()/2); }

template <typename Type>
inline TRect<Type>& TRect<Type>::offset (Type dx, Type dy)
{ left += dx; top += dy; right += dx; bottom += dy; return *this; }

template <typename Type>
inline TRect<Type>& TRect<Type>::offset (const TPoint<Type>& p)
{ offset (p.x, p.y); return *this; }

template <typename Type>
inline TRect<Type>& TRect<Type>::contract (Type v)
{ left += v; top += v; right -= v; bottom -= v; return *this; }

template <typename Type>
inline TRect<Type>& TRect<Type>::expand (Type v)
{ return contract (-v); }

template <typename Type>
inline TRect<Type>& TRect<Type>::zoom (float factor)
{ left *= factor; top *= factor; right *= factor; bottom *= factor; return *this; }

template <typename Type>
inline TRect<Type>& TRect<Type>::join (const TPoint<Type>& p)
{ return join (TRect (p.x, p.y, p.x, p.y)); }

template <typename Type>
inline TRect<Type>& TRect<Type>::center (const TRect& r)
{ centerH (r); centerV (r); return *this; }

template <typename Type>
inline bool TRect<Type>::intersect (const TRect& r) const
{ TRect t (*this); return t.bound (r); }

template <typename Type>
inline bool TRect<Type>::pointInside (const TPoint<Type>& p) const
{ return p.x >= left && p.x < right && p.y >= top && p.y < bottom; }

template <typename Type>
inline bool TRect<Type>::rectInside (const TRect& r) const
{ return pointInside (r.getLeftTop ()) && pointInside (r.getRightBottom ()); }

template <typename Type>
inline TRect<Type>& TRect<Type>::setEmpty ()
{ left = top = right = bottom = 0; return *this; }

template <typename Type>
inline TRect<Type>& TRect<Type>::setReallyEmpty ()
{ return operator () (Type(kMaxCoord), Type(kMaxCoord), Type(kMinCoord), Type(kMinCoord)); }

template <typename Type>
inline bool TRect<Type>::isEmpty () const
{ return right <= left || bottom <= top; }

template <typename Type>
inline TRect<Type>& TRect<Type>::operator () (Type l, Type t, Type r, Type b)
{ left = l; top = t; right = r; bottom = b; return *this; }

template <typename Type>
inline bool TRect<Type>::operator == (const TRect& r) const
{ return left == r.left && top == r.top && right == r.right && bottom == r.bottom; }

template <typename Type>
inline bool TRect<Type>::operator != (const TRect& r) const
{ return !(*this == r); }

//////////////////////////////////////////////////////////////////////////////////////////////////

template <typename Type>
TRect<Type>& TRect<Type>::moveTo (const TPoint<Type>& p)
{
	Type w = getWidth ();
	Type h = getHeight ();

	left   = p.x;
	right  = p.x + w;
	top    = p.y;
	bottom = p.y + h;
	return *this;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template <typename Type>
bool TRect<Type>::boundH (const TRect& rect)
{
	if(left < rect.left)
		left = rect.left;
	if(right > rect.right)
		right = rect.right;
	if(right <= left)
		return false;
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template <typename Type>
bool TRect<Type>::boundV (const TRect& rect)
{
	if(top < rect.top)
		top = rect.top;
	if(bottom > rect.bottom)
		bottom = rect.bottom;
	if(bottom <= top)
		return false;
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template <typename Type>
bool TRect<Type>::bound (const TRect& rect)
{
	return boundH (rect) && boundV (rect);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template <typename Type>
TRect<Type>& TRect<Type>::join (const TRect& r)
{
	if(r.left < left)
		left = r.left;
	if(r.top < top)
		top = r.top;
	if(r.right > right)
		right = r.right;
	if(r.bottom > bottom)
		bottom = r.bottom;
	return *this;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template <typename Type>
TRect<Type>& TRect<Type>::centerH (const TRect& r)
{
	Type w = getWidth ();
	left  = r.left + r.getWidth ()/2 - w/2;
	right = left + w;
	return *this;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template <typename Type>
TRect<Type>& TRect<Type>::centerV (const TRect& r)
{
	Type h = getHeight ();
	top    = r.top  + r.getHeight ()/2 - h/2;
	bottom = top + h;
	return *this;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template <typename Type>
TRect<Type>& TRect<Type>::align (const TRect& r, const Alignment& alignment)
{
	switch(alignment.getAlignH ())
	{
	case Alignment::kHCenter :
		centerH (r);
		break;
	case Alignment::kLeft :
		{
			Type w = getWidth ();
			left  = r.left;
			right = r.left + w;
		}
		break;
	case Alignment::kRight :
		{
			Type w = getWidth ();
			left  = r.right - w;
			right = r.right;
		}
		break;
	}

	switch(alignment.getAlignV ())
	{
	case Alignment::kVCenter :
		centerV (r);
		break;
	case Alignment::kTop :
		{
			Type h = getHeight ();
			top    = r.top;
			bottom = r.top + h;
		}
		break;
	case Alignment::kBottom :
		{
			Type h = getHeight ();
			top    = r.bottom - h;
			bottom = r.bottom;
		}
		break;
	}
	return *this;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template <typename Type>
TRect<Type>& TRect<Type>::fitProportionally (const TRect& r)
{
	Type width  = getWidth ();
	Type height = getHeight ();
	Type destWidth  = r.getWidth ();
	Type destHeight = r.getHeight ();
	if(width == 0)
		width = destWidth;
	if(height == 0)
		height = destHeight;

	left = r.left;
	top  = r.top;

	// try to adapt the dest width
	Type h = destWidth * height / width;
	if (h <= destHeight)
	{
		setWidth (destWidth);
		setHeight (h);
	}
	else // too high, adapt the dest height
	{
		setWidth  (destHeight * width / height);
		setHeight (destHeight);
	}
	return *this;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template <typename Type>
TRect<Type>& TRect<Type>::normalize ()
{
	if(left > right)
	{
		Type t = left;
		left = right;
		right = t;
	}
	if(top > bottom)
	{
		Type t = top;
		top = bottom;
		bottom = t;
	}
	return *this;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

} // namespace Core

#endif // _corerect_h
