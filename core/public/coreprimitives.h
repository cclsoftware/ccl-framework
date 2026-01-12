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
// Filename    : core/public/coreprimitives.h
// Description : Primitives
//
//************************************************************************************************

#ifndef _coreprimitives_h
#define _coreprimitives_h

#include "core/public/coretypes.h"

#define CORE_HAVE_GCC_BSWAP16 (__GNUC__ && (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 8))) || __llvm__
#define CORE_HAVE_GCC_BSWAP32 (__GNUC__ && (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 5 && __GNUC_PATCHLEVEL__ >= 1))) || __llvm__
#define CORE_HAVE_GCC_BSWAP64 CORE_HAVE_GCC_BSWAP32

#if _MSC_VER
#include <intrin.h>

#elif CORE_HAVE_GCC_BSWAP16 || CORE_HAVE_GCC_BSWAP32 || CORE_HAVE_GCC_BSWAP64
// use GCC intrinsics

#elif __ARM_ARCH_7M__ || __ARM_ARCH_7EM__ || __ARM_ARCH_6EM__
#include <intrinsics.h>
#endif

namespace Core {

//************************************************************************************************
// Byte order primitives
//************************************************************************************************

template<typename T> 
T byte_swap (T value) { return value; }

//////////////////////////////////////////////////////////////////////////////////////////////////

/** Swap 16 bit integer. \ingroup core */
template<>
int16 INLINE byte_swap (int16 value)
{
#if CORE_HAVE_GCC_BSWAP16
	return __builtin_bswap16 (value);
#else
	char* p = (char*)&value; char t;
	t = p[0]; p[0] = p[1]; p[1] = t;
	return value;
#endif
}

/** Swap 16 bit integer (unsigned). \ingroup core */
template<>
uint16 INLINE byte_swap (uint16 value)
{ return (uint16)byte_swap ((int16)value); }

//////////////////////////////////////////////////////////////////////////////////////////////////

/** Swap 32 bit integer. \ingroup core */
template<>
int32 INLINE byte_swap (int32 value)
{
#if CORE_HAVE_GCC_BSWAP32
	return __builtin_bswap32 (value);
#elif __ARM_ARCH_7M__ || __ARM_ARCH_7EM__ || __ARM_ARCH_6EM__
	return __rev(value);
#else
	char* p = (char*)&value; char t;
	t = p[0]; p[0] = p[3]; p[3] = t; t = p[1]; p[1] = p[2]; p[2] = t;
	return value;
#endif
}

/** Swap 32 bit integer (unsigned). \ingroup core */
template<>
uint32 INLINE byte_swap (uint32 value)
{ return (uint32)byte_swap ((int32)value); }

//////////////////////////////////////////////////////////////////////////////////////////////////

/** Swap 64 bit integer. \ingroup core */
template<>
int64 INLINE byte_swap (int64 value)
{
#if CORE_HAVE_GCC_BSWAP64
	return __builtin_bswap64 (value);
#else
	char* p = (char*)&value; char t;
	t = p[0]; p[0] = p[7]; p[7] = t; t = p[1]; p[1] = p[6]; p[6] = t;
	t = p[2]; p[2] = p[5]; p[5] = t; t = p[3]; p[3] = p[4]; p[4] = t;
	return value;
#endif
}

/** Swap 64 bit integer (unsigned). \ingroup core */
template<>
uint64 INLINE byte_swap (uint64 value)
{ return (uint64)byte_swap ((int64)value); }

//////////////////////////////////////////////////////////////////////////////////////////////////

/** Swap 32 float float. \ingroup core */
template<>
float32 INLINE byte_swap (float32 value)
{
	char* p = (char*)&value; char t;
	t = p[0]; p[0] = p[3]; p[3] = t; t = p[1]; p[1] = p[2]; p[2] = t;
	return value;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

/** Swap 64 bit float. \ingroup core */
template<>
float64 INLINE byte_swap (float64 value)
{
	char* p = (char*)&value; char t;
	t = p[0]; p[0] = p[7]; p[7] = t; t = p[1]; p[1] = p[6]; p[6] = t;
	t = p[2]; p[2] = p[5]; p[5] = t; t = p[3]; p[3] = p[4]; p[4] = t;
	return value;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

#if CORE_NATIVE_BYTEORDER == CORE_LITTLE_ENDIAN 
	#define MAKE_BIG_ENDIAN(v)      Core::byte_swap (v)	
	#define MAKE_LITTLE_ENDIAN(v)   (v)
#else
	#define MAKE_BIG_ENDIAN(v)      (v)	
	#define MAKE_LITTLE_ENDIAN(v)   Core::byte_swap (v)
#endif

//************************************************************************************************
// Bit primitives
//************************************************************************************************

/** Checks if the given integer is a power of 2. 
    http://graphics.stanford.edu/~seander/bithacks.html#DetermineIfPowerOf2
*/
template <typename T>
bool is_power2 (T x)
{
	return ((x != 0) && !(x & (x - 1)));
}

/** Find the index of the first bit set. \ingroup core */
int INLINE find_first_set (int32 mask)
{
#if _MSC_VER
	unsigned long index;
	if(_BitScanForward(&index, mask))
		return static_cast<int>(index);
	else
		return -1;
#elif __GNUC__
	return __builtin_ffs(mask) - 1;
#else
	for(int i = 0; i < 32; i++)
	{
		if(mask & (1 << i))
			return i;
	}

	return -1;
#endif
}

//////////////////////////////////////////////////////////////////////////////////////////////////

/** Return value of bit at given position. \ingroup core */
template <typename T>
bool INLINE get_bit (T mask, int index)
{
	return (mask & (T(1) << index)) != 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

/** Set value of bit at given position. \ingroup core */
template <typename T>
void INLINE set_bit (T& mask, int index, bool state = true)
{
	if(state)
		mask |= (T(1) << index);
	else
		mask &= ~(T(1) << index);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

/** Check if flag is set in bit mask. \ingroup core */
template <typename T>
bool INLINE get_flag (T mask, T flag)
{
	return (mask & flag) != 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

/** Set flag in bit mask. \ingroup core */
template <typename T>
void INLINE set_flag (T& mask, T flag, bool state = true)
{
	if(state)
		mask |= flag;
	else
		mask &= ~flag;
}

//************************************************************************************************
// Numerical primitives
//************************************************************************************************

/** Get minimum of two values. */
template <typename T>
inline T get_min (T a, T b)
{
	return a < b ? a : b;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

/** Get maximum of two values. */
template <typename T>
inline T get_max (T a, T b)
{
	return a > b ? a : b;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<int64 a, int64 b> 
struct static_min
{ 
	static const int value = a < b ? a : b; 
};

//////////////////////////////////////////////////////////////////////////////////////////////////

template<int64 a, int64 b> 
struct static_max
{ 
	static const int value = a > b ? a : b; 
};

//////////////////////////////////////////////////////////////////////////////////////////////////

/** Get the absolute value */
template <typename T> 
inline T get_abs (const T v)
{
	return v >= 0 ? v : -v;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

/** Bound value between given minimum and maximum. */
template <typename T> 
inline T bound (const T v, const T vmin = 0, const T vmax = 1)
{
	return v > vmax ? vmax : v < vmin ? vmin : v;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

/** Get the signum of a value. */
template <typename T> 
inline T sign (const T v)
{
	return v == 0 ? 0 : (v > 0 ? 1 : (T)-1);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

/** Round to the given floating point value to integer. */
template <typename FloatType>
inline int round (FloatType v)
{
	return (int)(v + FloatType(.5) * sign (v));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

/** Swap the contents of two variables. */
template <typename T>
inline void swap_vars (T& a, T& b)
{
	T tmp (a);
	a = b;
	b = tmp;
}

//************************************************************************************************
// Deleter
/**	Pointer deleter.
	\ingroup core */
//************************************************************************************************

template <class T>
struct Deleter
{
	T* _ptr;
	Deleter (T* ptr): _ptr (ptr) {}
	~Deleter () { if(_ptr) delete _ptr; }
};

//************************************************************************************************
// VectorDeleter
/**	Deleter for vectors.
	\ingroup core */
//************************************************************************************************

template <class T>
struct VectorDeleter
{
	T* _ptr;
	VectorDeleter (T* ptr): _ptr (ptr) {}
	~VectorDeleter () { if(_ptr) delete [] _ptr; }
};

//************************************************************************************************
// ScopedVar
/**	Sets the var to value when scope is entered and restores the previous value when scope is left.
	\ingroup core */
//************************************************************************************************

template<typename T>
struct ScopedVar
{
	T& var;
	T old;
	ScopedVar (T& var, const T& value): var (var) { old = var; var = value; }
	~ScopedVar () { var = old; }
};

//************************************************************************************************
// ScopedFlag
/**	Sets the flag in the variable when scope is entered and clears it when scope is left.
	\ingroup core */
//************************************************************************************************

template<int kFlag, typename T = int>
struct ScopedFlag
{
	T& var;
	ScopedFlag (T& var) : var (var) { var |= kFlag; }
	~ScopedFlag () { var &= ~kFlag; }
};

} // namespace Core

#endif // _coreprimitives_h
