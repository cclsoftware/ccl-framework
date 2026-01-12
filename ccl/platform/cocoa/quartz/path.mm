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
// Filename    : ccl/platform/cocoa/quartz/path.cpp
// Description : Quartz Path
//
//************************************************************************************************

#include "ccl/platform/cocoa/quartz/path.h"
#include "ccl/platform/cocoa/quartz/device.h"
#include "ccl/platform/cocoa/quartz/cghelper.h"

#include "ccl/public/math/mathprimitives.h"

using namespace CCL;
using namespace MacOS;

//************************************************************************************************
// QuartzPath
//************************************************************************************************

DEFINE_CLASS_HIDDEN (QuartzPath, NativeGraphicsPath)

//////////////////////////////////////////////////////////////////////////////////////////////////

QuartzPath::QuartzPath ()
: path (::CGPathCreateMutable ()), started (false), fillMode (kCGPathFill)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

QuartzPath::QuartzPath (const QuartzPath& other)
: path (::CGPathCreateMutableCopy (other.path)), started (true), fillMode (other.fillMode)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

QuartzPath::~QuartzPath ()
{
	if(path)
		::CGPathRelease (path);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult QuartzPath::draw (NativeGraphicsDevice& device, PenRef pen)
{
	QuartzDevice* qDevice = ccl_cast<QuartzDevice> (&device);
	if(!qDevice)
		return kResultUnexpected;

	qDevice->getState ().setPen (pen);
	AntiAliasSetter smoother (*qDevice); // enable anti-aliasing
	CGContextRef context = qDevice->getTarget ().getContext ();

	::CGContextBeginPath (context);
	::CGContextAddPath (context, path);
	::CGContextDrawPath (context, kCGPathStroke);

	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult QuartzPath::fill (NativeGraphicsDevice& device, BrushRef brush)
{
	QuartzDevice* qDevice = ccl_cast<QuartzDevice> (&device);
	if(!qDevice)
		return kResultUnexpected;

	qDevice->getState ().setBrush (brush);
	AntiAliasSetter smoother (*qDevice); // enable anti-aliasing
	CGContextRef context = qDevice->getTarget ().getContext ();

	::CGContextBeginPath (context);
	::CGContextAddPath (context, path);
	::CGContextClosePath (context);
	::CGContextDrawPath (context, fillMode);

	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API QuartzPath::getBounds (Rect& bounds) const
{
	CGRect rect = ::CGPathGetBoundingBox (path);
	fromCGRect (bounds, rect);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API QuartzPath::getBounds (RectF& bounds) const
{
	CGRect rect = ::CGPathGetBoundingBox (path);
	fromCGRect (bounds, rect);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API QuartzPath::transform (TransformRef t)
{
	::CGAffineTransform tx = {t.a0, t.a1, t.b0, t.b1, t.t0, t.t1};

	::CGMutablePathRef pathCopy = ::CGPathCreateMutable ();
	::CGPathAddPath (pathCopy, &tx, path);

	::CGPathRelease (path);
	path = pathCopy;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API QuartzPath::closeFigure ()
{
	::CGPathCloseSubpath (path);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API QuartzPath::startFigure (PointFRef p)
{
	::CGPathMoveToPoint (path, 0, p.x, p.y);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API QuartzPath::lineTo (PointRef p)
{
	::CGPathAddLineToPoint (path, 0, p.x, p.y);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API QuartzPath::lineTo (PointFRef p)
{
	::CGPathAddLineToPoint (path, 0, p.x, p.y);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API QuartzPath::addRect (RectRef rect)
{
	CGRect rect2;
	toCGRect (rect2, rect);
	::CGPathAddRect (path, 0, rect2);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API QuartzPath::addRect (RectFRef rect)
{
	CGRect rect2;
	toCGRect (rect2, rect);
	::CGPathAddRect (path, 0, rect2);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API QuartzPath::addRoundRect (RectRef _rect, Coord rx, Coord ry)
{
	// check for simple rectangle...
	if(rx == 0 && ry == 0)
	{
		addRect (_rect);
		return;
	}

	CGRect rect2;
	toCGRect (rect2, _rect);
	rect2.origin.y += 0.5f;
	rect2.origin.x += 0.5f;
	rect2.size.height -= 1;
	rect2.size.width -= 1;

	::CGPathMoveToPoint (path, 0, rect2.origin.x + rx, rect2.origin.y);

	::CGPathAddLineToPoint (path, 0, rect2.origin.x + rect2.size.width - rx, rect2.origin.y);
	::CGPathAddArcToPoint (path, 0, rect2.origin.x + rect2.size.width, rect2.origin.y, rect2.origin.x + rect2.size.width, rect2.origin.y + ry, rx);

	::CGPathAddLineToPoint (path, 0, rect2.origin.x + rect2.size.width, rect2.origin.y + rect2.size.height - ry);
	::CGPathAddArcToPoint (path, 0, rect2.origin.x + rect2.size.width, rect2.origin.y + rect2.size.height, rect2.origin.x + rect2.size.width - rx, rect2.origin.y + rect2.size.height, rx);

	::CGPathAddLineToPoint (path, 0, rect2.origin.x + rx, rect2.origin.y + rect2.size.height);
	::CGPathAddArcToPoint (path, 0, rect2.origin.x, rect2.origin.y + rect2.size.height, rect2.origin.x, rect2.origin.y + rect2.size.height - ry, rx);

	::CGPathAddLineToPoint (path, 0, rect2.origin.x, rect2.origin.y + ry);
	::CGPathAddArcToPoint (path, 0, rect2.origin.x, rect2.origin.y, rect2.origin.x + rx, rect2.origin.y, rx);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API QuartzPath::addRoundRect (RectFRef _rect, CoordF rx, CoordF ry)
{
	// check for simple rectangle...
	if(rx == 0 && ry == 0)
	{
		addRect (_rect);
		return;
	}

	CGRect rect2;
	toCGRect (rect2, _rect);
	rect2.origin.y += 0.5f;
	rect2.origin.x += 0.5f;
	rect2.size.height -= 1;
	rect2.size.width -= 1;

	::CGPathMoveToPoint (path, 0, rect2.origin.x + rx, rect2.origin.y);

	::CGPathAddLineToPoint (path, 0, rect2.origin.x + rect2.size.width - rx, rect2.origin.y);
	::CGPathAddArcToPoint (path, 0, rect2.origin.x + rect2.size.width, rect2.origin.y, rect2.origin.x + rect2.size.width, rect2.origin.y + ry, rx);

	::CGPathAddLineToPoint (path, 0, rect2.origin.x + rect2.size.width, rect2.origin.y + rect2.size.height - ry);
	::CGPathAddArcToPoint (path, 0, rect2.origin.x + rect2.size.width, rect2.origin.y + rect2.size.height, rect2.origin.x + rect2.size.width - rx, rect2.origin.y + rect2.size.height, rx);

	::CGPathAddLineToPoint (path, 0, rect2.origin.x + rx, rect2.origin.y + rect2.size.height);
	::CGPathAddArcToPoint (path, 0, rect2.origin.x, rect2.origin.y + rect2.size.height, rect2.origin.x, rect2.origin.y + rect2.size.height - ry, rx);

	::CGPathAddLineToPoint (path, 0, rect2.origin.x, rect2.origin.y + ry);
	::CGPathAddArcToPoint (path, 0, rect2.origin.x, rect2.origin.y, rect2.origin.x + rx, rect2.origin.y, rx);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API QuartzPath::addBezier (PointRef p1, PointRef c1, PointRef c2, PointRef p2)
{
	CGPoint p = {0,0};
	if(!started)
		started = true;
	else
		p = ::CGPathGetCurrentPoint (path);
	if(p.x != p1.x || p.y != p1.y)
		::CGPathMoveToPoint (path, 0, p1.x, p1.y);

	::CGPathAddCurveToPoint (path, 0, c1.x, c1.y, c2.x, c2.y, p2.x, p2.y);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API QuartzPath::addBezier (PointFRef p1, PointFRef c1, PointFRef c2, PointFRef p2)
{
	CGPoint p = {0,0};
	if(!started)
		started = true;
	else
		p = ::CGPathGetCurrentPoint (path);
	if(p.x != p1.x || p.y != p1.y)
		::CGPathMoveToPoint (path, 0, p1.x, p1.y);

	::CGPathAddCurveToPoint (path, 0, c1.x, c1.y, c2.x, c2.y, p2.x, p2.y);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API QuartzPath::addArc (RectRef r, float startAngle, float sweepAngle)
{
    return addArc (rectIntToF (r), startAngle, sweepAngle);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API QuartzPath::addArc (RectFRef r, float _startAngle, float _sweepAngle)
{
    PointF center (r.getCenter ());
    CoordF width = r.getWidth ();
    CoordF height = r.getHeight ();
    CoordF rx = width / 2.f;

	if(_sweepAngle >= 360.)
	{
		::CGPathMoveToPoint (path, 0, center.x + rx, center.y);
		::CGPathAddArc (path, 0, center.x, center.y, rx, (CGFloat)(2 * M_PI), 0, true);
		return;
	}

	float startAngle = _startAngle;
	float endAngle = startAngle + _sweepAngle;

	if(startAngle >= 360.)
		startAngle -= 360.f;
	if(endAngle >= 360.)
		endAngle -= 360.f;

    bool clockWise = _sweepAngle < 0;

	float startRad = Math::degreesToRad (startAngle);
	float endRad = Math::degreesToRad (endAngle);

	//::CGPathMoveToPoint (path, 0, center.x + rx * cos (startRad), center.y + rx * sin (startRad)); // commented out as hotfix for wrong arcs in svgs
    if(width == height)
        ::CGPathAddArc (path, 0, center.x, center.y, rx, startRad, endRad, clockWise);
    else if(width != 0)
    {
        // CGPathAddArc has only one radius argument for both directions (circular arc).
        // to get the desired ellipse, we stretch the circle by providing a transform matrix
        CGAffineTransform t = CGAffineTransformMakeTranslation (center.x, center.y);
        t = CGAffineTransformConcat (CGAffineTransformMakeScale (1.0, height / width), t);

        ::CGPathAddArc (path, &t, 0, 0, rx, startRad, endRad, clockWise);
    }
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API QuartzPath::setFillMode (FillMode mode)
{
	fillMode = (mode == kFillNonZero) ? kCGPathFill : kCGPathEOFill;
}
