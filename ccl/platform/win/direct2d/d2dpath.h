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
// Filename    : ccl/platform/win/direct2d/d2dpath.h
// Description : Direct2D Path
//
//************************************************************************************************

#ifndef _ccl_direct2d_path_h
#define _ccl_direct2d_path_h

#include "ccl/platform/win/direct2d/d2dbase.h"

#include "ccl/gui/graphics/nativegraphics.h"

namespace CCL {
namespace Win32 {

//************************************************************************************************
// D2DPathGeometry
//************************************************************************************************

class D2DPathGeometry: public NativeGraphicsPath,
					   private D2DResource
{
public:
	DECLARE_CLASS (D2DPathGeometry, NativeGraphicsPath)

	D2DPathGeometry (TypeHint type = IGraphicsPath::kPaintPath);
	~D2DPathGeometry ();

	ID2D1PathGeometry* getID2D1Path ();

	// NativeGraphicsPath
	tresult draw (NativeGraphicsDevice& device, PenRef pen) override;
	tresult fill (NativeGraphicsDevice& device, BrushRef brush) override;
	void CCL_API getBounds (Rect& bounds) const override;
	void CCL_API getBounds (RectF& bounds) const override;
	void CCL_API transform (TransformRef matrix) override;
	void CCL_API startFigure (PointFRef p) override;
	void CCL_API closeFigure () override;
	void CCL_API lineTo (PointFRef p) override;
	void CCL_API addRect (RectRef rect) override;
	void CCL_API addRect (RectFRef rect) override;
	void CCL_API addRoundRect (RectRef rect, Coord rx, Coord ry) override;
	void CCL_API addRoundRect (RectFRef rect, CoordF rx, CoordF ry) override;
	void CCL_API addBezier (PointRef p1, PointRef c1, PointRef c2, PointRef p2) override;
	void CCL_API addBezier (PointFRef p1, PointFRef c1, PointFRef c2, PointFRef p2) override;
	void CCL_API addArc (RectRef rect, float startAngle, float sweepAngle) override;
	void CCL_API addArc (RectFRef rect, float startAngle, float sweepAngle) override;
	void CCL_API addTriangle (PointRef p1, PointRef p2, PointRef p3) override;
	void CCL_API addTriangle (PointFRef p1, PointFRef p2, PointFRef p3) override;
	void CCL_API setFillMode (FillMode fillMode) override;

private:	
	TypeHint type;
	ComPtr<ID2D1PathGeometry> pathGeometry;
	ComPtr<ID2D1GeometrySink> sink;
	bool wasOpen;
	bool figureStarted;	
	bool rebuildNeeded;
	PointF currentPos;
	D2D1_FILL_MODE fillMode;

	void closeSink ();
	bool openSink ();
	void checkRebuild ();

	// D2DResource
	void discardDirect2dResource (bool isShutdown) override;
};

} // namespace Win32
} // namespace CCL

#endif // _ccl_direct2d_path_h
