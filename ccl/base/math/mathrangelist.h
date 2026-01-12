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
// Filename    : ccl/base/math/mathrangelist.h
// Description : Range List class
//
//************************************************************************************************

#ifndef _ccl_mathrangelist_h
#define _ccl_mathrangelist_h

#include "ccl/base/math/mathrange.h"

namespace CCL {
namespace Math {

template <class T> class RangeListIterator;

//************************************************************************************************
// Math::RangeList
/** Ordered list of ranges. */
//************************************************************************************************

template <class T>
class RangeList
{
public:
	RangeList ();
	RangeList (const RangeList& other);
	RangeList (const Range<T>& range);

	RangeList& include (const T start, const T end);
	RangeList& exclude (const T start, const T end);

	RangeList& include (const Range<T> range);
	RangeList& exclude (const Range<T> range);
	RangeList& include (const RangeList<T> rangeList);
	RangeList& exclude (const RangeList<T> rangeList);

	RangeList& fillGaps (T tolerance); ///< fill gaps smaller than tolerance by combining ranges

	bool isEmpty () const;
	bool isInside (T value) const;
	bool isOverlap (const T start, const T end) const;
	Range<T> getBounds () const;

	RangeIterator<LinkedList<T>, ListIterator<T>, T&> begin () const;
	RangeIterator<LinkedList<T>, ListIterator<T>, T&> end () const;

private:
	friend class RangeListIterator<T>;

	// define Container and iterator
	typedef LinkedList<Range<T> > Ranges;
	typedef ListIterator<Range<T> > RangesIterator;

	Ranges ranges;
};

//************************************************************************************************
// Math::RangeListIterator
//************************************************************************************************

template <class T>
class RangeListIterator
{
public:
	RangeListIterator (const RangeList<T>& rangeList);

	void first ();
	void last ();
	bool next (Range<T>& range);
	bool previous (Range<T>& range);

private:
	typename RangeList<T>::RangesIterator iter;
};

//************************************************************************************************
// Math::RangeList implementation
//************************************************************************************************

template <class T>
inline RangeList<T>::RangeList ()
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class T>
inline RangeList<T>::RangeList (const RangeList& other)
{
	RangesIterator iter (other.ranges);
	while(!iter.done ())
		ranges.append (iter.next ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class T>
inline RangeList<T>::RangeList (const Range<T>& range)
{
	include (range.start, range.end);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class T>
inline RangeList<T>& RangeList<T>::include (const Range<T> range)
{
	include (range.start, range.end);
	return *this;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class T>
inline RangeList<T>& RangeList<T>::exclude (const Range<T> range)
{
	exclude (range.start, range.end);
	return *this;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class T>
RangeList<T>& RangeList<T>::include (const T start, const T end)
{
	RangesIterator iter (ranges);
	while(!iter.done ())
	{
		Range<T>& range = iter.next ();

		// skip ranges ending before start
		if(range.end >= start)
		{
			if(range.start <= end) // found overlapping range
			{
				// 1) extend range left to include start
				ccl_upper_limit (range.start, start);

				// 2) extend range right to include end
				if(end > range.end)
				{
					range.end = end;

					// 3) absorb any following ranges touched by range
					while(!iter.done ())
					{
						Range<T>& next = iter.next ();
						if(next.start > end)
							return *this;	// beyond end

						if(next.end <= end)
						{
							// next is completely covered by range: remove it and continue
							ranges.remove (iter);
						}
						else
						{
							// next is longer than range: remove it, extend range
							range.end = next.end;
							ranges.remove (iter);
							return *this;
						}
					}
				}
			}
			else // range.start > end
			{
				// range is completely after end: insert before it
				ranges.insertBefore (iter, Range<T> (start,end));
			}
			return *this;
		}
	}

	// only saw ranges ending before start
	ranges.append (Range<T> (start,end));
	return *this;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class T>
RangeList<T>& RangeList<T>::exclude (const T start, const T end)
{
	RangesIterator iter (ranges);
	while(!iter.done ())
	{
		Range<T>& range = iter.next ();

		// skip ranges ending before start
		if(range.end >= start)
		{
			if(range.start <= end) // found overlapping range
			{
				if(start <= range.start)
				{
					if(end >= range.end)
						ranges.remove (iter); // remove range completely and continue
					else
					{
						range.start = end; // overlapping at start
						return *this; // (no following can overlap)
					}
				}
				else // start > range.start
				{
					if(end < range.end)
					{
						// cut a hole into range:
						ranges.insertAfter (iter, Range<T> (end, range.end)); // add new end range
						range.end = start; // shorten start
						return *this; // (no following can overlap)
					}
					else
						range.end = start; // cut at end and continue
				}
			}
			else // range.start > end
				return *this; // range is completely after end
		}
	}
	return *this;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class T>
inline RangeList<T>& RangeList<T>::include (const RangeList<T> rangeList)
{
	RangesIterator iter (rangeList.ranges);
	while(!iter.done ())
		include (iter.next ());
	return *this;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class T>
inline RangeList<T>& RangeList<T>::exclude (const RangeList<T> rangeList)
{
	RangesIterator iter (rangeList.ranges);
	while(!iter.done ())
		exclude (iter.next ());
	return *this;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class T>
inline RangeList<T>& RangeList<T>::fillGaps (T tolerance)
{
	RangesIterator iter (ranges);
	while(!iter.done ())
	{
		Range<T>& range = iter.next ();
		if(iter.done ())
			break; // at last range

		// if the gap between current and next range is small enough, enlarge next and remove current
		Range<T>& nextRange = iter.peekNext ();
		if(nextRange.start - range.end <= tolerance)
		{
			nextRange.start = range.start;
			ranges.remove (iter);
		}
	}
	return *this;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class T>
inline bool RangeList<T>::isEmpty () const
{
	return ranges.isEmpty ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class T>
bool RangeList<T>::isInside (T value) const
{
	RangesIterator iter (ranges);
	while(!iter.done ())
		if(iter.next ().isInside (value))
			return true;

	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class T>
bool RangeList<T>::isOverlap (const T start, const T end) const
{
	Range<T> range (start, end);
	RangesIterator iter (ranges);
	while(!iter.done ())
		if(iter.next ().isOverlap (range))
			return true;

	return false;

}

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class T>
Range<T> RangeList<T>::getBounds () const
{
	if(isEmpty ())
		return Range<T> ();

	return Range<T> (ranges.getFirst ().start, ranges.getLast ().end);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class T>
RangeIterator<LinkedList<T>, ListIterator<T>, T&> RangeList<T>::begin () const
{
	return ranges.begin ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class T>
RangeIterator<LinkedList<T>, ListIterator<T>, T&> RangeList<T>::end () const
{
	return ranges.end ();
}

//************************************************************************************************
// Math::RangeListIterator implementation
//************************************************************************************************

template <class T>
inline RangeListIterator<T>::RangeListIterator (const RangeList<T>& rangeList)
: iter (rangeList.ranges)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class T>
inline void RangeListIterator<T>::first ()
{ iter.first (); }

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class T>
inline void RangeListIterator<T>::last ()
{ iter.last (); }

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class T>
inline bool RangeListIterator<T>::next (Range<T>& range)
{
	if(iter.done ())
		return false;

	range = iter.next ();
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class T>
inline bool RangeListIterator<T>::previous (Range<T>& range)
{
	if(iter.done ())
		return false;

	range = iter.previous ();
	return true;
}

} // namespace Math
} // namespace CCL

#endif // _ccl_mathrangelist_h
