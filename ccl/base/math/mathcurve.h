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
// Filename    : ccl/base/math/mathcurve.h
// Description : Curve class
//
//************************************************************************************************

#ifndef _ccl_mathcurve_h
#define _ccl_mathcurve_h

#include "ccl/base/object.h"
#include "ccl/base/math/mathpoint.h"

#include "ccl/public/collections/vector.h"

namespace CCL {
namespace Math {

//************************************************************************************************
// Math::Curve
/** Curve base class. */
//************************************************************************************************

class Curve: public Object
{
public:
	virtual ~Curve () {}

	virtual double getY (double x) const = 0;	///< calculate y for x
};

//************************************************************************************************
// Math::LinearCurve
/** Linear curve defined by slope and offset (y = k * x + d). */
//************************************************************************************************

class LinearCurve: public Curve
{
public:
	LinearCurve (double k = 1., double d = 0.)
	: k (k), d (d)
	{}

	PROPERTY_VARIABLE (double, k, K)	///< slope
	PROPERTY_VARIABLE (double, d, D)	///< offset

	/** Define curve by two known points. */
	LinearCurve& setPoints (double x1, double y1, double x2, double y2);

	// Curve
	double getY (double x) const override { return k * x + d; }
};

//************************************************************************************************
// Math::LogarithmicCurve
/** General logarithmic curve defined as: y = k * ln (m * x + c) + d. */
//************************************************************************************************

class LogarithmicCurve: public Curve
{
public:
	LogarithmicCurve (double k = 1., double m = 1., double c = 0., double d = 0.)
	: k (k), m (m), c (c), d (d)
	{}

	PROPERTY_VARIABLE (double, k, YScaler)	///< y-scaling
	PROPERTY_VARIABLE (double, m, XScaler)	///< x-scaling
	PROPERTY_VARIABLE (double, c, XOffset)	///< x-offset
	PROPERTY_VARIABLE (double, d, YOffset)	///< y-offset

	/** Translate the curve to go through two known points. Only changes c and d. */
	LogarithmicCurve& setPoints (double x1, double y1, double x2, double y2);

	// Curve
	double getY (double x) const override;
};

//************************************************************************************************
// Math::ExponentialCurve
/** General exponential curve defined as: y = k * exp (m * x) + d. */
//************************************************************************************************

class ExponentialCurve: public Curve
{
public:
	ExponentialCurve (double k = 1., double m = 1., double d = 0.)
	: k (k), m (m), d (d)
	{}

	PROPERTY_VARIABLE (double, k, YScaler)	///< y-scaling
	PROPERTY_VARIABLE (double, m, XScaler)	///< x-scaling
	PROPERTY_VARIABLE (double, d, YOffset)	///< y-offset

	/** Translate the curve to go through two known points. Changes k and d for a given m. */
	ExponentialCurve& setPoints (double x1, double y1, double x2, double y2);

	// Curve
	double getY (double x) const override;
};

//************************************************************************************************
// Math::QuadraticCurve
/** Quadratic polynomial defined as: y = a*x^2 + b*x + c */
//************************************************************************************************

class QuadraticCurve: public Curve
{
public:
	QuadraticCurve (double a = 0., double b = 1., double c = 0.) ///< defaults to linear
	: a (a), b (b), c (c)
	{}

	PROPERTY_VARIABLE (double, a, QuadraticCoefficient)
	PROPERTY_VARIABLE (double, b, LinearCoefficient)
	PROPERTY_VARIABLE (double, c, ConstantCoefficient)

	QuadraticCurve& getDerivative (QuadraticCurve& derivative) const;

	struct Roots
	{
		int numRoots;
		double root1;
		double root2;
	};

	Roots getRoots (double y) const;  ///< returns the real results of the quadratic formula. There may be 0, 1, or 2 solutions.

	// Curve
	double getY (double x) const override;
};

//************************************************************************************************
// Math::CubicCurve
/** Cubic polynomial defined as: y = a*x^3 + b*x^2 + c*x + d */
//************************************************************************************************

class CubicCurve: public Curve
{
public:
	CubicCurve (double a = 0., double b = 0., double c = 1., double d = 0.) ///< defaults to linear
	: a (a), b (b), c (c), d (d)
	{}

	PROPERTY_VARIABLE (double, a, CubicCoefficient)
	PROPERTY_VARIABLE (double, b, QuadraticCoefficient)
	PROPERTY_VARIABLE (double, c, LinearCoefficient)
	PROPERTY_VARIABLE (double, d, ConstantCoefficient)

	CubicCurve& getDerivative (CubicCurve& derivative) const;
	double getRealX (double y) const; ///< returns real-value inversion (2 other solutions may exist)

	// Curve
	double getY (double x) const override;
};

//************************************************************************************************
// Math::CubicBezierCurve
/** Cubic bezier curve defined by anchor points PO and P3 and control points P1 and P2. */
//************************************************************************************************

class CubicBezierCurve: public Curve
{
public:
	CubicBezierCurve (double p0 = 0., double p1 = 0., double p2 = 1., double p3 = 1.) ///< defaults to linear
	{ assign (p0, p1, p2, p3); }

	CubicBezierCurve& assign (double p0, double p1, double p2, double p3);

	double getX (double y) const { return cubicCurve.getRealX (y); }
	CubicCurve& getDerivative (CubicCurve& derivative) const { return cubicCurve.getDerivative (derivative);  }

