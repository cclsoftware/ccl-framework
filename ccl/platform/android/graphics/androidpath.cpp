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
// Filename    : ccl/platform/android/graphics/androidpath.cpp
// Description : Android Graphics Path
//
//************************************************************************************************

#include "androidpath.h"
#include "androidgraphics.h"
#include "frameworkgraphics.h"

//************************************************************************************************
// JNI classes
//************************************************************************************************

namespace CCL {
namespace Android {

//************************************************************************************************
// FrameworkGraphicsPath
//************************************************************************************************

DECLARE_JNI_CLASS (FrameworkGraphicsPath, CCLGUI_CLASS_PREFIX "FrameworkGraphicsPath")
	DECLARE_JNI_METHOD (void, computeBounds, jobject)
	DECLARE_JNI_METHOD (void, transform, float, float, float, float, float, float)
	DECLARE_JNI_METHOD (void, moveTo, float, float)
	DECLARE_JNI_METHOD (void, close)
	DECLARE_JNI_METHOD (void, lineTo, float, float)
	DECLARE_JNI_METHOD (void, addRect, float, float, float, float)
	DECLARE_JNI_METHOD (void, addRoundRect, float, float, float, float, float, float)
	DECLARE_JNI_METHOD (void, addBezier, float, float, float, float, float, float)
	DECLARE_JNI_METHOD (void, addArc, float, float, float, float, float, float)
	DECLARE_JNI_METHOD (void, setFillMode, bool)
END_DECLARE_JNI_CLASS (FrameworkGraphicsPath)

DEFINE_JNI_CLASS (FrameworkGraphicsPath)
	DEFINE_JNI_DEFAULT_CONSTRUCTOR
	DEFINE_JNI_METHOD (computeBounds, "(Landroid/graphics/RectF;)V")
	DEFINE_JNI_METHOD (transform, "(FFFFFF)V")
	DEFINE_JNI_METHOD (moveTo, "(FF)V")
	DEFINE_JNI_METHOD (close, "()V")
	DEFINE_JNI_METHOD (lineTo, "(FF)V")
	DEFINE_JNI_METHOD (addRect, "(FFFF)V")
	DEFINE_JNI_METHOD (addRoundRect, "(FFFFFF)V")
	DEFINE_JNI_METHOD (addBezier, "(FFFFFF)V")
	DEFINE_JNI_METHOD (addArc, "(FFFFFF)V")
	DEFINE_JNI_METHOD (setFillMode, "(Z)V")
END_DEFINE_JNI_CLASS

} // namespace Android
} // namespace CCL

using namespace CCL;
using namespace CCL::Android;

//************************************************************************************************
// AndroidGraphicsPath
//************************************************************************************************

DEFINE_CLASS (AndroidGraphicsPath, NativeGraphicsPath)

//////////////////////////////////////////////////////////////////////////////////////////////////

AndroidGraphicsPath::AndroidGraphicsPath ()
{
	newObject (FrameworkGraphicsPath);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult AndroidGraphicsPath::draw (NativeGraphicsDevice& device, PenRef pen)
{
	IGraphics& graphics (device);
	return graphics.drawPath (this, pen);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult AndroidGraphicsPath::fill (NativeGraphicsDevice& device, BrushRef brush)
{
	IGraphics& graphics (device);
	return graphics.fillPath (this, brush);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API AndroidGraphicsPath::getBounds (Rect& bounds) const
{
	RectF b;
	getBounds (b);
	bounds (coordFToInt (b.left), coordFToInt (b.top), coordFToInt (b.right), coordFToInt (b.bottom));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API AndroidGraphicsPath::getBounds (RectF& bounds) const
{
	// create a java RectF for transferring result
	JniAccessor jni;
	LocalRef jrect (jni, jni.newObject (AndroidRectF));
	if(jrect)
	{
		FrameworkGraphicsPath.computeBounds (*this, jrect);

		bounds.left = jni.getField (jrect, AndroidRectF.left);
		bounds.top = jni.getField (jrect, AndroidRectF.top);
		bounds.right = jni.getField (jrect, AndroidRectF.right);
		bounds.bottom = jni.getField (jrect, AndroidRectF.bottom);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API AndroidGraphicsPath::transform (TransformRef t)
{
	FrameworkGraphicsPath.transform (*this, t.a0, t.a1, t.b0, t.b1, t.t0, t.t1);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API AndroidGraphicsPath::startFigure (PointFRef p)
{
	FrameworkGraphicsPath.moveTo (*this, p.x, p.y);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API AndroidGraphicsPath::closeFigure ()
{
	FrameworkGraphicsPath.close (*this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API AndroidGraphicsPath::lineTo (PointRef p)
{
	lineTo (pointIntToF (p));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API AndroidGraphicsPath::lineTo (PointFRef p)
{
	FrameworkGraphicsPath.lineTo (*this, p.x, p.y);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API AndroidGraphicsPath::addRect (RectRef rect)
{
	addRect (rectIntToF (rect));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API AndroidGraphicsPath::addRect (RectFRef rect)
{
	FrameworkGraphicsPath.addRect (*this, rect.left, rect.top, rect.right, rect.bottom);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API AndroidGraphicsPath::addRoundRect (RectRef rect, Coord rx, Coord ry)
{
	addRoundRect (rectIntToF (rect), CoordF (rx), CoordF (ry));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API AndroidGraphicsPath::addRoundRect (RectFRef rect, CoordF rx, CoordF ry)
{
	FrameworkGraphicsPath.addRoundRect (*this, rect.left, rect.top, rect.right, rect.bottom, rx, ry);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API AndroidGraphicsPath::addBezier (PointRef p1, PointRef c1, PointRef c2, PointRef p2)
{
	addBezier (pointIntToF (p1), pointIntToF (c1), pointIntToF (c2), pointIntToF (p2));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API AndroidGraphicsPath::addBezier (PointFRef p1, PointFRef c1, PointFRef c2, PointFRef p2)
{
	FrameworkGraphicsPath.lineTo (*this, p1.x, p1.y);
	FrameworkGraphicsPath.addBezier (*this, c1.x, c1.y, c2.x, c2.y, p2.x, p2.y);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API AndroidGraphicsPath::addArc (RectRef rect, float startAngle, float sweepAngle)
{
	addArc (rectIntToF (rect), startAngle, sweepAngle);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API AndroidGraphicsPath::addArc (RectFRef rect, float startAngle, float sweepAngle)
{
	FrameworkGraphicsPath.addArc (*this, rect.left, rect.top, rect.right, rect.bottom, startAngle, sweepAngle);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API AndroidGraphicsPath::addTriangle (PointRef p1, PointRef p2, PointRef p3)
{
	addTriangle (pointIntToF (p1), pointIntToF (p2), pointIntToF (p3));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API AndroidGraphicsPath::addTriangle (PointFRef p1, PointFRef p2, PointFRef p3)
{
	startFigure (p1);
	lineTo (p2);
	lineTo (p3);
	closeFigure ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API AndroidGraphicsPath::setFillMode (FillMode fillMode)
{
	FrameworkGraphicsPath.setFillMode (*this, fillMode == kFillNonZero);
}
