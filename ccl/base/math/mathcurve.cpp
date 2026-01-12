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
// Filename    : ccl/base/math/mathcurve.cpp
// Description : Curve class
//
//************************************************************************************************

#include "ccl/base/math/mathcurve.h"

#include "ccl/public/math/mathprimitives.h"

using namespace CCL;
using namespace Math;

//************************************************************************************************
// Math::LinearCurve
//************************************************************************************************

LinearCurve& LinearCurve::setPoints (double x1, double y1, double x2, double y2)
{
	double dy = y2 - y1;
	double dx = x2 - x1;
	k = dy / dx;
	d = y1 - k * x1;
	return *this;
}

//************************************************************************************************
// Math::LogarithmicCurve
//************************************************************************************************

double LogarithmicCurve::getY (double x) const
{
	return k * log (m * x + c) + d;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

LogarithmicCurve& LogarithmicCurve::setPoints (double x1, double y1, double x2, double y2)
{
	ASSERT (x1 != x2)
	ASSERT (y1 != y2)
	ASSERT (k != 0)
	double helper = exp ((y2 - y1) / k);

	c = (m * x2 - m* x1 * helper) / (helper - 1);
	d = y1 - k *  log (m * x1 + c);

	ASSERT (m * x1 + c > 0)
	return *this;
}

//************************************************************************************************
// Math::ExponentialCurve
//************************************************************************************************

double ExponentialCurve::getY (double x) const
{
	return k * exp (m * x) + d;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ExponentialCurve& ExponentialCurve::setPoints (double x1, double y1, double x2, double y2)
{
	ASSERT (x1 != x2)
	ASSERT (y1 != y2)
	double mx1 = m * x1;

	if(m == 0)
		k = (x2 == x1) ? 0 : (y2 - y1) / (x2 - x1);
	else
	{
		// prevent division by 0 (possible due to limited numerical precision, e.g. for very small m)
		double divisor = (exp(m * x2) - exp (mx1));
		if(divisor == 0)
			divisor = 1e-12;

		k = (y2 - y1) / divisor;
	}
	d = y1 - k * exp (mx1);
	return *this;
}

//************************************************************************************************
// Math::QuadraticCurve
//************************************************************************************************

double QuadraticCurve::getY (double x) const
{
	// should be faster than: a * pow (x, 2.) + b * x + c
	return c + x * (b + a * x);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL::Math::QuadraticCurve::Roots QuadraticCurve::getRoots (double y) const
{
	// implementation of the quadratic equation, returning the real solutions if they exist
	CCL::Math::QuadraticCurve::Roots r;

	double discriminant = (b * b) - (4 * a * (c - y));
	if(discriminant > 0)
	{
		r.numRoots = 2;
		r.root1 = (-b + sqrt (discriminant)) / (2 * a);
		r.root2 = (-b - sqrt (discriminant)) / (2 * a);
	}
	else if (discriminant == 0)
	{
		r.numRoots = 1;
		r.root1 = -b / (2 * a);
	}
	else
		r.numRoots = 0;

	return r;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

QuadraticCurve& QuadraticCurve::getDerivative (QuadraticCurve& derivative) const
{
	// 1st derivative: 2ax + b
	derivative.setQuadraticCoefficient (0.f);
	derivative.setLinearCoefficient (2. * a);
	derivative.setConstantCoefficient (b);
	return derivative;
}

//************************************************************************************************
// Math::CubicCurve
//************************************************************************************************

double CubicCurve::getY (double x) const
{
	// should be faster than: a * pow (x, 3.) + b * pow (x, 2.) + c * x + d
	return d + x * (c + x * (b + a * x));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

double CubicCurve::getRealX (double y) const
{
	// return real-value inversion (2 other complex solutions exist and may be real-valued)

	static const double one3rd = (1./3.);
	static const double two3rd = 2. * one3rd;
	static const double twoTo13rd = pow (2., one3rd);
	static const double twoTo23rd = twoTo13rd * twoTo13rd;
	const double aSq = a * a;
 	const double aSq27 = aSq * 27.;
	const double bSq = b * b;
	const double bCb = bSq * b;
 	const double ac = a * c;
  	const double abc = ac * b;
   	const double abc9 = abc * 9.;

	const double termP = (-2. * bCb + abc9 - aSq27 * d + sqrt (-4. * pow ((bSq - 3. * ac), 3.) + pow ((2. * bCb - abc9 + aSq27 * (d - y)), 2.)) + aSq27 * y);

	return (2. * twoTo13rd * bSq - 6 * twoTo13rd * ac - 2 * b * pow (termP, one3rd) + twoTo23rd * pow (termP, two3rd)) / (6. * a * pow (termP, one3rd));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CubicCurve& CubicCurve::getDerivative (CubicCurve& derivative) const
{
	// 1st derivative: 3ax^2 + 2bx + c
	derivative.setCubicCoefficient (0.f);
	derivative.setQuadraticCoefficient (3. * a);
	derivative.setLinearCoefficient (2. * b);
	derivative.setConstantCoefficient (c);
	return derivative;
}

//************************************************************************************************
// Math::CubicBezierCurve
//************************************************************************************************

CubicBezierCurve& CubicBezierCurve::assign (double p0, double p1, double p2, double p3)
{
	// Calculate polynomial coefficients
	// see http://www.algorithmist.net/bezier3.html ("Curve Evaluation")

	double a = 3. * p0;
	double b = 3. * p1;
	double c = 3. * p2;

	double c0 = p0;					// constant
	double c1 = b - a;				// linear
	double c2 = a - 2. * b + c;		// quadratic
	double c3 = p3 - p0 + b - c;	// cubic

	cubicCurve.setConstantCoefficient (c0);
	cubicCurve.setLinearCoefficient (c1);
	cubicCurve.setQuadraticCoefficient (c2);
	cubicCurve.setCubicCoefficient (c3);
	return *this;
}

//************************************************************************************************
// Math::CurveApproacher
//************************************************************************************************

CurveApproacher::CurveApproacher (const Curve& curve, const Curve& derivative, double yMin, double yMax)
: curve (curve),
  derivative (derivative),
  yMin (yMin),
  yMax (yMax)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

double CurveApproacher::getX (double y, double epsilon) const
{
	// Newton's method
	double t2 = y;
	for(int i = 0; i < 8; i++)
	{
		double y2 = curve.getY (t2) - y;
		if(fabs (y2) < epsilon)
			return t2;

		double d2 = derivative.getY (t2);
		if(fabs (d2) < 1e-6)
			break;

		t2 -= y2 / d2;
	}

	// Bisection method
	double t0 = yMin;
	double t1 = yMax;
	t2 = y;

	if(t2 < t0)
		return t0;
	if(t2 > t1)
		return t1;

	while(t0 < t1)
	{
		double y2 = curve.getY (t2);
		if(fabs (y2 - y) < epsilon)
			return t2;

		if(y > y2)
			t0 = t2;
		else
			t1 = t2;

		t2 = (t1 - t0) * .5 + t0;
	}

	return t2;
}

//************************************************************************************************
// Math::ScaledCurve
//************************************************************************************************

double ScaledCurve::getY (double x) const
{
	// normalize x from interval (x1, x2), call curve, scale normalized y to interval (y1, y2)
	return scaleY (curve.getY (normalizeX (x)));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ScaledCurve::scalePoint (Point& p)
{
	p.x = scaleX (p.x);
	p.y = scaleY (p.y);
}

//************************************************************************************************
// Math::CurveNormalizer
//************************************************************************************************

double CurveNormalizer::getY (double x) const
{
	// denormalize x from interval (x1, x2), call curve, and normalize y
	return normalizeY (curve.getY (scaleX (x)));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CurveNormalizer::normalizePoint (Point& p)
{
	p.x = normalizeX (p.x);
	p.y = normalizeY (p.y);
}

//************************************************************************************************
// Math::LinearSpline
//************************************************************************************************

LinearSpline::LinearSpline (SplineIterator* iter)
: iter (nullptr)
{
	setIterator (iter);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void LinearSpline::setIterator (SplineIterator* _iter)
{
	iter = _iter;
	if(iter)
		init ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void LinearSpline::init () const
{
	// prefetch the first point and setup the first interval ]-oo, p1.x[
	if(iter->splineNext (current.x2, current.y2))
	{
		current.x1 = - NumericLimits::kLargeDouble;
		current.y1 = current.y2;
	}
	else
	{
		// there is no point at all, 0 will always be delivered
		current.x1 = - NumericLimits::kLargeDouble;
		current.x2 =   NumericLimits::kLargeDouble;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

double LinearSpline::getY (double x) const
{
	ASSERT (iter != nullptr)

	if(x < current.x1)
	{
		iter->splineReset ();
		init ();
	}

	while(1)
	{
		if(x >= current.x1 && x < current.x2)
			return tempCurve.setPoints (current.x1, current.y1, current.x2, current.y2).getY (x);

		current.x1 = current.x2;
		current.y1 = current.y2;

		double nextX = 0., nextY = 0.;
		if(iter->splineNext (nextX, nextY))
		{
			current.x2 = nextX;
			current.y2 = nextY;
		}
		else
		{
			// no more points, setup last interval [lastPoint.x, +oo[
			current.x2 = NumericLimits::kLargeDouble;
			return current.y2;
		}
	}
	return 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool LinearSpline::isCurrentLastSegment () const
{
	return current.x2 == NumericLimits::kLargeDouble;
}

//************************************************************************************************
// Math::StepSpline
//************************************************************************************************

StepSpline::StepSpline (SplineIterator* iter)
: LinearSpline (iter)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

double StepSpline::getY (double x) const
{
	ASSERT (iter != nullptr)

	if(x < current.x1)
	{
		iter->splineReset ();
		init ();
	}

	while(1)
	{
		if(x >= current.x1 && x < current.x2)
			return current.y1;

		current.x1 = current.x2;
		current.y1 = current.y2;

		double nextX = 0., nextY = 0.;
		if(iter->splineNext (nextX, nextY))
		{
			current.x2 = nextX;
			current.y2 = nextY;
		}
		else
		{
			// no more points, setup last interval [lastPoint.x, +oo[
			current.x2 = NumericLimits::kLargeDouble;
			return current.y2;
		}
	}
	return 0;
}

//************************************************************************************************
// Math::NaturalSpline
//************************************************************************************************

NaturalSpline::NaturalSpline ()
: a (nullptr), b (nullptr), c (nullptr), d (nullptr), currentIndex (0)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline int NaturalSpline::getDegree () const
{
	return points.count () - 1;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline double NaturalSpline::xAt (int i) const
{
	return points [i].x;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline double NaturalSpline::yAt (int i) const
{
	return points [i].y;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool NaturalSpline::getLimits (double& minX, double& minY, double& maxX, double& maxY) const
{
	if(points.isEmpty ())
		return false;

	const Point& first = points.first ();
	minX = maxX = first.x;
	minY = maxY = first.y;
	for(int i = 1; i < points.count (); i++)
	{
		const Point& p = points[i];
		minX = ccl_min (minX, p.x);
		maxX = ccl_max (maxX, p.x);
		minY = ccl_min (minY, p.y);
		maxY = ccl_max (maxY, p.y);
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void NaturalSpline::reset ()
{
	points.empty ();
	coefficients.empty ();
	a = b = c = d = nullptr;
	currentIndex = 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void NaturalSpline::addPoint (const Point& p)
{
	if(points.isEmpty () == false)
	{
		if(p.x < points.first ().x)
		{
			points.insertAt (0, p);
			return;
		}
		if(p.x < points.last ().x)
		{
			for(int i = 0; i < points.count (); i++)
				if(p.x > points[i].x)
				{
					points.insertAt (i + 1, p);
					return;
				}
		}
	}

	points.add (p);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int NaturalSpline::countPoints () const
{
	return points.count ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const Point& NaturalSpline::getPoint (int index) const
{
	return points.at (index);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void NaturalSpline::setup (SplineIterator& iter)
{
	iter.splineReset ();
	reset ();
	double x,y;
	while(iter.splineNext (x,y))
		addPoint (Point (x,y));
	calculateCoefficients ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void NaturalSpline::calculateCoefficients ()
{
	int n = getDegree ();

	if(n > 1)
	{
		coefficients.resize (n * 4);
		coefficients.zeroFill ();
		a = coefficients.getItems ();
		b = coefficients.getItems () + n;
		c = coefficients.getItems () + (n * 2);
		d = coefficients.getItems () + (n * 3);

		if(n == 2)
		{
			// quadratic
			double h0 = xAt (1) - xAt (0);
			double h1 = xAt (2) - xAt (1);
			double d2 = yAt (2);

			d[0] = yAt (0);
			d[1] = yAt (1);
			b[0] = 0;
			b[1] = (((3./h1) * (d2-d[1])) - ((3./h0) * (d[1]-d[0]))) / (2.* (xAt (2)-xAt (0)));
			c[0] = ((1./h0) * (d[1]-d[0])) - ((h0/3.) * b[1]);
			c[1] = ((1./h1) * (d2-d[1])) - ((h1/3.) * (2. * b[1]));
			a[0] = (1./ (3.*h0)) * b[1];
			a[1] = (1./ (3.*h1)) * (-b[1]);
		}
		else
		{
			// cubic
			Vector<double> data (n * 7);

			double* dataPtr = data.getItems ();
			double* f2 = dataPtr;  dataPtr += n+1;
			double* alpha = dataPtr; dataPtr += n;
			double* beta = dataPtr; dataPtr += n-1;
			double* m = dataPtr; dataPtr += n;
			double* l = dataPtr; dataPtr += n-1;
			double* h = dataPtr; dataPtr += n;
			double* y = dataPtr;

			for(int i = 0; i < n; i++)
				h[i] = xAt(i+1) - xAt(i);

			// 2nd derivations
			f2[0] = f2[n] = 0.0;

			// setup system of equations
			for(int i = 1; i < n-1; i++)
			{
				beta[i] = h[i];
				alpha[i] = 2.0 * (h[i-1] + h[i]);
			}
			alpha[n-1] = 2.0 * (h[n-2] + h[n-1]);

			// calcultate system of equations
			m[1] = alpha[1];
			for(int i = 1; i < n-1; i++)
			{
				l[i] = beta[i] / m[i];
				m[i+1] = alpha[i+1] - (l[i] * beta[i]);
			}

			y[1] = ((6.0 / h[1]) * (yAt (2) - yAt(1))) - ((6.0 / h[0]) * (yAt(1) - yAt(0)));

			for(int i = 2; i < n; i++)
			{
				double b = ((6.0 / h[i]) * (yAt(i+1) - yAt(i))) - ((6.0 / h[i-1]) * (yAt(i) - yAt(i-1)));
				y[i] = b - (l[i-1] * y[i-1]);
			}

			// 2nd derivations ready
			f2[n-1] = y[n-1] / m[n-1];
			for(int i = n-2; i > 0; i--)
				f2[i] = (y[i] - (beta[i] * f2[i+1])) / m[i];

			// calculate coefficients
			for(int i = 0; i < n; i++)
			{
				a[i] = (1. / (6. * h[i])) * (f2[i+1] - f2[i]);
				b[i] = 0.5 * f2[i];
				c[i] = ((1.0 / h[i]) * (yAt(i+1) - yAt(i))) - ((h[i] / 6.0) * (f2[i+1] + (2.0 * f2[i])));
				d[i] = yAt(i);
			}
		}
	}
	else
	{
		coefficients.resize (0);
		a = b = c = d = nullptr;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

double NaturalSpline::getY (double x) const
{
	int degree = getDegree ();
	if(a)
	{
		if(x < xAt (currentIndex))
			currentIndex = 0;

		while(currentIndex < (degree - 1) && x >= xAt (currentIndex + 1))
			currentIndex++;

		double x2 = x - xAt (currentIndex);
		return d[currentIndex] + (c[currentIndex] + (b[currentIndex] + (a[currentIndex] * x2)) * x2) * x2;
	}
	else if(degree == 1)
		return LinearCurve ().setPoints (xAt(0), yAt(0), xAt (1), yAt (1)).getY (x);

	return x;
}







