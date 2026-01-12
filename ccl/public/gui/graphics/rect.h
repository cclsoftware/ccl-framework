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
// Filename    : ccl/public/gui/graphics/rect.h
// Description : Rectangle class
//
//************************************************************************************************

#ifndef _ccl_rect_h
#define _ccl_rect_h

#include "ccl/public/gui/graphics/point.h"

#include "core/public/gui/corerect.h"

namespace CCL {

using Core::Rect;
using Core::RectRef;

using Core::RectF;
using Core::RectFRef;

using Core::TRect;

//////////////////////////////////////////////////////////////////////////////////////////////////

#if DEBUG
/** Dump coordinates to debug output. */
void dumpRect (const Rect& rect, const char* string = nullptr);
#endif

/** Convert float to integer rectangle. */
inline Rect rectFToInt (RectFRef r)
{
	return Rect (coordFToInt (r.left), coordFToInt (r.top), coordFToInt (r.right), coordFToInt (r.bottom));
}

/** Convert integer to float rectangle. */
inline RectF rectIntToF (RectRef r)
{
	return RectF ((CoordF)r.left, (CoordF)r.top, (CoordF)r.right, (CoordF)r.bottom);
}

//************************************************************************************************
// SizeLimit
/** Size limit definition. 
	\ingroup gui_graphics */
//************************************************************************************************

struct SizeLimit
{
	Coord minWidth;			///< minimum width
	Coord minHeight;		///< minimum height
	Coord maxWidth;			///< maximum width
	Coord maxHeight;		///< maximum height

	SizeLimit (Coord minW = 0, Coord minH = 0, Coord maxW = 0, Coord maxH = 0)
	: minWidth (minW),
	  minHeight (minH),
	  maxWidth (maxW),
	  maxHeight (maxH)
	{}

	SizeLimit (RectRef rect)
	: minWidth (rect.left),
	  minHeight (rect.top),
	  maxWidth (rect.right),
	  maxHeight (rect.bottom)
	{}

	bool isValid () const;
	SizeLimit& setUnlimited ();
	SizeLimit& setFixed (PointRef size);
	SizeLimit& setFixedWidth (Coord w);
	SizeLimit& setFixedHeight (Coord h);

	SizeLimit& include (const SizeLimit& limits);
	SizeLimit& resolveConflicts (); // force maximum >= minimum (prefers minimum)

	bool isAllowed (const Point& size) const;
	Point& makeValid (Point& size) const;
	Rect& makeValid (Rect& rect) const;

	bool operator == (const SizeLimit& limits) const;

	operator Rect () const;
};

//////////////////////////////////////////////////////////////////////////////////////////////////
// SizeLimit inline
//////////////////////////////////////////////////////////////////////////////////////////////////

inline bool SizeLimit::isValid () const
{ return ((Rect)*this) != Rect (); }

inline SizeLimit& SizeLimit::setUnlimited ()
{ minWidth = minHeight = 0;  maxWidth = maxHeight = kMaxCoord; return *this; }

inline SizeLimit&  SizeLimit::setFixed (PointRef size)
{ minWidth = maxWidth = size.x;  minHeight = maxHeight = size.y; return *this; }

inline SizeLimit&  SizeLimit::setFixedWidth (Coord w)
{ minWidth = maxWidth = w; return *this; }

inline SizeLimit&  SizeLimit::setFixedHeight (Coord h)
{ minHeight = maxHeight = h; return *this; }

inline bool SizeLimit::isAllowed (const Point& s) const
{ return s.x >= minWidth && s.x <= maxWidth && s.y >= minHeight && s.y <= maxHeight; }

inline bool SizeLimit::operator == (const SizeLimit& l) const
{ return minWidth == l.minWidth && minHeight == l.minHeight && maxWidth == l.maxWidth && maxHeight == l.maxHeight; }

inline SizeLimit::operator Rect () const
{ return Rect (minWidth, minHeight, maxWidth, maxHeight); }

//////////////////////////////////////////////////////////////////////////////////////////////////

} // namespace CCL

#endif // _ccl_rect_h
