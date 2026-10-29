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
// Filename    : ccl/gui/theme/renderer/dialoggrouprenderer.cpp
// Description : DialogGroup Renderer
//
//************************************************************************************************

#ifndef _ccl_dialoggrouprenderer_h
#define _ccl_dialoggrouprenderer_h

#include "ccl/gui/theme/themerenderer.h"

namespace CCL {

//************************************************************************************************
// DialogGroupRenderer
//************************************************************************************************

class DialogGroupRenderer: public ThemeRenderer
{
public:
	DialogGroupRenderer (VisualStyle* visualStyle, View* owner);

	PROPERTY_VARIABLE (int, headerGap, HeaderGap)
	PROPERTY_VARIABLE (Color, headerlinecolor, HeaderLineColor)

	// ThemeRenderer
	void draw (const UpdateRgn& updateRgn) override;
	int hitTest (PointRef where, Point* clickOffset = nullptr) override;
	bool getPartRect (Rect& rect, int partCode) override;

protected:
	SharedPtr<IImage> image;
	SharedPtr<IImage> secondaryImage;
};

DECLARE_TYPELIB_ENTRY (DialogGroup)

} // namespace CCL

#endif // _ccl_theme_h
