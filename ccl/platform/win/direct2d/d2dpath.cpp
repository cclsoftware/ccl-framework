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
// Filename    : ccl/platform/win/direct2d/d2dpath.cpp
// Description : Direct2D Path
//
//************************************************************************************************

#include "ccl/platform/win/direct2d/d2dpath.h"
#include "ccl/platform/win/direct2d/d2ddevice.h"
#include "ccl/platform/win/direct2d/dxgiengine.h"
#include "ccl/public/math/mathprimitives.h"

using namespace CCL;
using namespace Win32;

//************************************************************************************************
// D2DPathGeometry
//************************************************************************************************

DEFINE_CLASS_HIDDEN (D2DPathGeometry, NativeGraphicsPath)

//////////////////////////////////////////////////////////////////////////////////////////////////

D2DPathGeometry::D2DPathGeometry (TypeHint type)
: type (type),
  wasOpen (false),
  figureStarted (false),
  rebuildNeeded (false),
  fillMode (D2D1_FILL_MODE_WINDING) // (this is the default on other platforms and in SVG)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

D2DPathGeometry::~D2DPathGeometry ()
{
	closeSink ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void D2DPathGeometry::discardDirect2dResource (bool isShutdown)
{
	if(!pathGeometry)
		return;

	closeSink ();

	if(isShutdown)
	{
		pathGeometry.release ();

		D2DResource::setRegistered (false);
	}
	else
		rebuildNeeded = true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void D2DPathGeometry::checkRebuild ()
{
	if(!rebuildNeeded)
		return;

	rebuildNeeded = false;

	if(!pathGeometry)
		return;

	ComPtr<ID2D1PathGeometry> previousPathGeometry = pathGeometry;
	pathGeometry.release ();

	if(openSink ())
	{
		HRESULT hr = previousPathGeometry->Stream (sink);
		ASSERT (SUCCEEDED (hr))
		closeSink ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ID2D1PathGeometry* D2DPathGeometry::getID2D1Path ()
{
	closeSink ();
	checkRebuild ();
	return pathGeometry;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void D2DPathGeometry::closeSink ()
{
	if(sink)
	{
		if(figureStarted)
		{
			sink->EndFigure (D2D1_FIGURE_END_OPEN);
			figureStarted = false;
		}

		sink->Close ();
		sink = nullptr;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool D2DPathGeometry::openSink ()
{
	if(!pathGeometry)
	{
		HRESULT hr = DXGIEngine::instance ().getDirect2dFactory ()->CreatePathGeometry (pathGeometry);
		ASSERT (SUCCEEDED (hr))
		if(pathGeometry)
			D2DResource::setRegistered (true);
	}

	if(pathGeometry && sink == 0)
	{
		checkRebuild ();

		HRESULT hr = pathGeometry->Open (sink);
		ASSERT (SUCCEEDED (hr))
		if(SUCCEEDED (hr))
			wasOpen = true;

		if(sink)
			sink->SetFillMode (fillMode);
	}
	return sink != 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API D2DPathGeometry::lineTo (PointFRef p)
{
	ASSERT (sink && figureStarted)
	if(sink && figureStarted)
	{
		sink->AddLine (D2DInterop::fromCCL (p));
		currentPos = p;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult D2DPathGeometry::draw (NativeGraphicsDevice& device, PenRef pen)
{
	closeSink ();
	checkRebuild ();

	if(wasOpen == false) // path is empty
		return kResultOk;

	D2DGraphicsDevice* d2dDevice = ccl_cast<D2DGraphicsDevice> (&device);
	if(!d2dDevice || !pathGeometry)
		return kResultUnexpected;

	AntiAliasSetter smoother (*d2dDevice); // enable anti-aliasing
	D2DRenderTarget& target = d2dDevice->getTarget ();

	ID2D1Brush* brush = target.getBrushForPen (pen);
	ID2D1StrokeStyle* strokeStyle = target.getStyleForPen (pen);
	target->DrawGeometry (pathGeometry, brush, pen.getWidth (), strokeStyle);
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult D2DPathGeometry::fill (NativeGraphicsDevice& device, BrushRef brush)
{
	closeSink ();
	checkRebuild ();

	if(wasOpen == false) // path is empty
		return kResultOk;

	D2DGraphicsDevice* d2dDevice = ccl_cast<D2DGraphicsDevice> (&device);
	if(!d2dDevice || !pathGeometry)
		return kResultUnexpected;

	AntiAliasSetter smoother (*d2dDevice); // enable anti-aliasing
	D2DRenderTarget& target = d2dDevice->getTarget ();

	ID2D1Brush* d2dBrush = target.getUnderlyingBrush (brush);
	target->FillGeometry (pathGeometry, d2dBrush);
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API D2DPathGeometry::getBounds (Rect& bounds) const
{	
	RectF boundsF;
	getBounds (boundsF);
	bounds = CCL::rectFToInt (boundsF);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API D2DPathGeometry::getBounds (RectF& bounds) const
{
	// must close before getting bounds
	D2DPathGeometry& self = const_cast<D2DPathGeometry&> (*this);
	self.closeSink ();
	self.checkRebuild ();

	// TODO: Quote from MSDN: "...If the bounds are empty, the first value of the bounding box will be NaN..."
	D2D1_RECT_F boundsF = {};
	if(pathGeometry)
		pathGeometry->GetBounds (nullptr, &boundsF);
	bounds = D2DInterop::toCCL (boundsF);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API D2DPathGeometry::transform (TransformRef matrix)
{
	CCL_NOT_IMPL ("D2DPathGeometry::transform()")

	// TODO: see ID2D1Factory::CreateTransformedGeometry (ID2D1TransformedGeometry**)
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API D2DPathGeometry::startFigure (PointFRef p)
{
	if(openSink ())
	{
		closeFigure ();

		currentPos = p;
		sink->BeginFigure (D2DInterop::fromCCL (p), type == kPaintPath ? D2D1_FIGURE_BEGIN_FILLED : D2D1_FIGURE_BEGIN_HOLLOW);
		figureStarted = true;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API D2DPathGeometry::closeFigure ()
{
	if(figureStarted)
	{
		sink->EndFigure (D2D1_FIGURE_END_CLOSED);
		figureStarted = false;
	}	
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API D2DPathGeometry::addTriangle (PointRef p1, PointRef p2, PointRef p3)
{
	addTriangle (CCL::pointIntToF (p1), CCL::pointIntToF (p2), CCL::pointIntToF (p3));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API D2DPathGeometry::addTriangle (PointFRef p1, PointFRef p2, PointFRef p3)
{
	if(openSink ())
	{
		startFigure (p1);
		lineTo (p2);
		lineTo (p3);
		sink->EndFigure (D2D1_FIGURE_END_CLOSED); 
		figureStarted = false;
		currentPos = p1;
	}		
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API D2DPathGeometry::addRect (RectRef rect)
{
	addRect (CCL::rectIntToF (rect));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API D2DPathGeometry::addRect (RectFRef rect)
{
	if(openSink ())
	{
		startFigure (rect.getLeftTop ());
		sink->AddLine (D2D1::Point2F (rect.right, rect.top));
		sink->AddLine (D2D1::Point2F (rect.right, rect.bottom));
		sink->AddLine (D2D1::Point2F (rect.left, rect.bottom));
		sink->AddLine (D2D1::Point2F (rect.left, rect.top));
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API D2DPathGeometry::addRoundRect (RectRef rect, Coord rx, Coord ry)
{
	addRoundRect (CCL::rectIntToF (rect), (CoordF)rx, (CoordF)ry);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API D2DPathGeometry::addRoundRect (RectFRef rect, CoordF _rx, CoordF _ry)
{	
	float w = rect.getWidth ();
	float h = rect.getHeight ();

	if(w <= 0 || h <= 0)
		return;

	if(openSink ())
	{
		float left = rect.left + 0.5f;
		float top = rect.top + 0.5f;
		float right = rect.right - 0.5f;
		float bottom = rect.bottom - 0.5f;

		float rx = ccl_bound (_rx, 0.f, w / 2.f);
		float ry = ccl_bound (_ry, 0.f, h / 2.f);

		bool needHLine = w > 2.f * rx;
		bool needVLine = h > 2.f * ry;

		startFigure (PointF (left + rx, top));
		if(needHLine)
			sink->AddLine (D2D1::Point2F (right-rx, top));
		sink->AddArc (D2D1::ArcSegment (D2D1::Point2F (right, top + ry), D2D1::SizeF (rx, ry), 0, D2D1_SWEEP_DIRECTION_CLOCKWISE, D2D1_ARC_SIZE_SMALL));
		if(needVLine)
			sink->AddLine (D2D1::Point2F (right, bottom-ry));
		sink->AddArc (D2D1::ArcSegment (D2D1::Point2F (right-rx, bottom), D2D1::SizeF (rx, ry), 0, D2D1_SWEEP_DIRECTION_CLOCKWISE, D2D1_ARC_SIZE_SMALL));
		if(needHLine)
			sink->AddLine (D2D1::Point2F (left+rx, bottom));
		sink->AddArc (D2D1::ArcSegment (D2D1::Point2F (left, bottom-ry), D2D1::SizeF (rx, ry), 0, D2D1_SWEEP_DIRECTION_CLOCKWISE, D2D1_ARC_SIZE_SMALL));
		if(needVLine)
			sink->AddLine (D2D1::Point2F (left, top+ry));
		sink->AddArc (D2D1::ArcSegment (D2D1::Point2F (left + rx, top), D2D1::SizeF (rx, ry), 0, D2D1_SWEEP_DIRECTION_CLOCKWISE, D2D1_ARC_SIZE_SMALL));
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API D2DPathGeometry::addBezier (PointRef p1, PointRef c1, PointRef c2, PointRef p2)
{
	addBezier (CCL::pointIntToF (p1), CCL::pointIntToF (c1), CCL::pointIntToF (c2), CCL::pointIntToF (p2));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API D2DPathGeometry::addBezier (PointFRef p1, PointFRef c1, PointFRef c2, PointFRef p2)
{
	if(openSink ())
	{
		if(figureStarted == false)
			startFigure (p1);
		else if(currentPos != p1)
			lineTo (p1);
		sink->AddBezier (D2D1::BezierSegment (D2DInterop::fromCCL (c1), D2DInterop::fromCCL (c2), D2DInterop::fromCCL (p2)));
		currentPos = p2;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API D2DPathGeometry::addArc (RectRef rect, float startAngle, float sweepAngle)
{
	addArc (CCL::rectIntToF (rect), startAngle, sweepAngle);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API D2DPathGeometry::addArc (RectFRef rect, float startAngle, float sweepAngle)
{
	ASSERT (rect.isEmpty () == false)
	if(rect.isEmpty ())
		return;

	// startAngle: Specifies the clockwise angle, in degrees, between the horizontal axis of the ellipse and the starting point of the arc.
	// sweepAngle: Specifies the clockwise angle, in degrees, between the starting point (startAngle) and ending point of the arc. 

	if(openSink ())
	{
		// D2D draws nothing if the arc is a circle... so make it a little bit smaller:
		bool drawCircle = false;
		if(sweepAngle >= 360.f)
		{
			sweepAngle = 359.5f;
			startAngle = 0.f;
			drawCircle = true;
		}

		double centerX = rect.getWidth () / 2.0;	
		double centerY = rect.getHeight () / 2.0;	
		double left = rect.left;
		double top = rect.top;
		
		double startRadian = Math::degreesToRad<double> (startAngle);
		double lengthRadian = Math::degreesToRad<double> (sweepAngle);
		double endRadian = startRadian + lengthRadian;
		if(endRadian > 2.0 * Math::Constants<double>::kPi)
			endRadian -= 2.0 * Math::Constants<double>::kPi;

		float startX = float (left + centerX + (centerX * Math::Functions<double>::cos (startRadian)));
		float startY = float (top + centerY + (centerY * Math::Functions<double>::sin (startRadian)));

		float endX = float (left + centerX + (centerX * Math::Functions<double>::cos (endRadian)));
		float endY = float (top + centerY + (centerY * Math::Functions<double>::sin (endRadian)));

		PointF start (startX, startY);
		if(figureStarted == false)
			startFigure (start);
		else if(currentPos != start)
			lineTo (start);			

		CCL_PRINTF ("D2DPathGeometry::addArc center (%.3f, %.3f), rx: %.3f, ry:%.3f %.3f, %.3f\t=>\tstart (%.3f, %.3f) end (%.3f, %.3f)\n\n", rect.getCenter ().x, rect.getCenter ().y, rect.getWidth () / 2.0, rect.getHeight () / 2.0, startX, startY, endX, endY)

		D2D1_ARC_SEGMENT segment = D2D1::ArcSegment (
			D2D1::Point2F (endX, endY),
			D2D1::SizeF (float (centerX), float (centerY)),
			0,
			sweepAngle > 0 ? D2D1_SWEEP_DIRECTION_CLOCKWISE : D2D1_SWEEP_DIRECTION_COUNTER_CLOCKWISE,
			ccl_abs (sweepAngle) > 180 ? D2D1_ARC_SIZE_LARGE : D2D1_ARC_SIZE_SMALL);

		sink->AddArc (segment);
		currentPos = D2DInterop::toCCL (segment.point);
		if(drawCircle)
			closeFigure ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API D2DPathGeometry::setFillMode (FillMode mode)
{
	fillMode = (mode == kFillNonZero) ? D2D1_FILL_MODE_WINDING : D2D1_FILL_MODE_ALTERNATE;
}
