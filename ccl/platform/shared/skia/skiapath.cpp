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
// Filename    : ccl/platform/shared/skia/skiapath.cpp
// Description : Skia Path
//
//************************************************************************************************

#include "ccl/platform/shared/skia/skiapath.h"

#include "ccl/platform/shared/skia/skiadevice.h"

#include "ccl/public/math/mathprimitives.h"

using namespace CCL;

//************************************************************************************************
// SkiaPath
//************************************************************************************************

DEFINE_CLASS_HIDDEN (SkiaPath, NativeGraphicsPath)

//////////////////////////////////////////////////////////////////////////////////////////////////

SkiaPath::SkiaPath ()
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

SkiaPath::SkiaPath (const SkiaPath& other)
: path (SkPath (other.getSkPath ()))
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

SkiaPath::~SkiaPath ()
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult SkiaPath::draw (NativeGraphicsDevice& device, PenRef pen)
{
	SkiaDevice* sDevice = ccl_cast<SkiaDevice> (&device);
	if(!sDevice)
		return kResultUnexpected;

	sDevice->getState ().setPen (pen);
	AntiAliasSetter smoother (*sDevice); // enable anti-aliasing
	SkPaint paint = sDevice->getState ().getPaint ();
	paint.setStyle (SkPaint::kStroke_Style);

	SkCanvas* canvas = sDevice->getCanvas ();
	if(!canvas)
		return kResultUnexpected;

	canvas->drawPath (path, paint);
	
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult SkiaPath::fill (NativeGraphicsDevice& device, BrushRef brush)
{
	SkiaDevice* sDevice = ccl_cast<SkiaDevice> (&device);
	if(!sDevice)
		return kResultUnexpected;

	sDevice->getState ().setBrush (brush);
	AntiAliasSetter smoother (*sDevice); // enable anti-aliasing
	SkPaint paint = sDevice->getState ().getPaint ();
	paint.setStyle (SkPaint::kFill_Style);
		
	SkCanvas* canvas = sDevice->getCanvas ();
	if(!canvas)
		return kResultUnexpected;

	canvas->drawPath (path, paint);
	
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API SkiaPath::getBounds (Rect& bounds) const
{
	SkiaDevice::fromSkRect (bounds, path.computeTightBounds ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API SkiaPath::getBounds (RectF& bounds) const
{
	SkiaDevice::fromSkRect (bounds, path.computeTightBounds ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API SkiaPath::transform (TransformRef t)
{
	path.transform (SkMatrix::MakeAll (t.a0, t.b0, t.t0, t.a1, t.b1, t.t1 , 0, 0, 1));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API SkiaPath::closeFigure ()
{
	path.close ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API SkiaPath::startFigure (PointFRef p)
{
	path.moveTo (p.x, p.y);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API SkiaPath::lineTo (PointRef p)
{
	lineTo (pointIntToF (p));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API SkiaPath::lineTo (PointFRef p)
{
	path.lineTo (p.x, p.y);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API SkiaPath::addRect (RectRef rect)
{
	addRect (rectIntToF (rect));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API SkiaPath::addRect (RectFRef rect)
{
	SkRect skRect;
	SkiaDevice::toSkRect (skRect, rect);
	path.addRect (skRect);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API SkiaPath::addRoundRect (RectRef rect, Coord rx, Coord ry)
{
	// check for simple rectangle...
	if(rx == 0 && ry == 0)
	{
		addRect (rect);
		return;
	}
	
	addRoundRect (rectIntToF (rect), (CoordF)rx, (CoordF)ry);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API SkiaPath::addRoundRect (RectFRef rect, CoordF rx, CoordF ry)
{
	SkRect skRect;
	SkiaDevice::toSkRect (skRect, rect);
	skRect.inset (0.5f, 0.5f);
	path.addRoundRect (skRect, rx, ry);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API SkiaPath::addBezier (PointRef p1, PointRef c1, PointRef c2, PointRef p2)
{
	addBezier (pointIntToF (p1), pointIntToF (c1), pointIntToF (c2), pointIntToF (p2));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API SkiaPath::addBezier (PointFRef p1, PointFRef c1, PointFRef c2, PointFRef p2)
{
	SkPoint sP1;
	SkiaDevice::toSkPoint (sP1, p1);
	SkPoint lastPoint;
	path.getLastPt (&lastPoint);
	if(lastPoint != sP1)
		path.moveTo (sP1);
		
	SkPoint sC1;
	SkPoint sC2;
	SkPoint sP2;
	path.cubicTo (SkiaDevice::toSkPoint (sC1, c1), SkiaDevice::toSkPoint (sC2, c2), SkiaDevice::toSkPoint (sP2, p2));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API SkiaPath::addArc (RectRef r, float startAngle, float sweepAngle)
{
	addArc (rectIntToF (r), startAngle, sweepAngle);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API SkiaPath::addArc (RectFRef r, float startAngle, float sweepAngle)
{
	// without this adjustment, a full circle arc would be drawn as a line
	if(sweepAngle >= 360.)
	{
		sweepAngle = 359.99995f;
		startAngle = 0.f;
	}

	SkRect rect;
	path.arcTo (SkiaDevice::toSkRect (rect, r), startAngle, sweepAngle, false);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API SkiaPath::setFillMode (FillMode mode)
{
	SkPathFillType filltype = SkPathFillType::kWinding;
	
	switch (mode)
	{
	case IGraphicsPath::kFillNonZero:
		filltype = SkPathFillType::kWinding;
		break;
	case IGraphicsPath::kFillEvenOdd:
		filltype = SkPathFillType::kEvenOdd;
		break;
	}
	
	path.setFillType (filltype);
}
