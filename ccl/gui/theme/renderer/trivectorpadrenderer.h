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
// Filename    : ccl/gui/theme/renderer/trivectorpadrenderer.h
// Description : Triangular Vector Pad Renderer
//
//************************************************************************************************

#ifndef _ccl_trivectorpadrenderer_h
#define _ccl_trivectorpadrenderer_h

#include "ccl/gui/theme/themerenderer.h"

namespace CCL {

//************************************************************************************************
// TriVectorPadRenderer
//************************************************************************************************

class TriVectorPadRenderer: public ThemeRenderer
{
public:
	TriVectorPadRenderer (VisualStyle* visualStyle);

	void draw (View* view, const UpdateRgn& updateRgn) override;
	int hitTest (View* view, const Point& loc, Point* offset = nullptr) override;
	bool getPartRect (const View* view, int partCode, Rect& rect) override;

protected:
	SharedPtr<IImage> background;
	SharedPtr<IImage> handleImage;
	SharedPtr<IImage> triangleImage;
	SharedPtr<IImage> snapPointImage;
	float handleThickness;
	int handleSize;
	int snapPointSize;
	Color backcolor;
	Color triangleColor;
	Color handleColor;
	Color snapPointColor;
	Rect padding;
	Rect hoverPadding;
	
	bool getHandleRect (const View* view, Rect& rect) const;
	bool getTriangleRect (const View* view, Rect& rect) const;
	bool getHoverTriangleRect (const View* view, Rect& rect) const;
	bool getSnapPointRect (const View* view, int partCode, Rect& rect) const;
	void drawTriangleShape (View* view);
	int getFrameForSnapPointCode (int code) const;
};

DECLARE_VISUALSTYLE_CLASS (TriVectorPad)

} // namespace CCL

#endif // _ccl_trivectorpadrenderer_h
