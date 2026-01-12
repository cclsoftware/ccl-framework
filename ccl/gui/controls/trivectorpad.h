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
// Filename    : ccl/gui/controls/trivectorpad.h
// Description : Triangular Vector Pad (XYZ-Control)
//
//************************************************************************************************

#ifndef _ccl_trivectorpad_h
#define _ccl_trivectorpad_h

#include "ccl/gui/controls/control.h"

#include "ccl/public/gui/graphics/3d/point3d.h"

namespace CCL {

//************************************************************************************************
// TriVectorPad
/** A Control to edit three parameters in a triangle. The summed-up value of these parameters is always 1.
 With the "invert" option the sum is 2 in the corners and 1 at the snappoints (the edge-center points). 
 If this control is used in addition with individual level controls, the "keeplevels" option allows 
 different sums and reset the parameters to the original values when the handle is moved back to the initial position. 
 (Alt/Option)+drag up-down can change the overall level in the "keeplevel" case */
//************************************************************************************************

class TriVectorPad: public Control
{
public:
	DECLARE_CLASS (TriVectorPad, Control)

	TriVectorPad (RectRef size = Rect (), IParameter* param = nullptr, IParameter* yParam = nullptr, IParameter* zParam = nullptr, StyleRef style = 0);
	~TriVectorPad ();

	enum CornerID
	{
		kCornerA,
		kCornerB,
		kCornerC
	};
	
	enum SideID
	{
		kSideAB,
		kSideBC,
		kSideCA
	};
	
	enum PartCodes
	{
		kPartSnapPointA,
		kPartSnapPointB,
		kPartSnapPointC,
		kPartSnapPointAB,
		kPartSnapPointBC,
		kPartSnapPointCA,
		kFirstSnapPoint = kPartSnapPointA,
		kLastSnapPoint = kPartSnapPointCA,
		kPartHandle,
		kPartTriangle,
		kPartHoverTriangle,
		kNumPartCodes
	};
	
	enum OuterSectionID
	{
		kOutSnapA,
		kOutSnapB,
		kOutSnapC,
		kOutAB,
		kOutBC,
		kOutCA
	};

	DECLARE_STYLEDEF (customStyles)
	
	IParameter* getYParameter () const { return yParam; }
	IParameter* getZParameter () const { return zParam; }

	void beginEditing ();
	void endEditing ();

	double getKeptSum () const { return keptSum; }
	void setKeptSum (double sum);
	
	bool isInsideTriangle (PointRef where, bool hoverTriangle = false) const;
	void calcTriangleIntersection (Point& p) const;
	void setHandlePosition (PointRef p);
	
	Point getHandlePosition () const;
	Point getTrianglePoint (CornerID code, bool hoverTriangle = false) const;
	Point getSnapPoint (int partCode) const;
	Point getTriangleSidePoint (SideID code) const;
	int getHighlightSnapPointCode () const { return highlightSnapPoint; }
	
	// Control
	void CCL_API notify (ISubject* subject, MessageRef msg) override;
	bool onMouseEnter (const MouseEvent& event) override;
	bool onMouseMove (const MouseEvent& event) override;
	bool onMouseDown (const MouseEvent& event) override;
	bool onMouseLeave (const MouseEvent& event) override;
	bool onMouseWheel (const MouseWheelEvent& event) override;
	ThemeRenderer* getRenderer () override;
	MouseHandler* createMouseHandler (const MouseEvent& event) override;
	bool onContextMenu (const ContextMenuEvent& event) override;
	void paramChanged () override;
	void performReset () override;
	
protected:
	bool keepLevels;
	bool upsideDown;
	bool inverted;
	bool editing;
	bool keepSnapPointHighlight;
	IParameter* yParam;
	IParameter* zParam;
	double keptSum;
	double xOver;
	double yOver;
	double zOver;
	double snapPointValueFactor;
	int highlightSnapPoint;
	Point explicitHandlePosition;
	
	double getXValue () const;
	void setXValue (double v, bool update = true);
	double getYValue () const;
	void setYValue (double v, bool update = true);
	double getZValue () const;
	void setZValue (double v, bool update = true);
	void setNormalizedValue (IParameter* p, double v, bool update);
	
	Point getHandlePositionFromValues () const;
	Point getTriangleCenterOfMass () const;
	Point convertBarycentricToCartesian (PointF3DRef b) const;
	Point convertNormalizedToCartesian (PointF& a) const;
	PointF3D convertCartesianToBarycentric (PointRef p) const;
	PointF convertCartesianToNormalized (PointRef p) const;
	bool setValuesFromBarycentric (PointF3DRef b);
	PointF3D getBarycentricFromValues () const;
	PointF3D validateBarycentric (double& a, double& b, double& c) const;
	void calcSnapPointValueFactor (PointF3DRef b);
	PointF3D getValuesAtNearestSnapPoint (double xs, double ys, double zs) const;
	
	void setHighlightSnapPoint (int partCode);
	bool hitHandleOrSnapPoint (int& partCode, const MouseEvent& event);
	void setExplicitHandlePosition (PointRef p);
	void resetExplicitHandlePosition ();
	bool isSnapPointCode (int partCode) const;
	OuterSectionID calcOuterSectionID (PointRef where) const;
	CornerID getCornerIDForSection (OuterSectionID code) const;
	void getBasisForCornerID (Point& start, Point& end, CornerID cornerPoint) const;
	int getNearestSnapPoint (PointRef where) const;
	IParameter* getParameterForSnapPoint (int snapPoint) const;
	IParameter* getNearestParameter (PointRef where) const;
};

} // namespace CCL

#endif // _ccl_trivectorpad_h
