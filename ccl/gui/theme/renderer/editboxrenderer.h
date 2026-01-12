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
// Filename    : ccl/gui/theme/renderer/editboxrenderer.cpp
// Description : Edit Box Renderer
//
//************************************************************************************************

#ifndef _ccl_editboxrenderer_h
#define _ccl_editboxrenderer_h

#include "ccl/gui/theme/renderer/textboxrenderer.h"

namespace CCL {

//************************************************************************************************
// EditBoxRenderer
//************************************************************************************************

class EditBoxRenderer: public TextBoxRenderer
{
public:
	EditBoxRenderer (VisualStyle* visualStyle);

	// TextBoxRenderer
	void draw (View* view, const UpdateRgn& updateRgn) override;

protected:
	bool colorsInitialized;
	Rect nativePadding;
	Color selectionColor;
	Color placeholderColor;
	Font placeholderFont;
		
	void initialize (StyleRef style) override;
	void initializeColors (const View* view);
	
	// TextBoxRenderer
	bool drawLayout (View* view, GraphicsPort& port, ITextLayout* layout, BrushRef textBrush) override;
	bool getPartRect (const View* view, int partCode, Rect& rect) override;
	bool isOpaque (const View* view) const override;
};

DECLARE_VISUALSTYLE_CLASS (EditBox)

} // namespace CCL

#endif // _ccl_editboxrenderer_h
