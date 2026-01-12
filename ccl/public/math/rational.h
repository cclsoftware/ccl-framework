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
// Filename    : ccl/public/math/rational.h
// Description : Rational number template class
//
//************************************************************************************************

#ifndef _ccl_rational_h
#define _ccl_rational_h

#include "ccl/public/math/mathprimitives.h"

namespace CCL {

namespace Math {

//************************************************************************************************
// Rational
//************************************************************************************************

template <class T>
class Rational
{
public:
	/** Construcion and assigment. */
	Rational (T numerator = 0, T denominator = 1);

	Rational<T>& assign (T numerator, T denominator = 1);
	Rational<T>& setZero ();

	/** Accessors. */
	T getNumerator () const { return numerator; }
	T getDenominator () const { return denominator; }
	void setNumerator (int num) { numerator = num;}
	void setDenominator (int denom) { denominator = denom; }

	/** Conversion to decimal. */
	inline float asFloat () const;
	inline double asDouble () const;

	/** Normalize rational number. */
	Rational<T>& normalize ();
	bool denormalize (T denominator);

	/** Comparison operators. */
	bool operator == (const Rational<T>& b) const;
	bool operator != (const Rational<T>& b) const;

	bool operator < (const Rational<T>& b) const;
	bool operator > (const Rational<T>& b) const;
	bool operator <= (const Rational<T>& b) const;
	bool operator >= (const Rational<T>& b) const;

	/** Arithmetic operators. */
	Rational<T> operator + (const Rational<T>& b) const;
	Rational<T> operator - (const Rational<T>& b) const;
	Rational<T> operator * (const Rational<T>& b) const;
	Rational<T> operator / (const Rational<T>& b) const;

	/** Arithmetic assignment operators. */
	Rational<T>& operator += (const Rational<T>& b);
	Rational<T>& operator -= (const Rational<T>& b);
	Rational<T>& operator *= (const Rational<T>& b);
	Rational<T>& operator /= (const Rational<T>& b);

protected:
	T numerator;
	T denominator;
};

typedef Rational<int> Rational32;
typedef Rational<int64> Rational64;

//////////////////////////////////////////////////////////////////////////////////////////////////
// Rational implementation
//////////////////////////////////////////////////////////////////////////////////////////////////

/** Construct with numerator and denominator. */
template <class T>
Rational<T>::Rational (T numerator, T denominator)
: numerator (numerator),
  denominator (denominator)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

/** Assign new value. */
template <class T>
Rational<T>& Rational<T>::assign (T num, T denom)
{
	numerator = num;
	denominator = denom;
	return *this;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class T>
Rational<T>& Rational<T>::setZero ()
{
	return this->assign (0, 1);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

/** Compute decimal float value. */
template <class T>
inline float Rational<T>::asFloat () const
{
	return ((float)numerator / (float)denominator);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

/** Compute decimal double value. */
template <class T>
inline double Rational<T>::asDouble () const
{
	return ((double)numerator / (double)denominator);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

/** Equal. */
template <class T>
inline bool Rational<T>::operator == (const Rational<T>& b) const
{
	return (numerator * b.denominator == b.numerator * denominator);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

/** Not equal. */
template <class T>
inline bool Rational<T>::operator != (const Rational<T>& b) const
{
	return !(*this == b);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

/** Less than. */
template <class T>
inline bool Rational<T>::operator < (const Rational<T>& b) const
{
	return (numerator * b.denominator < b.numerator * denominator);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

/** Greater than. */
template <class T>
inline bool Rational<T>::operator > (const Rational<T>& b) const
{
	return !(*this == b) && !(*this < b);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

/** Less or equal. */
template <class T>
inline bool Rational<T>::operator <= (const Rational<T>& b) const
{
	return (*this == b || *this < b);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

/** Greater or equal. */
template <class T>
inline bool Rational<T>::operator >= (const Rational<T>& b) const
{
	return !(*this < b);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

/** Addition assignment. */
template <class T>
Rational<T>& Rational<T>::operator += (const Rational<T>& b)
{
	return *this = *this + b;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

/** Subtraction assignment. */
template <class T>
Rational<T>& Rational<T>::operator -= (const Rational<T>& b)
{
	return *this = *this - b;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

/** Multiplication assignment. */
template <class T>
Rational<T>& Rational<T>::operator *= (const Rational<T>& b)
{
	return *this = *this * b;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

/** Division assignment. */
template <class T>
Rational<T>& Rational<T>::operator /= (const Rational<T>& b)
{
	return *this = *this / b;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

/** Addition operator. */
template <class T>
Rational<T> Rational<T>::operator + (const Rational<T>& b) const
{
	if(denominator == 0)
		return b;

	if(b.denominator == 0)
		return *this;

	if(denominator == b.denominator)
		return Rational<T> (numerator + b.numerator, denominator);

	// If denominators differ, calculate their LCM and divide that first to avoid overflow.
	T den = ccl_lcm (denominator, b.denominator);
	T num = numerator * (den / denominator) + b.numerator * (den / b.denominator);

	return Rational<T> (num, den);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

/** Subtraction operator. */
template <class T>
Rational<T> Rational<T>::operator - (const Rational<T>& b) const
{
	if(denominator == 0)
		return b;

	if(b.denominator == 0)
		return *this;

	if(denominator == b.denominator)
		return Rational<T> (numerator - b.numerator, denominator);

	// If denominators differ, calculate their LCM and divide that first to avoid overflow.
	T den = ccl_lcm (denominator, b.denominator);
	T num = numerator * (den / denominator) - b.numerator * (den / b.denominator);

	return Rational<T> (num, den);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

/** Multiplication operator. */
template <class T>
Rational<T> Rational<T>::operator * (const Rational<T>& b) const
{
	if(denominator == 0)
		return b;

	if(b.denominator == 0)
		return *this;

	return Rational<T> (numerator * b.numerator, denominator * b.denominator);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

/** Division operator. */
template <class T>
Rational<T> Rational<T>::operator / (const Rational<T>& b) const
{
	if(denominator == 0)
		return b;

	if(b.numerator == 0 || b.denominator == 0)
		return *this;

	return Rational<T> (numerator * b.denominator, denominator * b.numerator);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class T>
Rational<T>& Rational<T>::normalize ()
{
	T gcd = ccl_gcd (numerator, denominator);

	if(gcd > 0)
	{
		numerator /= gcd;
		denominator /= gcd;
	}
	return *this;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class T>
bool Rational<T>::denormalize (T newDenom)
{
	if(newDenom == denominator)
		return true;

	T denomTest = ccl_lcm (denominator, newDenom);
	if(denomTest != newDenom)
		return false;

	numerator *= (newDenom / denominator);
	denominator = newDenom;
	return true;
}

} // namespace Math
} // namespace CCL

#endif // _ccl_rational_h
