//************************************************************************************************
//
// This file is part of Crystal Class Library (R)
// Copyright (c) 2026 CCL Software Licensing GmbH.
// All Rights Reserved.
//
// Licensed for use under either:
//  1. a Commercial License provided by CCL Software Licensing GmbH, or
//  2. GNU Affero General Public License v3.0 (AGPLv3).
//
// You must choose and comply with one of the above licensing options.
// For more information, please visit ccl.dev.
//
// Filename    : ccl/app/paramcurve.h
// Description : Parameter Curve classes
//
//************************************************************************************************

#ifndef _ccl_paramcurve_h
#define _ccl_paramcurve_h

#include "ccl/base/object.h"

#include "ccl/public/gui/iparamcurve.h"
#include "ccl/public/collections/vector.h"
#include "ccl/public/text/cstring.h"

namespace Core {
class Interpolator; }

namespace CCL {

//************************************************************************************************
// ParamCurve
/** Base class for parameter curves. */
//************************************************************************************************

class ParamCurve: public Object,
				  public IParamCurve
{
public:
	DECLARE_CLASS_ABSTRACT (ParamCurve, Object)

	// IParamCurve
	double CCL_API getRelativeValue (double startValue, double endValue, double linearValue) const override;
	CStringPtr CCL_API getFactoryName () const override;

	CLASS_INTERFACE (IParamCurve, Object)
};

//************************************************************************************************
// ConcaveCurve
//************************************************************************************************

class ConcaveCurve: public ParamCurve
{
public:
	DECLARE_CLASS (ConcaveCurve, ParamCurve)

	// IParamCurve
	double CCL_API displayToNormalized (double linearValue) const override;
	double CCL_API normalizedToDisplay (double curveValue) const override;
};

//************************************************************************************************
// ConvexCurve
//************************************************************************************************

class ConvexCurve: public ParamCurve
{
public:
	DECLARE_CLASS (ConvexCurve, ParamCurve)

	// IParamCurve
	double CCL_API displayToNormalized (double linearValue) const override;
	double CCL_API normalizedToDisplay (double curveValue) const override;
};

//************************************************************************************************
// InterpolatorCurve
/** Wrapper for Core::Interpolator. */
//************************************************************************************************

class InterpolatorCurve: public ParamCurve
{
public:
	DECLARE_CLASS (InterpolatorCurve, ParamCurve)

	InterpolatorCurve (Core::Interpolator* interpolator = nullptr); ///< takes ownership!
	~InterpolatorCurve ();

	float getMinRange () const;
	float getMaxRange () const;
	float getMidRange () const;

	void setRange (float minRange, float maxRange, float midRange = 1.f);

	// ParamCurve
	double CCL_API displayToNormalized (double linearValue) const override;
	double CCL_API normalizedToDisplay (double curveValue) const override;
	double CCL_API getRelativeValue (double startValue, double endValue, double linearValue) const override;
	CStringPtr CCL_API getFactoryName () const override;

protected:
	Core::Interpolator* interpolator;
	Core::Interpolator* normalizer;

	void setInterpolator (Core::Interpolator* interpolator = nullptr);
};

//************************************************************************************************
// ParamCurveFactory
//************************************************************************************************

class ParamCurveFactory
{
public:
	static ParamCurveFactory& instance ();

	IParamCurve* create (StringID name);

	typedef IParamCurve* (*CreateFunc) ();
	void add (StringID name, CreateFunc createFunc);

	template <class Type>
	void addCurve (StringID name) { add (name, createCurve<Type>); }

protected:
	struct CurveClass
	{
		MutableCString name;
		CreateFunc createFunc;

		CurveClass (StringID name = nullptr, CreateFunc createFunc = nullptr)
		: name (name),
		  createFunc (createFunc)
		{}
	};

	Vector<CurveClass> classes;

	template <class Type>
	static IParamCurve* createCurve () { return NEW Type; }
};

} // namespace CCL

#endif // _ccl_paramcurve_h
