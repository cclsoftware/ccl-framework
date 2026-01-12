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
// Filename    : ccl/gui/theme/renderer/selectboxrenderer.cpp
// Description : Control Renderer
//
//************************************************************************************************

#ifndef _ccl_selectboxrenderer_h
#define _ccl_selectboxrenderer_h

#include "ccl/gui/theme/renderer/compositedrenderer.h"
#include "ccl/public/gui/framework/styleflags.h"

namespace CCL {

interface IColorParam;

//************************************************************************************************
// SelectBoxRenderer
//************************************************************************************************

class SelectBoxRenderer: public CompositedRenderer
{	
public:
	SelectBoxRenderer (VisualStyle* visualStyle);

	void draw (View* view, const UpdateRgn& updateRgn) override;
	int hitTest (View* view, const Point& loc, Point* clickOffset) override;
	bool getPartRect (const View* view, int partCode, CCL::Rect& rect) override;

protected:
	SharedPtr<IImage> background;
	SharedPtr<IImage> button;
	SharedPtr<IImage> states;
	float imageFillSize;
	float buttonFillSize;
	float textColorThreshold;
	Rect padding;
	Color textColor;
	Color textColorOn;
	Color textColorBright;
	Color textColorDisabled;
	Color textColorMouseOver;
	Color contextColor;
	Color contextColorOn;
	bool hideText;
	bool hideButton;
	bool hideImage;
	bool buttonBeneath;
	bool leadingButton;
	bool trailingButton;
	bool hasOffState;
	TextScaler textScaler;
	bool initialized;
	
	void initialize (StyleRef style, const View* view);	
	bool needsBrightText (IColorParam* colorParam) const;
};

DECLARE_VISUALSTYLE_CLASS (SelectBox)

} // namespace CCL

#endif // _ccl_theme_h
