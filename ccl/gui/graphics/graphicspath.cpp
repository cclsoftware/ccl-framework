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
// Filename    : ccl/gui/graphics/graphicspath.cpp
// Description : Graphical Path
//
//************************************************************************************************

#include "ccl/gui/graphics/graphicspath.h"

#include "ccl/gui/graphics/nativegraphics.h"

using namespace CCL;

//************************************************************************************************
// GraphicsPath
//************************************************************************************************

DEFINE_CLASS_HIDDEN (GraphicsPath, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

GraphicsPath::GraphicsPath (TypeHint type)
{
	nativePath = NativeGraphicsEngine::instance ().createPath (type);
	ASSERT (nativePath != nullptr)
}

//////////////////////////////////////////////////////////////////////////////////////////////////

GraphicsPath::GraphicsPath (const GraphicsPath& p)
{
	ASSERT (p.nativePath != nullptr)
	nativePath = (NativeGraphicsPath*)p.nativePath->clone ();
	ASSERT (nativePath != nullptr)
}

//////////////////////////////////////////////////////////////////////////////////////////////////

GraphicsPath::~GraphicsPath ()
{
	nativePath->release ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API GraphicsPath::setFillMode (FillMode fillMode)
{
	nativePath->setFillMode (fillMode);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API GraphicsPath::getBounds (Rect& bounds) const
{
	nativePath->getBounds (bounds);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API GraphicsPath::getBounds (RectF& bounds) const
{
	nativePath->getBounds (bounds);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API GraphicsPath::transform (TransformRef matrix)
{
	nativePath->transform (matrix);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API GraphicsPath::startFigure (PointRef p)
{
	nativePath->startFigure (p);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API GraphicsPath::startFigure (PointFRef p)
{
	nativePath->startFigure (p);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API GraphicsPath::closeFigure ()
{
	nativePath->closeFigure ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API GraphicsPath::lineTo (PointRef p)
{
	nativePath->lineTo (p);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API GraphicsPath::lineTo (PointFRef p)
{
	nativePath->lineTo (p);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API GraphicsPath::addRect (RectRef rect)
{
	nativePath->addRect (rect);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API GraphicsPath::addRect (RectFRef rect)
{
	nativePath->addRect (rect);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API GraphicsPath::addRoundRect (RectRef rect, Coord rx, Coord ry)
{
	nativePath->addRoundRect (rect, rx, ry);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API GraphicsPath::addRoundRect (RectFRef rect, CoordF rx, CoordF ry)
{
	nativePath->addRoundRect (rect, rx, ry);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API GraphicsPath::addTriangle (PointRef p1, PointRef p2, PointRef p3)
{
	nativePath->addTriangle (p1, p2, p3);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API GraphicsPath::addTriangle (PointFRef p1, PointFRef p2, PointFRef p3)
{
	nativePath->addTriangle (p1, p2, p3);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API GraphicsPath::addBezier (PointRef p1, PointRef c1, PointRef c2, PointRef p2)
{
	nativePath->addBezier (p1, c1, c2, p2);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API GraphicsPath::addBezier (PointFRef p1, PointFRef c1, PointFRef c2, PointFRef p2)
{
	nativePath->addBezier (p1, c1, c2, p2);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API GraphicsPath::addArc (RectRef rect, float startAngle, float sweepAngle)
{
	nativePath->addArc (rect, startAngle, sweepAngle);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API GraphicsPath::addArc (RectFRef rect, float startAngle, float sweepAngle)
{
	nativePath->addArc (rect, startAngle, sweepAngle);
}
