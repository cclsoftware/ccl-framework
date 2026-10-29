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
// Filename    : ccl/gui/theme/renderer/headerviewrenderer.cpp
// Description : HeaderView Renderer
//
//************************************************************************************************

#ifndef _ccl_headerrenderer_h
#define _ccl_headerrenderer_h

#include "ccl/gui/theme/themerenderer.h"

#include "ccl/gui/itemviews/headerview.h"

namespace CCL {

//************************************************************************************************
// HeaderViewRenderer
//************************************************************************************************

class HeaderViewRenderer: public ThemeRenderer,
						  public IHeaderViewRenderer
{	
public:
	HeaderViewRenderer (VisualStyle* visualStyle, View* owner);

	void draw (const UpdateRgn& updateRgn) override;

	// IHeaderViewRenderer
	void drawHeader (GraphicsDevice& port, RectRef r, StringRef label, BrushRef textBrush, FontRef font) override;

	// ThemeRenderer
	int hitTest (PointRef where, Point* clickOffset = nullptr) override;
	bool getPartRect (Rect& rect, int partCode) override;

	CLASS_INTERFACE (IHeaderViewRenderer, ThemeRenderer)

protected:
	static const int kTextInset = 2;

	SharedPtr<IImage> image;
	int columnSpacing;
	Color columnSpacingColor;
	Color columnSizableSpacingColor;
	
	void drawHeader (IGraphics& graphics, RectRef r, ColumnHeader& c, BrushRef textBrush, FontRef font);
};

DECLARE_TYPELIB_ENTRY (HeaderView)

} // namespace CCL

#endif // _ccl_headerrenderer_h
