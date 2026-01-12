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
// Filename    : ccl/public/base/primitives.h
// Description : Primitives
//
//************************************************************************************************

#ifndef _ccl_primitives_h
#define _ccl_primitives_h

#include "ccl/public/base/platform.h"

#include "core/public/coreprimitives.h"

namespace CCL {

//////////////////////////////////////////////////////////////////////////////////////////////////
/** \addtogroup ccl_base 
@{ */

// import Core Framework definitions
using Core::is_power2;
using Core::get_bit;
using Core::set_bit;
using Core::get_flag;
using Core::set_flag;
using Core::byte_swap;
using Core::swap_vars;
using Core::get_min;
using Core::get_max;
using Core::get_abs;
using Core::Deleter;
using Core::VectorDeleter;
using Core::ScopedVar;
using Core::ScopedFlag;

#define ccl_is_power2	Core::is_power2
#define ccl_min			Core::get_min
#define ccl_max			Core::get_max
#define ccl_abs			Core::get_abs
#define ccl_bound		Core::bound
#define ccl_sign		Core::sign
#define ccl_swap		Core::swap_vars

//************************************************************************************************
// Numerical Limits
//************************************************************************************************

namespace NumericLimits
{
	/** Maximum of 32 bit integer (unsigned). */
	extern const unsigned int kMaxUnsignedInt;

	/** Maximum of 32 bit integer. */
	extern const int kMaxInt;

	/** Minimum of 32 bit integer. */
	extern const int kMinInt;

	/** Maximum of 8 bit integer (unsigned). */
	extern const uint8 kMaxUnsignedInt8;

	/** Maximum of 8 bit integer. */
	extern const int8 kMaxInt8;

	/** Minimum of 8 bit integer. */
	extern const int8 kMinInt8;

	/** Maximum of 16 bit integer (unsigned). */
	extern const uint16 kMaxUnsignedInt16;

	/** Maximum of 16 bit integer. */
	extern const int16 kMaxInt16;

	/** Minimum of 16 bit integer. */
	extern const int16 kMinInt16;

	/** Maximum of 32 bit integer (unsigned). */
	extern const uint32 kMaxUnsignedInt32;

	/** Maximum of 32 bit integer. */
	extern const int32 kMaxInt32;

	/** Minimum of 32 bit integer. */
	extern const int32 kMinInt32;

	/** Maximum of 64 bit integer (unsigned). */
	extern const uint64 kMaxUnsignedInt64;

	/** Maximum of 64 bit integer. */
	extern const int64 kMaxInt64;

	/** Minimum of 64 bit integer. */
	extern const int64 kMinInt64;

	/** Maximum of 32 bit float. Don't use in computations, because it cause numerical overflow. */
	extern const float kMaximumFloat;

	/** Minimum of 32 bit float. Don't use in computations, because it cause numerical overflow.  */
	extern const float kMinimumFloat;

	/** Maximum of 64 bit float. Don't use in computations, because it cause numerical overflow. */
	extern const double kMaximumDouble;

	/** Minimum of 64 bit float. Don't use in computations, because it cause numerical overflow.  */
	extern const double kMinimumDouble;

	/** Maximum of 64 bit float. */
	extern const double kLargeDouble;

	/** Minimum of 64 bit float. */
	extern const double kSmallDouble;

	/** Small value to be used as tolerance for comparing values. */
	extern const double kPrecision;

	/** Support of limits in generic functions */
	template <typename T> T minValue ();        /// template declaration (no implementation, only specializations used)
	template <typename T> T maxValue ();        /// template declaration (no implementation, only specializations used)

	template <> inline int8 minValue ()         { return kMinInt8; }
	template <> inline int8 maxValue ()         { return kMaxInt8; }
	template <> inline uint8 minValue ()        { return 0; }
	template <> inline uint8 maxValue ()        { return kMaxUnsignedInt8; }

	template <> inline int16 minValue ()        { return kMinInt16; }
	template <> inline int16 maxValue ()        { return kMaxInt16; }
	template <> inline uint16 minValue ()       { return 0; }
	template <> inline uint16 maxValue ()       { return kMaxUnsignedInt16; }

