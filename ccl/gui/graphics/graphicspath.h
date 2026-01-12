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
// Filename    : ccl/gui/graphics/graphicspath.h
// Description : Graphical Path
//
//************************************************************************************************

#ifndef _ccl_graphicspath_h
#define _ccl_graphicspath_h

#include "ccl/base/object.h"

#include "ccl/public/gui/graphics/igraphicspath.h"

namespace CCL {

class NativeGraphicsPath;

//************************************************************************************************
// GraphicsPath
//************************************************************************************************

class GraphicsPath: public Object,
					public IGraphicsPath
{
public:
	DECLARE_CLASS (GraphicsPath, Object)

	GraphicsPath (TypeHint type = kPaintPath);
	GraphicsPath (const GraphicsPath&);
	~GraphicsPath ();

	NativeGraphicsPath& getNativePath () { return *nativePath; }
	operator IGraphicsPath* () { return this; }

	// IGraphicsPath
	void CCL_API setFillMode (FillMode fillMode) override;
	void CCL_API getBounds (Rect& bounds) const override;
	void CCL_API getBounds (RectF& bounds) const override;
	void CCL_API transform (TransformRef matrix) override;
	void CCL_API startFigure (PointFRef p) override;
	void CCL_API startFigure (PointRef p) override;
	void CCL_API closeFigure () override;
	void CCL_API lineTo (PointRef p) override;
	void CCL_API lineTo (PointFRef p) override;
	void CCL_API addRect (RectRef rect) override;
	void CCL_API addRect (RectFRef rect) override;
	void CCL_API addRoundRect (RectRef rect, Coord rx, Coord ry) override;
	void CCL_API addRoundRect (RectFRef rect, CoordF rx, CoordF ry) override;
	void CCL_API addTriangle (PointRef p1, PointRef p2, PointRef p3) override;
	void CCL_API addTriangle (PointFRef p1, PointFRef p2, PointFRef p3) override;
	void CCL_API addBezier (PointRef p1, PointRef c1, PointRef c2, PointRef p2) override;
	void CCL_API addBezier (PointFRef p1, PointFRef c1, PointFRef c2, PointFRef p2) override;
	void CCL_API addArc (RectRef rect, float startAngle, float sweepAngle) override;
	void CCL_API addArc (RectFRef rect, float startAngle, float sweepAngle) override;

	CLASS_INTERFACE (IGraphicsPath, Object)

private:
	NativeGraphicsPath* nativePath;
};

} // namespace CCL

#endif // _ccl_graphicspath_h
