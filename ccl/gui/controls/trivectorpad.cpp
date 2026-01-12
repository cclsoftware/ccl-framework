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
// Filename    : ccl/gui/controls/trivectorpad.cpp
// Description : Triangular Vector Pad (XYZ-Control)
//
//************************************************************************************************

#include "ccl/gui/controls/trivectorpad.h"

#include "ccl/gui/theme/themerenderer.h"
#include "ccl/gui/views/mousehandler.h"

#include "ccl/public/gui/iparameter.h"
#include "ccl/public/math/mathprimitives.h"

namespace CCL {

//************************************************************************************************
// TriVectorPadMouseHandler
//************************************************************************************************

class TriVectorPadMouseHandler: public MouseHandler
{
public:
	DECLARE_CLASS_ABSTRACT (TriVectorPadMouseHandler, MouseHandler)

	TriVectorPadMouseHandler (TriVectorPad* pad, PointRef clickOffset);
	~TriVectorPadMouseHandler ();
	
	void onBegin () override;
	void onRelease (bool canceled) override;
	bool onMove (int moveFlags) override;
	
protected:

	Point clickOffset;
	Point fineWhere;
	Point sumStartWhere;
	Point sumOffset;
	TriVectorPad* pad;
	bool wasFineMode;
	
	void updateTooltip ();
};

} // namespace CCL

using namespace CCL;

//************************************************************************************************
// TriVectorPad
//************************************************************************************************

DEFINE_CLASS_HIDDEN (TriVectorPad, Control)

//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_STYLEDEF (TriVectorPad::customStyles)
	{"keeplevel", Styles::kTriVectorPadBehaviorKeepLevel},
	{"upsidedown", Styles::kTriVectorPadAppearanceUpsideDown},
	{"tooltip", Styles::kTriVectorPadBehaviorTooltip},
	{"invert", Styles::kTriVectorPadBehaviorInvert},
END_STYLEDEF

//////////////////////////////////////////////////////////////////////////////////////////////////

