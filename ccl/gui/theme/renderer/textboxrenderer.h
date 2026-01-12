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
// Filename    : ccl/gui/theme/renderer/textboxrenderer.cpp
// Description : Control Renderer
//
//************************************************************************************************

#ifndef _ccl_textboxrenderer_h
#define _ccl_textboxrenderer_h

#include "ccl/gui/theme/renderer/compositedrenderer.h"
#include "ccl/public/gui/framework/styleflags.h"

namespace CCL {

class GraphicsPort;
interface IColorParam;

//************************************************************************************************
// TextBoxRenderer
//************************************************************************************************

class TextBoxRenderer: public CompositedRenderer
{
public:
	TextBoxRenderer (VisualStyle* visualStyle);
		
	// CompositedRenderer
	void draw (View* view, const UpdateRgn& updateRgn) override;
	int hitTest (View* view, const Point& loc, Point* clickOffset) override;
	bool getPartRect (const View* view, int partCode, Rect& rect) override;

protected:
	SharedPtr<IImage> background;
	SolidBrush textBrush;
	Rect padding;
	Color textColorOpaque;
	Color textContrastTransparentColor;
	Color textContrastBrightColor;
	Color textContrastDarkColor;
	Color alphaBlendColor;
	Color disabledTextColor;
	Color borderPenColor;
	mutable Color colorParamColor;

	float brightColorThreshold;
	bool initialized;
	
	virtual void initialize (StyleRef style);
	ColorRef getTextColor (IColorParam* colorParam) const;
	virtual bool drawLayout (View* view, GraphicsPort& port, ITextLayout* layout, BrushRef textBrush);
	virtual bool isOpaque (const View* view) const;
};

DECLARE_VISUALSTYLE_CLASS (TextBox)

} // namespace CCL

#endif // _ccl_textboxrenderer_h