	template <> inline int minValue ()          { return kMinInt; }
	template <> inline int maxValue ()          { return kMaxInt; }
	template <> inline uint32 minValue ()       { return 0; }
	template <> inline uint32 maxValue ()       { return kMaxUnsignedInt32; }

	template <> inline int64 minValue ()        { return kMinInt64; }
	template <> inline int64 maxValue ()        { return kMaxInt64; }
	template <> inline uint64 minValue ()       { return 0; }
	template <> inline uint64 maxValue ()       { return kMaxUnsignedInt64; }

	template <> inline float minValue ()        { return kMinimumFloat; }
	template <> inline float maxValue ()        { return kMaximumFloat; }
	template <> inline double minValue ()       { return kMinimumDouble; }
	template <> inline double maxValue ()       { return kMaximumDouble; }
}

//************************************************************************************************
// Numerical primitives
//************************************************************************************************

/** Limit the value of a variable to a maximum value. */
template <typename T>
inline T& ccl_upper_limit (T& var, T limit)
{
	if(var > limit)
		var = limit;
	return var;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

/** Limit the value of a variable to a minimum value. */
template <typename T>
inline T& ccl_lower_limit (T& var, T limit)
{
	if(var < limit)
		var = limit;
	return var;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

/** Get value with lowest distance to v. */
template <typename T>
inline T ccl_nearest (const T v1, const T v2, const T v)
{
	return ccl_abs (v1 - v) <= ccl_abs (v2 - v) ? v1 : v2;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

/** Get the greatest common divisor of two integers. */
template <typename T>
inline T ccl_greatest_common_divisor (const T a, const T b)
{
	return (a == 0) ? b : ccl_greatest_common_divisor (b % a, a);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

/** Get the lowest common multiple of two integers. */
template <typename T>
inline T ccl_lowest_common_multiple (const T a, const T b)
{
	return (a == 0 && b == 0) ? 0 : (a * b) / ccl_greatest_common_divisor (a, b);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

/** Determine number of digits. */
template <typename T>
inline int ccl_digits_of (T v)
{
	int number = 1;
	while(v /= 10)
		number++;
	return number;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

/** Swap the contents of an array. */
template <typename T>
inline void ccl_swap_array (T a[], int n)
{
	for(int i = 0; i < n/2; i++)
		ccl_swap (a[i], a[n-1-i]);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

/** Swap the contents of two variables if the first is greater. */
template <typename T>
inline void ccl_order (T& a, T& b)
{
	if(a > b)
		ccl_swap (a, b);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

/** Compare two values (returns -1/0/+1). */
template <typename T>
inline int ccl_compare (T a, T b)
{ return a == b ? 0 : (a > b) ? 1 : -1; } // (don't use ccl_sign to avoid float to int conversion)

//////////////////////////////////////////////////////////////////////////////////////////////////

/** Compare two values approximately (returns -1/0/+1). */
template <typename T>
inline int ccl_compare (T a, T b, T epsilon)
{
	if(a == b || (b < (a + epsilon) && b > (a - epsilon)) || (a < (b + epsilon) && a > (b - epsilon)))
		return 0;
	else if(a < b)
		return -1;
	else
		return 1;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

/** Compare two values for approximate equality. */
template <typename T>
inline bool ccl_equals (T a, T b, T epsilon)
{
	return a == b || ccl_abs (a - b) < epsilon;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

/** Get value from list nearest to given value. */
template <typename TList, typename T>
inline T ccl_get_nearest (const TList& list, T value)
{
	T nearest = T(0);
	T minDiff = T(0);
	bool first = true;
	for(T listValue : list)
	{	
		T diff = ccl_abs (listValue - value);
		if(first || diff < minDiff)
		{
			minDiff = diff;				
			nearest = listValue;
			first = false;
			if(minDiff == T(0))
				break;
		}
	}
	return nearest;
}

//************************************************************************************************
// Type Conversion
//************************************************************************************************

/** Helper function to prepare float to integer conversion. */
template<typename FloatType>
inline FloatType ccl_prepare_rounding (FloatType v)
{
	return (v < FloatType (0)) ? (v - FloatType (0.5)) : (v + FloatType (0.5));
}

/** Specialization for int64 (used in ccl_convert_type). */
template<>
inline int64 ccl_prepare_rounding (int64 v)
{
	return v;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

/** Convert floating point to int rounded - without limit check. */
template<typename IntType, typename FloatType>
inline IntType ccl_to_int (FloatType v)
{
	return IntType (ccl_prepare_rounding<FloatType> (v));
}

/** Convert floating point to int - without limit check. */
template<typename FloatType>
inline int ccl_to_int (FloatType v)
{
	return ccl_to_int<int> (v);
}

/** Convert floating point to int64 - without limit check. */
template<typename FloatType>
inline int64 ccl_to_int64 (FloatType v)
{
	return ccl_to_int<int64> (v);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

/** ccl_type_cast - Convert larger type to smaller and verify limits. */
template<typename DestType, typename SourceType>
INLINE DestType ccl_type_cast (SourceType v)
{
	v = ccl_prepare_rounding <SourceType> (v);
	static const DestType min = NumericLimits::minValue <DestType> ();
	static const DestType max = NumericLimits::maxValue <DestType> ();
	return v >= max ? max : v <= min ? min : (DestType) (v);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

/** ccl_const_cast - const_cast for pointer / reference without having to specify the type. */
template<class T> T* ccl_const_cast (const T* obj)
{ return const_cast<T*> (obj); }

template<class T> T& ccl_const_cast (const T& obj)
{ return const_cast<T&> (obj); }

//************************************************************************************************
// Memory primitives
//************************************************************************************************

/** Copy typed array. */
template <typename T>
inline T* ccl_copy (T* dst, const T* src, unsigned int count)
{
	return (T*)::memcpy (dst, src, count * sizeof(T));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

/** Fill typed array. */
template <typename T>
inline T* ccl_memset (T* dst, int value, unsigned int count)
{
	return (T*)::memset (dst, value, count * sizeof(T));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

/** Check pointer alignment. */
template <typename T>
inline int ccl_is_aligned (const T* ptr)
{
	return IntPtr (ptr) % alignof (T) == 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

/** Round to nearest multiple of \a alignment, rounding up if \a value is positive, rounding down if \a value is negative. */
template<typename T>
inline T ccl_align_to (T value, T alignment)
{
	return ((value + (value > 0 ? (alignment - 1) : 0)) / alignment) * alignment;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

/** Hash pointer to given hash size. */
INLINE int ccl_hash_pointer (const void* ptr, int hashSize)
{
	// The result needs to be positive and evenly spread accross the range given by hashSize
	// we shift it, because allocated memory will most likely aligned to 8 or more bytes.
	if(hashSize)
		return (int)((((int64)ptr) & 0x7FFFFFFF) >> 6) % hashSize;
	else
		return 0;
}

//************************************************************************************************
// Interface primitives
//************************************************************************************************

/** Assign pointer variable with reference counting. */
template <typename T>
inline void take_shared (T*& member, T* value)
{
	if(value)
		value->retain ();
	if(member)
		member->release ();
	member = value;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

/** Return pointer adding a reference count. */
template <typename T>
inline T* return_shared (T* value)
{
	if(value)
		value->retain ();
	return value;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

/** Release and null pointer variable. */
template <typename T>
inline void safe_release (T*& member)
{
	if(member)
		member->release ();
	member = nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

interface IObserver;

/** Assign Object variable with reference counting and observer registration. */
template <typename T>
inline void share_and_observe (IObserver* This, T*& member, T* value)
{
	if(member)
	{
		member->removeObserver (This);
		member->release ();
	}
	member = value;
	if(member)
	{
		member->retain ();
		member->addObserver (This);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

/** Assign Object variable with observer registration to simple pointer member or SharedPtr. */
template <typename TValue, typename TMember>
inline bool assign_and_observe (IObserver* This, TMember& member, TValue* value)			
{
	if(member != value)
	{
		if(member)
			member->removeObserver (This);
		member = value;
		if(member)
			member->addObserver (This);
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
/** @} */

} // namespace CCL

#endif // _ccl_primitives_h
