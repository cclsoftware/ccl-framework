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
	LabelRenderer (VisualStyle* visualStyle, View* owner);
	
	// ThemeRenderer
	void draw (const UpdateRgn& updateRgn) override;
	int hitTest (PointRef where, Point* clickOffset = nullptr) override;
	bool getPartRect (Rect& rect, int partCode) override;

protected:
	Point offset;
};

DECLARE_TYPELIB_ENTRY (Label)

} // namespace CCL

#endif // _ccl_theme_h