	// Curve
	double getY (double x) const override { return cubicCurve.getY (x); }

protected:
	CubicCurve cubicCurve;
};

//************************************************************************************************
// Math::CurveApproacher
/** Iterative approach to calculate x from y=f(x). */
//************************************************************************************************

class CurveApproacher
{
public:
	CurveApproacher (const Curve& curve, const Curve& derivative, double yMin = 0., double yMax = 1.);

	PROPERTY_VARIABLE (double, yMin, YMin)
	PROPERTY_VARIABLE (double, yMax, YMax)

	double getX (double y, double epsilon) const;

protected:
	const Curve& curve;
	const Curve& derivative;
};

//************************************************************************************************
// ScaledCurve
//************************************************************************************************

class ScaledCurve: public Curve
{
public:
	ScaledCurve (Curve& normalizedCurve, double x1, double y1, double x2, double y2);

	void scalePoint (Point& p);

	// Curve
	double getY (double x) const override;

protected:
	Curve& curve;
	double x1;
	double y1;
	double w;
	double h;

	double scaleX (double normalizedX) const { return (normalizedX * w) + x1; }
	double scaleY (double normalizedY) const { return (normalizedY * h) + y1; }
	double normalizeX (double scaledX) const { return w == 0 ? 0 : (scaledX - x1) / w; }
	double normalizeY (double scaledY) const { return h == 0 ? 0 : (scaledY - y1) / h; }
};

//************************************************************************************************
// CurveNormalizer
//************************************************************************************************

class CurveNormalizer: public ScaledCurve
{
public:
	CurveNormalizer (Curve& curve, double x1, double y1, double x2, double y2)
	: ScaledCurve (curve, x1,y1,x2,y2) { if(h == 0) h = 1;}

	void normalizePoint (Point& p);

	// Curve
	double getY (double x) const override;
};

//************************************************************************************************
// Math::SplineIterator
/** Spline iterator. */
//************************************************************************************************

class SplineIterator
{
public:
	virtual ~SplineIterator () {}

	virtual void splineReset () = 0;
	virtual bool splineNext (double& x, double& y) = 0;
};

//************************************************************************************************
// Math::LinearSpline
/** Spline curve with linear interpolation. */
//************************************************************************************************

class LinearSpline: public Curve
{
public:
	LinearSpline (SplineIterator* iter = nullptr);

	SplineIterator* getIterator () const;
	void setIterator (SplineIterator* iter);

	// Curve
	double getY (double x) const override;

	double getCurrentSegmentStart () const;
	double getCurrentSegmentEnd () const;
	double getCurrentSegmentStartY () const;
	double getCurrentSegmentEndY () const;
	bool isCurrentLastSegment () const;
	virtual bool isCurrentSegmentConstant (double fault = 0.0) const;

protected:
	void init () const;

	struct Segment
	{
		double x1;
		double y1;
		double x2;
		double y2;

		Segment ()
		: x1 (0.), y1 (0.), x2 (0.), y2 (0.)
		{}
	};

	SplineIterator* iter;
	mutable Segment current;
	mutable LinearCurve tempCurve;
};

//************************************************************************************************
// Math::StepSpline
/** Spline curve composed of horizontal lines between the points.
	Delivers the y value of the nearest point <= x. */
//************************************************************************************************

class StepSpline: public LinearSpline
{
public:
	StepSpline (SplineIterator* iter = nullptr);

	// Curve
	double getY (double x) const override;

	// LinearSpline
	bool isCurrentSegmentConstant (double fault = 0.0) const override {return true;}
};

//************************************************************************************************
// Math::NaturalSpline
/** Natural Spline curve. Degeree is defined by number of setup points (1: linear, 2: quadratic, >=3: cubic)
	Delivers the y value of the nearest point <= x. */
//************************************************************************************************

class NaturalSpline: public Curve
{
public:
	NaturalSpline ();

	void reset ();
	void addPoint (const Point& p);
	void calculateCoefficients ();
	void setup (SplineIterator& iter);

	int countPoints () const;
	const Point& getPoint (int index) const;
	bool getLimits (double& minX, double& minY, double& maxX, double& maxY) const;

	// Curve
	double getY (double x) const override;

protected:
	Vector<Point> points;
	Vector<double> coefficients;
	double* a;
	double* b;
	double* c;
	double* d;
	mutable int currentIndex;

	int getDegree () const;
	double xAt (int i) const;
	double yAt (int i) const;
};


//////////////////////////////////////////////////////////////////////////////////////////////////
// inline
//////////////////////////////////////////////////////////////////////////////////////////////////

inline ScaledCurve::ScaledCurve (Curve& normalizedCurve, double x1, double y1, double x2, double y2)
: curve (normalizedCurve), x1 (x1), y1 (y1), w (x2 - x1), h (y2 - y1)
{}

inline SplineIterator* LinearSpline::getIterator () const
{ return iter; }

inline double LinearSpline::getCurrentSegmentStart () const
{ return current.x1; }

inline double LinearSpline::getCurrentSegmentEnd () const
{ return current.x2; }

inline double LinearSpline::getCurrentSegmentStartY () const
{ return current.y1; }

inline double LinearSpline::getCurrentSegmentEndY () const
{ return current.y2; }

inline bool LinearSpline::isCurrentSegmentConstant (double fault) const
{ return ccl_abs (current.y1 - current.y2) <= fault; }

//////////////////////////////////////////////////////////////////////////////////////////////////

} // namespace Math
} // namespace CCL

#endif // _ccl_mathcurve_h
