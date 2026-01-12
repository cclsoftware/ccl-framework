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
// Filename    : ccl/base/math/mathrange.h
// Description : Range class
//
//************************************************************************************************

#ifndef _ccl_mathrange_h
#define _ccl_mathrange_h

#include "ccl/public/base/primitives.h"

namespace CCL {
namespace Math {

//************************************************************************************************
// Math::Range
//************************************************************************************************

template <class T>
struct Range
{
	T start;
	T end;

	Range ()
	: start (0),
	  end (0)
	{}

	constexpr Range (const T _start, const T _end)
	: start (_start),
	  end (_end)
	{}

	/** Check if range is empty. */
	bool isEmpty () const;

	/** Get range length. */
	T getLength () const;

	/** Check for overlap. */
	bool isOverlap (const Range& other) const;
	
	/** Get overlap with other range. */
	bool getOverlap (Range& result, const Range& other) const;
	
	/** Get length of overlap with other range. */
	T getOverlapLength (const Range& other) const;

	/** Get scaled value from range (0 -> start, 1 -> end). */
	T scaleValue (const T factor) const;

	/** Bound to overlap with other range. Returns false, if result is empty. */
	bool bound (const Range& other);

	/** Enlarge if necessary to include given value. */
	Range& include (T value);

	/** Join with other range. */
	Range& join (const Range& other);

	/** Offset start and end. */
	Range& offset (T offset);
	
	/** Get value bound to this range. */
	T getBound (T value) const;
	
	/** Check if value is inside this range as a closed interval [start, end]. */
	bool isInsideClosed (T value) const;

	/** Check if value is inside this range as an open interval (start, end). */
	bool isInsideOpen (T value) const;

	/** Check if value is inside this range as a left-closed, right-open interval [start, end). */
	bool isInside (T value) const;

	/** Assign new range. */
	Range& operator () (T start, T end);

	/** Compare ranges. */
	bool operator == (const Range& other) const;
	bool operator != (const Range& other) const;
};

//************************************************************************************************
// Range inline
//************************************************************************************************

template <class T> 
bool Range<T>::isEmpty () const	
{ return start >= end; }

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class T> 
T Range<T>::getLength () const
{ return end - start; }

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class T>
bool Range<T>::isOverlap (const Range& other) const	
{ Range temp; return getOverlap (temp, other); }

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class T>
bool Range<T>::getOverlap (Range& result, const Range& other) const
{
	result.start = ccl_max<T> (start, other.start);
	result.end   = ccl_min<T> (end,   other.end);
	return !result.isEmpty ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class T>
T Range<T>::getOverlapLength (const Range& other) const
{
	Range temp;
	if(getOverlap (temp, other))
		return temp.getLength ();
	return 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class T>
T Range<T>::scaleValue (const T factor) const
{ return ccl_bound<T> (start + factor * getLength (), start, end); }

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class T>
bool Range<T>::bound (const Range& other)
{ return getOverlap (*this, other); }

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class T>
Range<T>& Range<T>::join (const Range& other)
{
	if(other.start < start)
		start = other.start;
	if(other.end > end)
		end = other.end;
	return *this;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class T>
Range<T>& Range<T>::include (T value)
{
	if(value < start)
		start = value;
	if(value > end)
		end = value;
	return *this;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class T>
inline Range<T>& Range<T>::offset (T offset)
{ start += offset; end += offset; return *this; }

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class T>
T Range<T>::getBound (T value) const 
{ return ccl_bound<T> (value, start, end); }

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class T>
bool Range<T>::isInsideClosed (T value) const
{ return value >= start && value <= end; }

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class T>
bool Range<T>::isInsideOpen (T value) const
{ return value > start && value < end; }

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class T>
bool Range<T>::isInside (T value) const
{ return value >= start && value < end; }

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class T>
inline Range<T>& Range<T>::operator () (T _start, T _end)
{ start = _start; end = _end; return *this; }

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class T>
inline bool Range<T>::operator == (const Range& other) const
{ return start == other.start && end == other.end; }

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class T>
inline bool Range<T>::operator != (const Range& other) const
{ return start != other.start || end != other.end; }

//////////////////////////////////////////////////////////////////////////////////////////////////

} // namespace Math
} // namespace CCL

#endif // _ccl_mathrange_h