TriVectorPad::TriVectorPad (RectRef size, IParameter* param, IParameter* _yParam, IParameter* _zParam, StyleRef style)
: Control (size, param, style),
  keepLevels (style.isCustomStyle (Styles::kTriVectorPadBehaviorKeepLevel)),
  upsideDown (style.isCustomStyle (Styles::kTriVectorPadAppearanceUpsideDown)),
  inverted (style.isCustomStyle (Styles::kTriVectorPadBehaviorInvert)),
  editing (false),
  keepSnapPointHighlight (false),
  yParam (nullptr),
  zParam (nullptr),
  keptSum (0),
  xOver (0),
  yOver (0),
  zOver (0),
  snapPointValueFactor (0),
  highlightSnapPoint (-1)
{
	resetExplicitHandlePosition ();
	
	share_and_observe_unknown (this, yParam, _yParam);
	share_and_observe_unknown (this, zParam, _zParam);
	
	if(param && yParam && zParam)
		setKeptSum (param->getNormalized () + yParam->getNormalized () + zParam->getNormalized ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

TriVectorPad::~TriVectorPad ()
{
	share_and_observe_unknown<IParameter> (this, yParam, nullptr);
	share_and_observe_unknown<IParameter> (this, zParam, nullptr);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TriVectorPad::beginEditing ()
{
	editing = true;
	param->beginEdit ();
	yParam->beginEdit ();
	zParam->beginEdit ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TriVectorPad::endEditing ()
{
	editing = false;
	param->endEdit ();
	yParam->endEdit ();
	zParam->endEdit ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TriVectorPad::setKeptSum (double sum)
{
	keptSum = sum;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool TriVectorPad::isInsideTriangle (PointRef where, bool hoverTriangle) const
{
	Point a (getTrianglePoint (kCornerA, hoverTriangle));
	Point b (getTrianglePoint (kCornerB, hoverTriangle));
	Point c (getTrianglePoint (kCornerC, hoverTriangle));
	
	float cross0 = ((b.y - a.y) * (where.x - a.x)) - ((b.x - a.x) * (where.y - a.y));
	float cross1 = ((c.y - b.y) * (where.x - b.x)) - ((c.x - b.x) * (where.y - b.y));
	float cross2 = ((a.y - c.y) * (where.x - c.x)) - ((a.x - c.x) * (where.y - c.y));
	
	bool result = ((cross0 >= 0) && (cross1 >= 0) && (cross2 >= 0)) ? true : false;
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

TriVectorPad::OuterSectionID TriVectorPad::calcOuterSectionID (PointRef where) const
{
	Point a (getTrianglePoint (kCornerA));
	Point b (getTrianglePoint (kCornerB));
	Point c (getTrianglePoint (kCornerC));
	
	float cross0 = ((b.y - a.y) * (where.x - a.x)) - ((b.x - a.x) * (where.y - a.y));
	float cross1 = ((c.y - b.y) * (where.x - b.x)) - ((c.x - b.x) * (where.y - b.y));
	float cross2 = ((a.y - c.y) * (where.x - c.x)) - ((a.x - c.x) * (where.y - c.y));
	
	if(cross0 < 0 && cross2 < 0)
		return kOutSnapA;
	else if(cross0 < 0 && cross1 < 0)
		return kOutSnapB;
	else if(cross1 < 0 && cross2 < 0)
		return kOutSnapC;
	else if(cross0 < 0)
		return kOutAB;
	else if(cross1 < 0)
		return kOutBC;
	else
		return kOutCA;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

TriVectorPad::CornerID TriVectorPad::getCornerIDForSection (OuterSectionID code) const
{
	switch(code)
	{
	case kOutSnapA: return kCornerA;
	case kOutSnapB: return kCornerB;
	case kOutSnapC: return kCornerC;
	case kOutAB: return kCornerC;
	case kOutBC: return kCornerA;
	case kOutCA: return kCornerB;
	}
	
	ASSERT (false)
	return kCornerA;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TriVectorPad::getBasisForCornerID (Point& start, Point& end, CornerID cornerPoint) const
{
	switch(cornerPoint)
	{
	case kCornerA: start = getTrianglePoint (kCornerB); end = getTrianglePoint (kCornerC); return;
	case kCornerB: start = getTrianglePoint (kCornerC); end = getTrianglePoint (kCornerA); return;
	case kCornerC: start = getTrianglePoint (kCornerA); end = getTrianglePoint (kCornerB); return;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TriVectorPad::calcTriangleIntersection (Point& p) const
{
	OuterSectionID sectionID = calcOuterSectionID (p);
	CornerID cornerID = getCornerIDForSection (sectionID);

	switch(sectionID)
	{
	case kOutSnapA: p = getTrianglePoint (cornerID); return;
	case kOutSnapB: p = getTrianglePoint (cornerID); return;
	case kOutSnapC: p = getTrianglePoint (cornerID); return;
	default:
		{
			Point anchor = getTrianglePoint (cornerID);
			Point start, end;
			getBasisForCornerID (start, end, cornerID);
			Point handle (p);
			getIntersectionPoint (p, start, end, anchor, handle);
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TriVectorPad::setHandlePosition (PointRef p)
{
	setExplicitHandlePosition (p);
	setValuesFromBarycentric (convertCartesianToBarycentric (p));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Point TriVectorPad::getHandlePosition () const
{
	if(explicitHandlePosition.x == -1 && explicitHandlePosition.y == -1)
		return getHandlePositionFromValues (); 
	else
		return explicitHandlePosition;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Point TriVectorPad::getTrianglePoint (CornerID code, bool hoverTriangle) const
{
	Rect triangleRect;
	const_cast<TriVectorPad*>(this)->getRenderer ()->getPartRect (this, hoverTriangle ? kPartHoverTriangle : kPartTriangle, triangleRect);
	
	switch(code)
	{
	case kCornerA:
		return Point ((triangleRect.left + triangleRect.right) / 2.f, upsideDown ? triangleRect.bottom : triangleRect.top);
	case kCornerB:
		return Point (upsideDown ? triangleRect.right : triangleRect.left, upsideDown ? triangleRect.top : triangleRect.bottom);
	case kCornerC:
		return Point (upsideDown ? triangleRect.left : triangleRect.right, upsideDown ? triangleRect.top : triangleRect.bottom);
	}
	
	ASSERT (false)
	return Point ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Point TriVectorPad::getSnapPoint (int partCode) const
{
	if(inverted)
	{
		if(partCode < kPartSnapPointAB)
			return getTriangleSidePoint (SideID (partCode));
		else
			return getTrianglePoint (CornerID (partCode - 3));
	}
	else
	{
		if(partCode < kPartSnapPointAB)
			return getTrianglePoint (CornerID (partCode));
		else
			return getTriangleSidePoint (SideID (partCode - 3));
	}
}
	
//////////////////////////////////////////////////////////////////////////////////////////////////

Point TriVectorPad::getTriangleSidePoint (SideID code) const
{
	PointF p1 (0.5, upsideDown ? 1 : 0);
	PointF p2 (upsideDown ? 1 : 0, upsideDown ? 0 : 1);
	PointF p3 (upsideDown ? 0 : 1, upsideDown ? 0 : 1);
	
	PointF sideCenterA ((p2.x + p3.x) / 2.f, (p2.y + p3.y) / 2.f);
	PointF sideCenterB ((p3.x + p1.x) / 2.f, (p3.y + p1.y) / 2.f);
	PointF sideCenterC ((p1.x + p2.x) / 2.f, (p1.y + p2.y) / 2.f);
	
	switch(code)
	{
	case kSideAB:
		return convertNormalizedToCartesian (sideCenterA);
	case kSideBC:
		return convertNormalizedToCartesian (sideCenterB);
	case kSideCA:
		return convertNormalizedToCartesian (sideCenterC);
	}
	
	ASSERT (false)
	return Point ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API TriVectorPad::notify (ISubject* subject, MessageRef msg)
{
	if(msg == kChanged)
	{
		if(isEqualUnknown (subject, param) || isEqualUnknown (subject, yParam) || isEqualUnknown (subject, zParam))
		{
			if(!editing)
			{
				if(keepLevels) // remember new combined level
					setKeptSum (param->getNormalized () + yParam->getNormalized () + zParam->getNormalized ());

				// reset temporary manipulation values
				snapPointValueFactor = 0;
				xOver = yOver = zOver = 0;
				resetExplicitHandlePosition ();
			}
			paramChanged ();
			return;
		}
	}
	
	SuperClass::notify (subject, msg);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool TriVectorPad::onMouseEnter (const MouseEvent& event)
{
	keepSnapPointHighlight = false;
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool TriVectorPad::onMouseMove (const MouseEvent& event)
{
	if(isInsideTriangle (event.where, true))
	{
		int partCode = getRenderer ()->hitTest (this, event.where);
		if(isSnapPointCode (partCode))
		{
			setHighlightSnapPoint (partCode);
			return true;
		}
	}
	
	setHighlightSnapPoint (-1);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool TriVectorPad::isSnapPointCode (int partCode) const
{
	return (partCode >= kFirstSnapPoint && partCode <= kLastSnapPoint);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool TriVectorPad::onMouseDown (const MouseEvent& event)
{
	Rect rect;
	int partCode;
	if(hitHandleOrSnapPoint (partCode, event))
		getRenderer ()->getPartRect (this, partCode, rect);
	
	if(rect.pointInside (event.where) || isInsideTriangle (event.where))
		return SuperClass::onMouseDown (event);
		
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool TriVectorPad::onMouseLeave (const MouseEvent& event)
{
	if(!keepSnapPointHighlight)
		setHighlightSnapPoint (-1);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool TriVectorPad::onMouseWheel (const MouseWheelEvent& event)
{
	if(View::onMouseWheel (event))
		return true;
	
	if(isWheelEnabled ())
	{
		SharedPtr<View> holder (this);
		if(handleMouseWheel (event, getNearestParameter (event.where)))
			return true;
	}
	
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ThemeRenderer* TriVectorPad::getRenderer ()
{
	if(renderer == nullptr)
		renderer = getTheme ().createRenderer (ThemePainter::kTriVectorPadRenderer, visualStyle);

	return renderer;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

MouseHandler* TriVectorPad::createMouseHandler (const MouseEvent& event)
{
	if(isResetClick (event))
	{
		performReset ();
		return NEW NullMouseHandler (this); // swallow mouse click
	}

	Rect rect;
	int partCode;
	if(hitHandleOrSnapPoint (partCode, event))
		getRenderer ()->getPartRect (this, partCode, rect);
	
	Point clickOffset;
	if(rect.pointInside (event.where))
	{
		clickOffset.x = event.where.x - int (0.5 * (rect.left + rect.right));
		clickOffset.y = event.where.y - int (0.5 * (rect.top + rect.bottom));
		 
		setHighlightSnapPoint (-1); // hide snapPoint highlight when clicked
	}

	return NEW TriVectorPadMouseHandler (this, clickOffset);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TriVectorPad::paramChanged ()
{
	SuperClass::paramChanged ();
	
	if(yParam && yParam->isEnabled () || zParam && zParam->isEnabled ())
		enable (true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TriVectorPad::performReset ()
{
	auto resetParameter = [](IParameter* p)
	{
		p->beginEdit ();
		p->setValue (p->getDefaultValue (), true);
		p->endEdit ();
	};
	
	resetParameter (param);
	resetParameter (yParam);
	resetParameter (zParam);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

double TriVectorPad::getXValue () const
{
	return NormalizedValue (param).get ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

double TriVectorPad::getYValue () const
{
	return NormalizedValue (yParam).get ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

double TriVectorPad::getZValue () const
{
	return NormalizedValue (zParam).get ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TriVectorPad::setXValue (double v, bool update)
{
	setNormalizedValue (param, v, update);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TriVectorPad::setYValue (double v, bool update)
{
	setNormalizedValue (yParam, v, update);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TriVectorPad::setZValue (double v, bool update)
{
	setNormalizedValue (zParam, v, update);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TriVectorPad::setNormalizedValue (IParameter* p, double v, bool update)
{
	if(p == nullptr)
		return;

	if(p->isEnabled () == false)
		return;
	
	NormalizedValue (p).set (v, update);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Point TriVectorPad::getHandlePositionFromValues () const
{
	return convertBarycentricToCartesian (PointF3D (getBarycentricFromValues ()));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Point TriVectorPad::getTriangleCenterOfMass () const
{	
	PointF3D center (0.333334f, 0.333333f, 0.333333f);
	return convertBarycentricToCartesian (center);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Point TriVectorPad::convertBarycentricToCartesian (PointF3DRef bp) const
{
	// cartesian coordinates in normalized acute triangle
	PointF p1 (0.5, upsideDown ? 1 : 0);
	PointF p2 (upsideDown ? 1 : 0, upsideDown ? 0 : 1);
	PointF p3 (upsideDown ? 0 : 1, upsideDown ? 0 : 1);
	
	float px = (bp.x * p1.x + bp.y * p2.x + bp.z * p3.x);
	float py = (bp.x * p1.y + bp.y * p2.y + bp.z * p3.y);
	PointF trianglePoint (px, py);
	return convertNormalizedToCartesian (trianglePoint);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Point TriVectorPad::convertNormalizedToCartesian (PointF& a) const
{
	Rect triangleRect;
	const_cast<TriVectorPad*>(this)->getRenderer ()->getPartRect (this, kPartTriangle, triangleRect);

	a.x *= triangleRect.getWidth ();
	a.y *= triangleRect.getHeight ();
	a.x += triangleRect.left;
	a.y += triangleRect.top;
	
	return Point (a.x, a.y);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

PointF3D TriVectorPad::convertCartesianToBarycentric (PointRef p) const
{
	PointF np (convertCartesianToNormalized (p));
	
	// https://en.wikipedia.org/wiki/Barycentric_coordinate_system
	
	// cartesian coordinates in normalized acute triangle
	PointF p1 (0.5, upsideDown ? 1 : 0);
	PointF p2 (upsideDown ? 1 : 0, upsideDown ? 0 : 1);
	PointF p3 (upsideDown ? 0 : 1, upsideDown ? 0 : 1);
	
	double detA = ((p2.y - p3.y) * (p1.x - p3.x) + (p3.x - p2.x) * (p1.y - p3.y));
	
	PointF3D barycentric;
	barycentric.x = ((p2.y - p3.y) * (np.x - p3.x) + (p3.x - p2.x) * (np.y - p3.y)) / detA;
	barycentric.y = ((p3.y - p1.y) * (np.x - p3.x) + (p1.x - p3.x) * (np.y - p3.y)) / detA;
	barycentric.z = 1.f - barycentric.x - barycentric.y;
	
	return barycentric; 
}

//////////////////////////////////////////////////////////////////////////////////////////////////

PointF TriVectorPad::convertCartesianToNormalized (PointRef p) const
{
	PointF normP;
	Rect triangleRect;
	const_cast<TriVectorPad*>(this)->getRenderer ()->getPartRect (this, kPartTriangle, triangleRect);
	normP.x = (p.x - triangleRect.left) / (float)triangleRect.getWidth ();
	normP.y = (p.y - triangleRect.top) / (float)triangleRect.getHeight ();
	
	return normP; 
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool TriVectorPad::setValuesFromBarycentric (PointF3DRef b)
{ 
	double x = b.x;
	double y = b.y;
	double z = b.z;
	
	if(inverted)
	{
		x = 1 - x;
		y = 1 - y;
		z = 1 - z;
	}

	if(keepLevels)
	{
		double factor = inverted ? keptSum * 0.5 : keptSum;
		x *= factor;
		y *= factor;
		z *= factor;
	}
	
	if(inverted)
	{
		// blend with values at snappoint, depending on the distance (snapPointValueFactor)
		PointF3D s (getValuesAtNearestSnapPoint (x, y, z));
		calcSnapPointValueFactor (b);
		x = ((1 - snapPointValueFactor) * x) + (snapPointValueFactor * s.x);
		y = ((1 - snapPointValueFactor) * y) + (snapPointValueFactor * s.y);
		z = ((1 - snapPointValueFactor) * z) + (snapPointValueFactor * s.z);
	}
	
	if(x > 1)
		xOver = x - 1;
	if(y > 1)
		yOver = y - 1;
	if(z > 1)
		zOver = z - 1;
	
	setXValue (x, true);
	setYValue (y, true);
	setZValue (z, true);
	
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

PointF3D TriVectorPad::getBarycentricFromValues () const
{
	double a = getXValue ();
	double b = getYValue ();
	double c = getZValue ();
	
	if(xOver > 0)
		a += xOver;
	if(yOver > 0)
		b += yOver;
	if(zOver > 0)
		c += zOver;
	
	if(inverted) 
	{
		// subtract blended snappoint values, depending on snapPointValueFactor
		PointF3D s (getValuesAtNearestSnapPoint (a, b, c));
		a -= (snapPointValueFactor * s.x);
		b -= (snapPointValueFactor * s.y);
		c -= (snapPointValueFactor * s.z);
	
		a *= 1 / (1 - snapPointValueFactor);
		b *= 1 / (1 - snapPointValueFactor);
		c *= 1 / (1 - snapPointValueFactor);
	}
			
	if(keepLevels && keptSum > 0)
	{	
		double factor = inverted ? 1 / (keptSum * 0.5) : 1 / keptSum;
		a *= factor;
		b *= factor;
		c *= factor;
	}
	
	if(inverted)
	{
		a = 1 - a;
		b = 1 - b;
		c = 1 - c;
	}
	
	return validateBarycentric (a, b, c);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

PointF3D TriVectorPad::validateBarycentric (double& a, double& b, double& c) const
{
	if(a < 0)
	{
		if(b < 0)
			b = 0;
		else if(c < 0)
			c = 0;
		
		b *= 1 / ccl_max ((b + c), 0.01);
		c = 1 - b;
		a = 0;
	}
	else if(b < 0)
	{
		if(c < 0)
			c = 0;
			
		a *= 1 / (a + c);
		c = 1 - a;
		b = 0;
	}
	else if(c < 0)
	{
		b *= 1 / (b + a);
		a = 1 - b;
		c = 0;
	}
	
	if(a > 1)
	{
		a = 1;
		b = 0;
		c = 0;
	}
	else if(b > 1)
	{
		a = 0;
		b = 1;
		c = 0;
	}
	else if(c > 1)
	{
		a = 0;
		b = 0;
		c = 1;
	}
	
	double sum = a + b + c;
	if(sum != 1)
	{
		if(sum == 0)
		{
			a = 0.33333f;
			b = 0.33333f;
			c = 0.33334f;
		}
		else
		{
			a *= 1 / sum;
			b *= 1 / sum;
			c *= 1 / sum;
		}
	}
	
	return PointF3D (a, b, c);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TriVectorPad::calcSnapPointValueFactor (PointF3DRef b)
{
	// snapPointValueFactor [0-1] inverse to the distance: handle to snappoint inside spreadRangeThreshold
	Point currentPos (getHandlePosition ());

	auto getDistance = [](PointRef a, PointRef b)
	{
		return ::sqrtf ((a.x - b.x) * (a.x - b.x) + (a.y - b.y) * (a.y - b.y));
	};
	
	double distanceA = getDistance (currentPos, getSnapPoint (kPartSnapPointA));
	double distanceB = getDistance (currentPos, getSnapPoint (kPartSnapPointB));
	double distanceC = getDistance (currentPos, getSnapPoint (kPartSnapPointC));

	Rect triangleRect;
	getRenderer ()->getPartRect (this, kPartTriangle, triangleRect);
	double spreadRangeThreshold = ccl_min ((triangleRect.getHeight () / 4), (triangleRect.getWidth () / 4));
	
	snapPointValueFactor = 1 - (ccl_min (distanceA, ccl_min (distanceB, ccl_min (distanceC, spreadRangeThreshold))) / spreadRangeThreshold);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

PointF3D TriVectorPad::getValuesAtNearestSnapPoint (double xs, double ys, double zs) const
{
	if(xs >= ys)
	{
		if(xs >= zs)
		{
			xs = keepLevels ? ccl_min (keptSum, 1.) : 1.f;
			ys = 0;
			zs = 0;
		}
		else
		{
			xs = 0;
			ys = 0;
			zs = keepLevels ? ccl_min (keptSum, 1.) : 1.f;
		}
	}
	else
	{
		if(ys >= zs)
		{
			xs = 0;
			ys = keepLevels ? ccl_min (keptSum, 1.) : 1.f;
			zs = 0;
		}
		else
		{
			xs = 0;
			ys = 0;
			zs = keepLevels ? ccl_min (keptSum, 1.) : 1.f;
		}
	}
	
	return PointF3D (xs, ys, zs);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TriVectorPad::setHighlightSnapPoint (int partCode)
{
	if(highlightSnapPoint != partCode)
	{
		highlightSnapPoint = partCode;
		invalidate ();	
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool TriVectorPad::hitHandleOrSnapPoint (int& partCode, const MouseEvent& event)
{
	if(isInsideTriangle (event.where, true))
	{
		partCode = getRenderer ()->hitTest (this, event.where);
		if(partCode == kPartHandle || isSnapPointCode (partCode))
			return true;
	}
	
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TriVectorPad::setExplicitHandlePosition (PointRef p)
{
	explicitHandlePosition.x = p.x;
	explicitHandlePosition.y = p.y;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TriVectorPad::resetExplicitHandlePosition ()
{
	explicitHandlePosition.x = -1;
	explicitHandlePosition.y = -1;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int TriVectorPad::getNearestSnapPoint (PointRef where) const
{
	auto getManhattanDistance = [](PointRef a, PointRef b)
	{
		return ((a.x - b.x) * (a.x - b.x) + (a.y - b.y) * (a.y - b.y));
	};
	
	double distanceA = getManhattanDistance (where, getSnapPoint (kPartSnapPointA));
	double distanceB = getManhattanDistance (where, getSnapPoint (kPartSnapPointB));
	double distanceC = getManhattanDistance (where, getSnapPoint (kPartSnapPointC));
	
	int partCode;
	
	if(distanceA < distanceB)
		partCode = (distanceA < distanceC) ? kPartSnapPointA : kPartSnapPointC;
	else
		partCode = (distanceB < distanceC) ? kPartSnapPointB : kPartSnapPointC;
	
	return partCode;
}
	
//////////////////////////////////////////////////////////////////////////////////////////////////

IParameter* TriVectorPad::getParameterForSnapPoint (int snapPoint) const
{
	if(snapPoint == kPartSnapPointA)
		return param;
	else if(snapPoint == kPartSnapPointB)
		return yParam;
	else if(snapPoint == kPartSnapPointC)
		return zParam;
	else
		return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IParameter* TriVectorPad::getNearestParameter (PointRef where) const
{
	return getParameterForSnapPoint (getNearestSnapPoint (where));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool TriVectorPad::onContextMenu (const ContextMenuEvent& event)
{
	if(isContextMenuEnabled ())
	{
		Rect handleRect;
		getRenderer ()->getPartRect (this, kPartHandle, handleRect);
		
		// not above main handle
		if(!handleRect.pointInside (event.where))
		{
			int partCode = getNearestSnapPoint (event.where);
			setHighlightSnapPoint (partCode);
			keepSnapPointHighlight = true;
			return contextMenuForParam (event, getParameterForSnapPoint (partCode));
		}
	}
	return true;
}

//************************************************************************************************
// TriVectorPadMouseHandler
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (TriVectorPadMouseHandler, MouseHandler)

//////////////////////////////////////////////////////////////////////////////////////////////////

TriVectorPadMouseHandler::TriVectorPadMouseHandler (TriVectorPad* pad, PointRef clickOffset)
: MouseHandler (pad),
  pad (pad),
  clickOffset (clickOffset)
{
	checkKeys (true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

TriVectorPadMouseHandler::~TriVectorPadMouseHandler ()
{
	tooltipPopup.reserve (false);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TriVectorPadMouseHandler::onBegin ()
{ 	
	wasFineMode = (current.keys.getModifiers () & KeyState::kShift) != 0;
	fineWhere = current.where;
	sumStartWhere = current.where;

	if(!pad->isInsideTriangle (sumStartWhere))
		sumStartWhere = pad->getHandlePosition ();

	sumOffset = Point ();
	pad->setMouseState (View::kMouseDown);
	pad->beginEditing ();
	
	onMove (0);
} 

//////////////////////////////////////////////////////////////////////////////////////////////////

void TriVectorPadMouseHandler::onRelease (bool canceled)
{
	pad->endEditing ();
	pad->setMouseState (View::kMouseNone);
	tooltipPopup.reserve (false);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool TriVectorPadMouseHandler::onMove (int moveFlags)
{
	int modifiers = current.keys.getModifiers ();
	bool fineMode = (modifiers & KeyState::kShift) != 0;
	bool editSumMode = (modifiers & KeyState::kOption) != 0;
	
	if(editSumMode && pad->getStyle ().isCustomStyle (Styles::kTriVectorPadBehaviorKeepLevel))
	{
		double currentYDistance = previous.where.y - current.where.y;
		double delta = currentYDistance / (double)(pad->getSize ().getHeight ());
		
		if(delta != 0)
		{
			if(fineMode)
				delta *= 0.05f;
			
			double newValue = ccl_bound<double> (pad->getKeptSum () + delta, 0., 3.);
			
			pad->setKeptSum (newValue);
			pad->setHandlePosition (sumStartWhere);
		}
		
		sumOffset = current.where - sumStartWhere;
	}	
	else	
	{
		if(wasFineMode != fineMode)
		{
			fineWhere = current.where;
			wasFineMode = fineMode;
		}

		Point p (current.where.x, current.where.y);
		p.x -= (clickOffset.x + sumOffset.x);
		p.y -= (clickOffset.y + sumOffset.y);
		
		if(fineMode)
		{
			float deltaX = float (current.where.x - fineWhere.x);
			float deltaY = float (current.where.y - fineWhere.y);
			p.x = ((float)fineWhere.x + 0.05f * deltaX - clickOffset.x);
			p.y = ((float)fineWhere.y + 0.05f * deltaY - clickOffset.y);
		}

		if(!pad->isInsideTriangle (p))
			pad->calcTriangleIntersection (p);
			
		sumStartWhere = p;
		
		pad->setHandlePosition (p);
	}
	
	updateTooltip ();
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TriVectorPadMouseHandler::updateTooltip () 
{
	if(view->getStyle ().isCustomStyle (Styles::kTriVectorPadBehaviorTooltip))
	{
		IParameter* xP = pad->getParameter ();
		IParameter* yP = pad->getYParameter ();
		IParameter* zP = pad->getZParameter ();
		
		if(xP && yP && zP)
		{
			String text;
			String ytext;
			String ztext;
			text.append ("[");
			xP->toString (ytext);
			text.append (ytext);
			text.append (" | ");
			yP->toString (ytext);
			text.append (ytext);
			text.append (" | ");
			zP->toString (ztext);
			text.append (ztext);
			text.append ("]");

			tooltipPopup.setTooltip (text);
			tooltipPopup.reserve (true);
		}
	}
}
