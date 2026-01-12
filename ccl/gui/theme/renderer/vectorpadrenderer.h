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
// Filename    : ccl/gui/theme/renderer/vectorpadrenderer.h
// Description : VectorPad Renderer
//
//************************************************************************************************

#ifndef _ccl_vectorpadrenderer_h
#define _ccl_vectorpadrenderer_h

#include "ccl/gui/theme/themerenderer.h"

namespace CCL {

//************************************************************************************************
// VectorPadRenderer
//************************************************************************************************

class VectorPadRenderer: public ThemeRenderer
{
public:
	VectorPadRenderer (VisualStyle* visualStyle);

	void draw (View* view, const UpdateRgn& updateRgn) override;
	int hitTest (View* view, const Point& loc, Point* clickOffset) override;
	bool getPartRect (const View* view, int partCode, Rect& rect) override;

private:
	bool getHandleRect (const View* view, Rect& rect);
	void selectBackgroundFrame ();
	
	SharedPtr<IImage> back;
	SharedPtr<IImage> handle;
	
	Color backColor;
	Color handleColor;
	Color textColor;
	Color crosshairColor;
	Color referenceBackcolor;

	Font font;

	float handleThickness;
	int handleSize;
	bool drawLabels;
	bool drawCrosshair;
};

DECLARE_VISUALSTYLE_CLASS (VectorPad)

} // namespace CCL

#endif // _ccl_vectorpadrenderer_h
