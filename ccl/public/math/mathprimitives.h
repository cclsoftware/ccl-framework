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
// Filename    : ccl/public/math/mathprimitives.h
// Description : Mathematical Primitives
//
//************************************************************************************************

#ifndef _ccl_mathprimitives_h
#define _ccl_mathprimitives_h

#include "ccl/public/base/primitives.h"

#include "core/public/coremath.h"

// force inlining with MSVC++ Compiler
#if _MSC_VER
#pragma intrinsic (fabs)
#endif

namespace CCL {

//////////////////////////////////////////////////////////////////////////////////////////////////

// import Core Framework definitions
using Core::static_power;

//////////////////////////////////////////////////////////////////////////////////////////////////

/** Round to the given number of digits. */
template<int digits, typename FloatType>
inline FloatType ccl_round (FloatType v)
{
	int64 f = static_power<10, digits>::value;
	return (v < (FloatType)0)
		? (FloatType)ceil  (v * f - FloatType(.5)) / FloatType(f)
		: (FloatType)floor (v * f + FloatType(.5)) / FloatType(f);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

/** Calculate the remainder of the euclidean division.
    Unlike the % operator, the result is always positive and between 0 and base - 1 (also for a negative dividend). */
template<typename T>
inline T ccl_modulus (T dividend, T base)
{
	return ((dividend % base) + base) % base;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

/** Calculate the greatest common divisor of two integers. */
template<typename IntType>
inline IntType ccl_gcd (IntType a, IntType b)
{
	while(b != 0)
	{
		IntType c = a % b;
		a = b;
		b = c;
	}
	return a >= 0 ? a : a * -1;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

/** Calculate the least common multiple of two integers. */
template<typename IntType>
inline IntType ccl_lcm (IntType a, IntType b)
{
	if(a == 0 && b == 0) return 0;

	if(a < 0) a *= -1;
	if(b < 0) b *= -1;

	return IntType ((uint64 (a) * b) / ccl_gcd (a, b));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

/** Round up to the nearest power of 2. */
inline uint32 ccl_upperPowerOf2 (uint32 value)
{
	// http://graphics.stanford.edu/~seander/bithacks.html#RoundUpPowerOf2
	value--;
	value |= value >> 1;
	value |= value >> 2;
	value |= value >> 4;
	value |= value >> 8;
	value |= value >> 16;
	value++;
	return value;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

/** Normalize a value to the given range. */
template<typename FloatType>
inline FloatType ccl_normalize (FloatType value, FloatType min, FloatType max)
{
	return (max > min) ? ccl_bound ((value - min) / (max - min), FloatType (0), FloatType (1)) : FloatType (0);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

/** DeNormalize a value to the given range. */
template<typename FloatType>
inline FloatType ccl_fromNormalized (FloatType normalized, FloatType min, FloatType max)
{
	return (max > min) ? ((max - min) * normalized) + min : FloatType (0);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

/** Calculate overlap amout of 2 ranges. result is <= 0 when ranges do not overlap */
template<typename T>
inline T ccl_getOverlap (T start1, T end1, T start2, T end2)
{
	return ccl_min<T> (end1, end2) - ccl_max<T> (start1, start2);
}

namespace Math {

template <typename T> struct Constants;
template <typename T> struct Functions;

//************************************************************************************************
// Mathematical constants
//************************************************************************************************

/** Specialization for 32 bit float. */
template <> 
struct Constants<float>
{
	static const float kPi;	
	static const float kTwoPi;
	static const float kHalfPi;
	static const float kPiInv;
	static const float kTwoPiInv;
	static const float kHalfPiInv;
	static const float kE;			
	static const float kSqrtTwo;
	static const float kSqrtTwoInv;
	static const float kAntiDenormal;
};

/** Specialization for 64 bit float. */
template <>
struct Constants<double>
{
	static const double kPi;	
	static const double kTwoPi;
	static const double kHalfPi;
	static const double kPiInv;
	static const double kTwoPiInv;
	static const double kHalfPiInv;
	static const double kE;			
	static const double kSqrtTwo;
	static const double kSqrtTwoInv;
	static const double kAntiDenormal;
};

//************************************************************************************************
// Mathematical functions
//************************************************************************************************

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
	#if CCL_NATIVE_BYTEORDER == CCL_BIG_ENDIAN
	{ return ((((const int*)&d)[0]) & 0x7fe00000) != 0x7fe00000; }
	#else
	{ return ((((const int*)&d)[1]) & 0x7fe00000) != 0x7fe00000; }
	#endif

	static inline bool isDenormal (const double& d)
	#if CCL_NATIVE_BYTEORDER == CCL_BIG_ENDIAN
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

/** Convert degrees to radians. */
template <typename T> 
inline T degreesToRad (T a)
{
	return (Constants<T>::kPi / T(180)) * a;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

/** Convert radians to degrees. */
template <typename T> 
inline T radToDegrees (T a)
{
	return a * (T(180) / Constants<T>::kPi);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

/** Convert mm to inch . */
template <typename T> 
inline T millimeterToInch (T mm)
{
	return T (mm / 25.4);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

/** Convert inch to mm. */
template <typename T> 
inline T inchToMillimeter (T inch)
{
	return T (inch * 25.4);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

/** Convert inch to coordinates (dots). */
template <typename T> 
inline T inchToCoord (T inch, T dpi)
{
	return T (inch * dpi);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

/** Convert millimeter to coordinates (dots). */
template <typename T> 
inline T millimeterToCoord (T mm, T dpi)
{
	return inchToCoord (millimeterToInch (mm), dpi);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

/** Calculate DPI when one dot (coordinate) should be x inches wide. */
template <typename T> 
inline T dpiFromCoordSizeInch (T coordSizeInInch)
{
	return coordSizeInInch > T(0) ? (T(1) / T (coordSizeInInch)) : T(0);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

/** Calculate DPI when one dot (coordinate) should be x millimeters wide. */
template <typename T> 
inline T dpiFromCoordSizeMillimeter (T coordSizeInMM)
{
	return dpiFromCoordSizeInch (millimeterToInch (coordSizeInMM));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

/** Convert cent to frequency factor. */
template <typename T> 
inline T centToFreqFactor (T cent)
{
	return cent == 0 ? T(1) : Functions<T>::pow (T(2), cent * (T(1)/T(1200)));
}	
    
//////////////////////////////////////////////////////////////////////////////////////////////////

/** Convert cent to freq */
template <typename T> 
inline T centToFreq (T cent)
{
    return T(8.176) * Functions<T>::pow (T(2), cent * (T(1)/T(1200)));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

/** Convert freq to cent  */
template <typename T> 
inline T freqToCent (T freq)
{
    return Functions<T>::log2 (freq / T(8.176)) * T(1200);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

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

/** Convert timecent to seconds. */
template <class T>
inline T timeCentToSeconds (T timeCent)
{
	return Functions<T>::pow (2, timeCent / (T)1200.); 
}

//////////////////////////////////////////////////////////////////////////////////////////////////

/** Convert seconds to timecent. */
template <class T>
inline T secondsToTimeCent (T seconds)
{
	return (Functions<T>::log (seconds) * (T)1.442695040889f) * (T)1200.;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

/** Modify input value using a concave logarithmic curve. */
template <class T>
inline T concave (T v)
{
	if(v >= 1.f)
		return 1.f;

	return -(5.f/12.f) * Functions<T>::log10 (1.f-v);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

/** Modify input value using a convex logarithmic curve. */
template <class T>
inline T convex (T v)
{
	return T(1.) + T(5./12.) * Functions<T>::log10 ((v < T(0.0001)) ? T(0.0001) : v);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

/** Modify input value using a concave quadratic curve. */
template <class T>
inline T quadConcave (T v)
{
	return v * v;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

/** Modify input value using a convex quadratic curve. */
template <class T>
inline T quadConvex (T v)
{
	return T(1.) - ((T(1.) - v) * (T(1.) - v));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

/** convert xy-coordinates to angle and length */
template <class T>
inline void cartesianToPolar (T& theta, T& r, T deltaX, T deltaY)
{
	r = Functions<T>::sqrt (deltaX * deltaX + deltaY * deltaY);

	if(deltaX == T (0.))
	{
		theta = Constants<T>::kHalfPi;
		if(deltaY < T (0.))
			theta += Constants<T>::kPi;
	}
	else
	{
		theta = Functions<T>::atan (deltaY / deltaX);
		if(deltaX < T (0.))
			theta += Constants<T>::kPi;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

/** convert angle and length to xy-coordinates */
template <class T>
inline void polarToCartesian (T& deltaX, T& deltaY, T theta, T r)
{
	deltaX = r * Functions<T>::cos (theta);
	deltaY = r * Functions<T>::sin (theta);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

/** stretch the radius of a circle to a square */
template <class T>
inline T stretchRadiusToSquare (T theta, T r)
{	
	const T kHalfPi = Constants<T>::kHalfPi;
	const T kQuarterPi = kHalfPi * T(.5);

	while(theta > kHalfPi)
		theta -= kHalfPi;
	while(theta < T (0))
		theta += kHalfPi;

	T stretched = 0;
	if(theta > kQuarterPi)
		stretched = r / Functions<T>::sin (theta);	
	else
		stretched = r / Functions<T>::cos (theta);
	return stretched;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

/** normalize an angle in radians to (-pi, pi) */
template <class T>
inline void normalizeAngle (T& angle)
{
	const T kPi = Constants<T>::kPi;

	while(angle < -kPi)
		angle += 2 * kPi;
	while(angle > kPi)
		angle -= 2 * kPi;
}

} // namespace Math
} // namespace CCL

#endif // _ccl_mathprimitives_h
