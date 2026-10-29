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
	VectorPadRenderer (VisualStyle* visualStyle, View* owner);

	// ThemeRenderer
	void draw (const UpdateRgn& updateRgn) override;
	int hitTest (PointRef where, Point* clickOffset = nullptr) override;
	bool getPartRect (Rect& rect, int partCode) override;

private:	
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

	bool getHandleRect (Rect& rect);
	void selectBackgroundFrame ();
};

DECLARE_TYPELIB_ENTRY (VectorPad)

} // namespace CCL

#endif // _ccl_vectorpadrenderer_h
