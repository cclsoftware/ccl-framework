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
// Filename    : ccl/gui/theme/renderer/labelrenderer.cpp
// Description : Label Renderer
//
//************************************************************************************************

#ifndef _ccl_labelrenderer_h
#define _ccl_labelrenderer_h

#include "ccl/gui/theme/themerenderer.h"

namespace CCL {

//************************************************************************************************
// LabelRenderer
//************************************************************************************************

class LabelRenderer: public ThemeRenderer
{
public:
	LabelRenderer (VisualStyle* visualStyle);
	
	void draw (View* view, const UpdateRgn& updateRgn) override;
	int hitTest (View* view, const Point& loc, Point* clickOffset) override;
	bool getPartRect (const View* view, int partCode, CCL::Rect& rect) override;

protected:
	
	Point offset;
};

DECLARE_VISUALSTYLE_CLASS (Label)

} // namespace CCL

#endif // _ccl_theme_h
