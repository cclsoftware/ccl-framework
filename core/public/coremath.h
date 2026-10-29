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
// Filename    : core/public/coremath.h
// Description : Mathematical Utilities
//
//************************************************************************************************

#ifndef _coremath_h
#define _coremath_h

#include "core/public/coretypes.h"

#include <math.h>

namespace Core {

//************************************************************************************************
// static_power
/** Calculates base^exponent at compile time. */
//************************************************************************************************

template<int64 base, int exponent>
struct static_power
{
    static const int64 value = base * static_power<base, exponent-1>::value;
};

template<int64 base>
struct static_power<base, 0>
{
    static const int64 value = 1;
};

//************************************************************************************************
// Mathematical functions
//************************************************************************************************

namespace MathUtils {

template <typename T> struct Functions;

// force inlining with MSVC++ Compiler
#if _MSC_VER
#pragma intrinsic (fabs)
#endif

/** Specialization for 32 bit float. */
template <> 
struct Functions<float>
{
	static inline float abs (float f)
	{ return fabsf (f); }

	static inline float sqrt (float f)	
	{ return sqrtf (f); }

	static inline float exp (float f)	
	{ return expf (f); }

	static inline float log (float f)
	{ return logf (f); }

	static inline float log2 (float f)
	{ return logf (f) / logf (2.f); }		// log2() not available on MSVC++!

	static inline float log10 (float f)
	{ return log10f (f); }

	static inline float tan (float f)
	{ return tanf (f); }

	static inline float atan (float f)
	{ return atanf (f); }

	static inline float sin (float f)
	{ return sinf (f); }

	static inline float cos (float f)
	{ return cosf (f); }

	static inline float pow (float b, float e)
	{ return powf (b, e); }
	
	static inline bool isNan (const float& f)
	{ return ((*(const int*)&f) & 0x7F800000) == 0x7F800000; }

	static inline bool isDenormal (const float& f)
	{ return ((*(const int*)&f) & 0x7F800000) == 0 && ((*(const int*)&f) & 0x7FFFFFFF) != 0; }

	static inline void unDenormalise (float& f)
	{ if(isDenormal (f)) f = 0.f; }
};

/** Specialization for 64 bit float. */
template <>
struct Functions<double>
{
	static inline double abs (double f) 
	{ return (double)::fabs (f); }

	static inline double sqrt (double f)
	{ return (double)::sqrt (f); }

	static inline double exp (double f)
	{ return (double)::exp (f); }

	static inline double log (double f)
	{ return (double)::log (f); }

	static inline double log2 (double f)
	{ return (double)(::log (f) / ::log (2.)); }		// log2() not available on MSVC++!

	static inline double log10 (double f)
	{ return (double)::log10 (f); }

	static inline double tan (double f)
	{ return (double)::tan (f); }

	static inline double atan (double f)
	{ return (double)::atan (f); }

	static inline double sin (double f)
	{ return (double)::sin (f); }

	static inline double cos (double f)
	{ return (double)::cos (f); }

	static inline double pow (double b, double e)
	{ return (double)::pow (b, e); }

	static inline bool isNan (const double& d)
	#if CORE_NATIVE_BYTEORDER == CORE_BIG_ENDIAN
	{ return ((((const int*)&d)[0]) & 0x7fe00000) != 0x7fe00000; }
	#else
	{ return ((((const int*)&d)[1]) & 0x7fe00000) != 0x7fe00000; }
	#endif

	static inline bool isDenormal (const double& d)
	#if CORE_NATIVE_BYTEORDER == CORE_BIG_ENDIAN
	{ return ((((const int*)&d)[0]) & 0x7fe00000) != 0; }
	#else
	{ return ((((const int*)&d)[1]) & 0x7fe00000) != 0; }
	#endif

	static inline void unDenormalise (double& d)
	{ if(isDenormal (d)) d = 0.; }
};

//************************************************************************************************
// Conversion functions
//************************************************************************************************

#define DB_MAX		144
#define LEVEL_MIN	6.309573444802e-008

/** Convert dB to factor. */
template <class T>
inline T dbToFactor (T db)
{
	if(db < -DB_MAX)
		return 0;
	else
		return Functions<T>::pow (T(10), db * T(0.05));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

/** Convert factor to dB. */
template <class T>
inline T factorToDb (T factor)
{
	if(factor < LEVEL_MIN)
		return -DB_MAX;
	else
		return (T(20) * Functions<T>::log10 (factor));
}

#undef DB_MAX
#undef LEVEL_MIN

//////////////////////////////////////////////////////////////////////////////////////////////////

} // namespace MathUtils
} // namespace Core

#endif // _coremath_h
