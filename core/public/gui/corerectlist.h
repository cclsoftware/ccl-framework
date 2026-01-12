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
// Filename    : core/public/gui/corerectlist.h
// Description : Rect List
//
//************************************************************************************************

#ifndef _corerectlist_h
#define _corerectlist_h

#include "core/public/corevector.h"
#include "core/public/gui/corerect.h"

namespace Core {

//////////////////////////////////////////////////////////////////////////////////////////////////
// ForEachRectFast
//////////////////////////////////////////////////////////////////////////////////////////////////

/** Iterate through rectangles in list (RectList). */
#define ForEachRectFast(rectList, T, var) \
{ T* __p = (rectList).getRects ().getItems (); for(int __iter = (rectList).getRects ().count (); __iter != 0; __iter--) { \
    T& var = *__p++;

//************************************************************************************************
// RectList
/**	List of non-intersecting rectangles, e.g. used as dirty region.
	\ingroup core_gui */
//************************************************************************************************

template<int maxRects = 5, typename T = Coord>
class RectList
{
public:
	/** Join (add) given rectangle. */
	void join (const TRect<T>& rect);
	
	/** Exclude (remove) given rectangle. */	
	void exclude (const TRect<T>& rect);
	
	/** Exclude (remove) list of rectangles. */
	void exclude (const RectList& rectList);

	/** Copy from other list of rectangles. */
	void copyFrom (const RectList& other);
	
	/** Empty list. */
	void setEmpty ();

	/** Check if list is empty. */
	bool isEmpty () const;
	
	/** Check if list is equal to other list. */
	bool isEqual (const RectList& other) const;
	
	/** Get bounding box of all rectangles. */
	TRect<T> getBoundingBox () const;
	
	/** Read-only access to rectangles in list. */
	const ConstVector<TRect<T> >& getRects () const;

	/** Check if list consists of exactly this rect. */
	bool operator == (const TRect<T>& rect) const;

	RangeIterator<ConstVector<TRect<T> >, VectorIterator<TRect<T> >, TRect<T>&> begin () const;
	RangeIterator<ConstVector<TRect<T> >, VectorIterator<TRect<T> >, TRect<T>&> end () const;

protected:
	FixedSizeVector<TRect<T>, maxRects> rects;

	void joinInternal (int index, const TRect<T>& newRect);
};

//////////////////////////////////////////////////////////////////////////////////////////////////
// RectList inline
//////////////////////////////////////////////////////////////////////////////////////////////////

template<int maxRects, typename T>
inline RangeIterator<ConstVector<TRect<T> >, VectorIterator<TRect<T> >, TRect<T>&> RectList<maxRects, T>::begin () const
{
	return rects.begin ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<int maxRects, typename T>
inline RangeIterator<ConstVector<TRect<T> >, VectorIterator<TRect<T> >, TRect<T>&> RectList<maxRects, T>::end () const
{
	return rects.end ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<int maxRects, typename T>
inline void RectList<maxRects, T>::join (const TRect<T>& newRect)
{
	// check if new rect intersects with any existing
	VectorForEach (rects, TRect<T>&, r)
		if(newRect.intersect (r))
		{
			// merge newRect into existing rect (r)
			joinInternal (__iter, newRect);
			return;
		}
	EndFor

	if(rects.count () < maxRects)
		rects.add (newRect);
	else
		// capacity exceeded: merge with first (todo: could try to find the "best" merge candidate)
		joinInternal (0, newRect);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<int maxRects, typename T>
inline void RectList<maxRects, T>::exclude (const TRect<T>& exRect)
{
	// look for an exact match first
	VectorForEach (rects, TRect<T>&, r)
		if(r == exRect)
		{
			rects.removeAt (__iter);
			return;
		}
	EndFor

	// exclude affects our rects that intersect with exRect
	VectorForEach (rects, TRect<T>&, rect)
		TRect<T> intersection (rect);
		if(intersection.bound (exRect))
		{
			// remove our rect, unaffected parts of rect will be re-added below
			TRect<T> r (rect); // copy before removing
			rects.removeAt (__iter);
			
			// also exclude the remaining parts of exRect (outside of intersection):
			if(exRect.top < intersection.top)
				exclude (TRect<T> (exRect.left, exRect.top, exRect.right, intersection.top));

			if(exRect.bottom > intersection.bottom)
				exclude (TRect<T> (exRect.left, intersection.bottom, exRect.right, exRect.bottom));

			if(exRect.left < intersection.left)
				exclude (TRect<T> (exRect.left, intersection.top, intersection.left, intersection.bottom));

			if(exRect.right > intersection.right)
				exclude (TRect<T> (intersection.right, intersection.top, exRect.right, intersection.bottom));

			// re-add remaining parts of r (outside of intersection, not to be excluded):
			if(r.top < intersection.top)
				join (TRect<T> (r.left, r.top, r.right, intersection.top));

			if(r.bottom > intersection.bottom)
				join (TRect<T> (r.left, intersection.bottom, r.right, r.bottom));

			if(r.left < intersection.left)
				join (TRect<T> (r.left, intersection.top, intersection.left, intersection.bottom));

			if(r.right > intersection.right)
				join (TRect<T> (intersection.right, intersection.top, r.right, intersection.bottom));

			return; // we are done, intersections with other rects are handled in the recursions
		}
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<int maxRects, typename T>
inline void RectList<maxRects, T>::exclude (const RectList& rectList)
{
	VectorForEach (rectList.getRects (), TRect<T>&, r)
		exclude (r);
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<int maxRects, typename T>
inline void RectList<maxRects, T>::copyFrom (const RectList& other)
{
	rects.setCount (other.rects.count ());
	for(int i = 0; i < rects.count (); i++)
		rects[i] = other.rects[i];
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<int maxRects, typename T>
inline void RectList<maxRects, T>::joinInternal (int index, const TRect<T>& newRect)
{
	TRect<T>* writeRect = &rects.at (index);
	ASSERT (writeRect)
	if(!writeRect)
		return;

	writeRect->join (newRect);

	// other existing rects might now intersect with the enlarged writeRect: join them as well into writeRect
	for(int i = 0, count = rects.count (); i < count; i++)
	{
		if(i != index)
		{
			TRect<T>& r = rects.at (i);
			if(writeRect->intersect (r))
			{
				writeRect->join (r);

				// remove r
				rects.removeAt (i);
				count--;
				if(i < index)
				{
					// writeRect shifted down in memory
					index--;
					writeRect = &rects.at (index);
				}
					
				i = -1; // writeRect enlarged, must check all remaining again
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<int maxRects, typename T>
inline void RectList<maxRects, T>::setEmpty ()
{
	rects.removeAll ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<int maxRects, typename T>
inline bool RectList<maxRects, T>::isEmpty () const
{
	return rects.isEmpty ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<int maxRects, typename T>
inline bool RectList<maxRects, T>::isEqual (const RectList& other) const
{
	if(rects.count () != other.rects.count ())
		return false;
	for(int i = 0; i < rects.count (); i++)
		if(rects[i] != other.rects[i])
			return false;
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<int maxRects, typename T>
inline TRect<T> RectList<maxRects, T>::getBoundingBox () const
{
	TRect<T> bounding;
	bounding.setReallyEmpty ();

	ForEachRectFast (*this, TRect<T>, r)
		bounding.join (r);
	EndFor

	return bounding;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<int maxRects, typename T>
inline const ConstVector<TRect<T> >& RectList<maxRects, T>::getRects () const
{
	return rects;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<int maxRects, typename T>
inline bool RectList<maxRects, T>::operator == (const TRect<T>& rect) const
{
	return rects.count () == 1 && rects[0] == rect;
}

} // namespace Core

#endif // _corerectlist_h
