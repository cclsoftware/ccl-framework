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
// Filename    : ccl/platform/cocoa/quartz/path.h
// Description : Quartz Path
//
//************************************************************************************************

#ifndef _ccl_quartz_path_h
#define _ccl_quartz_path_h

#include "ccl/gui/graphics/nativegraphics.h"

#include "ccl/public/base/ccldefpush.h"
#include <CoreGraphics/CGPath.h>
#include <CoreGraphics/CGContext.h>
#include "ccl/public/base/ccldefpop.h"

namespace CCL {
namespace MacOS {

//************************************************************************************************
// QuartzPath
//************************************************************************************************

class QuartzPath: public NativeGraphicsPath
{
public:
	DECLARE_CLASS (QuartzPath, NativeGraphicsPath)

	QuartzPath ();
	QuartzPath (const QuartzPath&);
	~QuartzPath ();

	CGMutablePathRef getCGPath () { return path; }

	// NativeGraphicsPath
	tresult draw (NativeGraphicsDevice& device, PenRef pen) override;
	tresult fill (NativeGraphicsDevice& device, BrushRef brush) override;
	void CCL_API getBounds (Rect& bounds) const override;
	void CCL_API transform (TransformRef matrix) override;
	void CCL_API startFigure (PointFRef p) override;
	void CCL_API closeFigure () override;
	void CCL_API lineTo (PointRef p) override;		
	void CCL_API lineTo (PointFRef p) override;		
	void CCL_API addRect (RectRef rect) override;
	void CCL_API addRoundRect (RectRef rect, Coord rx, Coord ry) override;
	void CCL_API addBezier (PointRef p1, PointRef c1, PointRef c2, PointRef p2) override;
	void CCL_API addArc (RectRef rect, float startAngle, float sweepAngle) override;

	void CCL_API getBounds (RectF& bounds) const override;
	void CCL_API addRect (RectFRef rect) override;
	void CCL_API addRoundRect (RectFRef rect, CoordF rx, CoordF ry) override;
	void CCL_API addBezier (PointFRef p1, PointFRef c1, PointFRef c2, PointFRef p2) override;
	void CCL_API addArc (RectFRef rect, float startAngle, float sweepAngle) override;
	void CCL_API setFillMode (FillMode fillMode) override;

protected:
	bool started;
	CGMutablePathRef path;
	CGPathDrawingMode fillMode;
};

} // namespace MacOS
} // namespace CCL

#endif // _ccl_quartz_path_h
