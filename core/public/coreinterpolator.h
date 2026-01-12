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
// Filename    : core/public/coreinterpolator.h
// Description : Interpolator class
//
//************************************************************************************************

#ifndef _coreinterpolator_h
#define _coreinterpolator_h

#include "core/public/coretypes.h"

namespace Core {

class Interpolator;

//************************************************************************************************
// InterpolatorFactory
/**	Interpolator class registration.
	\ingroup core_interpolator */
//************************************************************************************************

class InterpolatorFactory
{
public:
	typedef Interpolator* (*CreateFunc) ();

	/** Create interpolator by name. */
	static Interpolator* create (CStringPtr name);

	/** Register interpolator class. */
	static void add (CStringPtr name, CreateFunc createFunc);
	
	/** Register interpolator class. */
	template <class Type>
	static void add (CStringPtr name) { add (name, createInterpolator<Type>); }

	/** Interpolator create function template. */
	template <class Type>
	static Interpolator* createInterpolator () { return NEW Type; }
};

//************************************************************************************************
// Interpolator
/**	Base class for conversion between value range and normalized domain.
	\ingroup core_interpolator */
//************************************************************************************************

class Interpolator
{
public:
	Interpolator (float minRange = 0.f, float maxRange = 1.f, float midRange = 1.f, CStringPtr name = nullptr)
 	: minRange (minRange),
	  maxRange (maxRange),
	  midRange (midRange),
	  name (name)
	{}
	
	virtual ~Interpolator ()
	{}

	CStringPtr getName () const		{ return name; }
	void setName (CStringPtr _name)	{ name = _name; }

	float getMinRange () const { return minRange; }	
	float getMaxRange () const { return maxRange; }
	float getMidRange () const { return midRange; }

	/** Set value range [min..max]. (middle value optional) */
	virtual void setRange (float minRange, float maxRange, float midRange = 1.f)
	{
		this->minRange = minRange;
		this->maxRange = maxRange;
		this->midRange = midRange;
	}

	/** Convert from normalized domain [0..1] to value range [min..max]. */
	virtual float normalizedToRange (float normalized) const { return normalized; }
	
	/** Convert from value range [min..max] to normalized domain [0..1]. */
	virtual float rangeToNormalized (float value) const { return value; }
	
protected:
	float minRange;
	float maxRange;
	float midRange;
	CStringPtr name;
};

//************************************************************************************************
// LinearInterpolator
/**	Linear conversion between value range and normalized domain.
	\ingroup core_interpolator */
//************************************************************************************************

class LinearInterpolator: public Interpolator
{
public:
	LinearInterpolator (float minRange = 0, float maxRange = 1);

	// Interpolator
	float normalizedToRange (float normalized) const override;
	float rangeToNormalized (float value) const override;
};

//************************************************************************************************
// LinearReverseInterpolator
/**	Linear conversion between value range and normalized domain.
	Parameter is reversed.
	\ingroup core_interpolator */
//************************************************************************************************

class LinearReverseInterpolator: public Interpolator
{
public:
	LinearReverseInterpolator (float minRange = 0, float maxRange = 1);

	// Interpolator
	float normalizedToRange (float normalized) const override;
	float rangeToNormalized (float value) const override;
};

//************************************************************************************************
// SegmentInterpolator
/**	Base Class for multiple segment linear interpolators
	\ingroup core_interpolator */
//************************************************************************************************

class SegmentInterpolator: public Interpolator
{
public:
	struct BreakPoint 
	{
		float normalized;
		float range;
	};

	/** First and last points need to be minRange / maxRange. */
	SegmentInterpolator (int numPoints, const BreakPoint points[]);

	// Interpolator
	void setRange (float minRange, float maxRange, float midRange = 1.f) override;
	float normalizedToRange (float normalized) const override;
	float rangeToNormalized (float value) const override;

protected:
	const BreakPoint* breakPoints;
	int numPoints;
};

//************************************************************************************************
// ZoomInterpolator
/**	Zooms around 0 (adjust curvature of powercurve, midRange is power, not midrange)
	\ingroup core_interpolator  */
//************************************************************************************************

class ZoomInterpolator: public Interpolator
{
public:
	ZoomInterpolator (float minRange = 0, float maxRange = 1, float midRange = 1);

	// Interpolator
	void setRange (float minRange, float maxRange, float midRange = 1.f) override;
	float normalizedToRange (float normalized) const override;
	float rangeToNormalized (float value) const override;

protected:
	float midRangeInv;
};

} // namespace Core

#endif // _coreinterpolator_h

