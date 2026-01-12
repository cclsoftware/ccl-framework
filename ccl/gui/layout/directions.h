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
// Filename    : ccl/gui/layout/directions.h
// Description : DirectionTraits template
//
//************************************************************************************************

#ifndef _ccl_directions_h
#define _ccl_directions_h

namespace CCL {

//************************************************************************************************
// DirectionTraits
/** A traits class template that abstracts from the direction (horizontal / vertical).
	Can be used to write a geometric algorithm once for both directions.
	We use the notions Coord, StartCoord, EndCoord, Length here to abstract from
	x/y, left/top, right/bottom, width/height. */
//************************************************************************************************

template<int direction> struct DirectionTraits {}; ///< direction can be Styles::kHorizontal or Styles::kVertical

//////////////////////////////////////////////////////////////////////////////////////////////////

template<> struct DirectionTraits<Styles::kHorizontal>
{
	typedef DirectionTraits<Styles::kVertical> OtherDirection;

	static inline bool isHorizontal ()							{ return true; }
	static inline bool isVertical ()							{ return false; }

	static inline Coord& getCoord (Point& p)					{ return p.x; }
	static inline Coord  getCoord (const Point& p)				{ return p.x; }
	static inline Coord& getStartCoord (Rect& r)				{ return r.left; }
	static inline Coord& getEndCoord   (Rect& r)				{ return r.right; }
	static inline Coord  getStartCoord (const Rect& r)			{ return r.left; }
	static inline Coord  getEndCoord   (const Rect& r)			{ return r.right; }
	static inline Coord  getLength (const Rect& r)				{ return r.getWidth (); }
	static inline Coord  getLength (const View* view)			{ return view->getWidth (); }

	static inline Coord& getMin (SizeLimit& limits)				{ return limits.minWidth; }
	static inline Coord& getMax (SizeLimit& limits)				{ return limits.maxWidth; }
	static inline Coord  getMin (const SizeLimit& limits)		{ return limits.minWidth; }
	static inline Coord  getMax (const SizeLimit& limits)		{ return limits.maxWidth; }

	static inline Rect& offset (Rect& r, Coord c)				{ return r.offset (c, 0); }
	static inline Rect& moveTo (Rect& r, Coord c)				{ return r.moveTo (Point (c, r.top)); }
	static inline Rect& setLength (Rect& r, Coord c)			{ return r.setWidth (c); }

	enum
	{
		kAttachStart = IView::kAttachLeft,
		kAttachEnd   = IView::kAttachRight,
		kCenter      = IView::kHCenter,
		kFitSize     = IView::kHFitSize
	};
};

//////////////////////////////////////////////////////////////////////////////////////////////////

template<> struct DirectionTraits<Styles::kVertical>
{
	typedef DirectionTraits<Styles::kHorizontal> OtherDirection;

	static inline bool isHorizontal ()							{ return false; }
	static inline bool isVertical ()							{ return true; }

	static inline Coord& getCoord (Point& p)					{ return p.y; }
	static inline Coord  getCoord (const Point& p)				{ return p.y; }
	static inline Coord& getStartCoord (Rect& r)				{ return r.top; }
	static inline Coord& getEndCoord   (Rect& r)				{ return r.bottom; }
	static inline Coord  getStartCoord (const Rect& r)			{ return r.top; }
	static inline Coord  getEndCoord   (const Rect& r)			{ return r.bottom; }
	static inline Coord  getLength (const Rect& r)				{ return r.getHeight (); }
	static inline Coord  getLength (const View* view)			{ return view->getHeight (); }

	static inline Coord& getMin (SizeLimit& limits)				{ return limits.minHeight; }
	static inline Coord& getMax (SizeLimit& limits)				{ return limits.maxHeight; }
	static inline Coord  getMin (const SizeLimit& limits)		{ return limits.minHeight; }
	static inline Coord  getMax (const SizeLimit& limits)		{ return limits.maxHeight; }

	static inline Rect& offset (Rect& r, Coord c)				{ return r.offset (0, c); }
	static inline Rect& moveTo (Rect& r, Coord c)				{ return r.moveTo (Point (r.left, c)); }
	static inline Rect& setLength (Rect& r, Coord c)			{ return r.setHeight (c); }

	enum
	{
		kAttachStart = IView::kAttachTop,
		kAttachEnd   = IView::kAttachBottom,
		kCenter      = IView::kVCenter,
		kFitSize     = IView::kVFitSize
	};
};

//////////////////////////////////////////////////////////////////////////////////////////////////

typedef DirectionTraits<Styles::kHorizontal> HorizontalDirection;
typedef DirectionTraits<Styles::kVertical>   VerticalDirection;

//////////////////////////////////////////////////////////////////////////////////////////////////

} // namespace CCL

#endif // _ccl_directions_h
