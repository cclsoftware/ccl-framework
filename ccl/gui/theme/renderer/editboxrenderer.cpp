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

#include "ccl/gui/theme/renderer/editboxrenderer.h"

#include "ccl/gui/controls/editbox.h"

using namespace CCL;

//************************************************************************************************
// EditBoxRenderer
/** An EditBox is drawn just like a TextBox. */
//************************************************************************************************

BEGIN_VISUALSTYLE_CLASS (EditBox, TextBox, "EditBoxStyle")
	ADD_VISUALSTYLE_COLOR ("selectioncolor")		 ///< background color for selected ranges
	ADD_VISUALSTYLE_COLOR ("textcolor.placeholder")  ///< dedicated color for optional placeholder string
	ADD_VISUALSTYLE_FONT ("textfont.placeholder")  	 ///< dedicated font for optional placeholder string
	ADD_VISUALSTYLE_METRIC ("native.padding.left")	 ///< left padding for the native text control
	ADD_VISUALSTYLE_METRIC ("native.padding.top")	 ///< top padding for the native text control
	ADD_VISUALSTYLE_METRIC ("native.padding.right")	 ///< right padding for the native text control
	ADD_VISUALSTYLE_METRIC ("native.padding.bottom") ///< bottom padding for the native text control
	ADD_VISUALSTYLE_METRIC ("native.padding")		 ///< padding for the native text control, used if one of the paddings for left, top, right, bottom is not specified
END_VISUALSTYLE_CLASS (EditBox)

//////////////////////////////////////////////////////////////////////////////////////////////////

EditBoxRenderer::EditBoxRenderer (VisualStyle* visualStyle)
: TextBoxRenderer (visualStyle),
  colorsInitialized (false)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void EditBoxRenderer::draw (View* view, const UpdateRgn& updateRgn)
{
	if(!colorsInitialized)
		initializeColors (view);

	TextBoxRenderer::draw (view, updateRgn);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool EditBoxRenderer::drawLayout (View* view, GraphicsPort& port, ITextLayout* layout, BrushRef textBrush)
{
	EditBox* editBox = static_cast<EditBox*> (view);

	StyleRef style = view->getStyle ();
	if(style.isCustomStyle (Styles::kEditBoxAppearanceHideText) && !editBox->isFocused ())
		return true;

	StringRef placeholder = editBox->getPlaceholderString ();
	if(!placeholder.isEmpty () && layout->getText ().isEmpty () && !editBox->isEditing ()) 
		port.drawString (editBox->getTextRect (), placeholder, placeholderFont, SolidBrush (placeholderColor), Alignment::kCenter);
		
	Color brushColor (selectionColor);
	if(!editBox->isFocused ())
		brushColor.grayScale ();

	SolidBrush selectionBrush (brushColor);
	for(Rect rect : editBox->getSelection ())
		port.fillRect (rect.offset (editBox->getTextRect ().getLeftTop ()), selectionBrush);

	return TextBoxRenderer::drawLayout (view, port, layout, textBrush);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool EditBoxRenderer::getPartRect (const View* view, int partCode, CCL::Rect& r)
{
	if(partCode == EditBox::kPartNativeControlArea)
	{
		TextBoxRenderer::getPartRect (view, TextBox::kPartContentArea, r);

		r.left	 += nativePadding.left;
		r.right	 -= nativePadding.right;
		r.top	 += nativePadding.top;
		r.bottom -= nativePadding.bottom;
		return true;
	}
	return TextBoxRenderer::getPartRect (view, partCode, r);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void EditBoxRenderer::initialize (StyleRef style)
{
	if(visualStyle == nullptr)
		return;
		
	placeholderColor = visualStyle->getColor ("textcolor.placeholder", visualStyle->getTextColor ());
	placeholderFont = visualStyle->getFont ("textfont.placeholder", visualStyle->getTextFont ());

	Coord np = visualStyle->getMetric<Coord> ("native.padding", 0);
	nativePadding.left = visualStyle->getMetric<Coord> ("native.padding.left", np);
	nativePadding.top = visualStyle->getMetric<Coord> ("native.padding.top", np);
	nativePadding.right = visualStyle->getMetric<Coord> ("native.padding.right", np);
	nativePadding.bottom = visualStyle->getMetric<Coord> ("native.padding.bottom", np);
	
	TextBoxRenderer::initialize (style);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void EditBoxRenderer::initializeColors (const View* view)
{
	if(visualStyle == nullptr)
		return;

	selectionColor = visualStyle->getColor ("selectioncolor", view->getTheme ().getThemeColor (ThemeElements::kSelectionColor));
	colorsInitialized = true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool EditBoxRenderer::isOpaque (const View* view) const
{
	const EditBox* editBox = static_cast<const EditBox*> (view);
	StyleRef style = view->getStyle ();
	return style.isOpaque () || (style.isCustomStyle (Styles::kEditBoxAppearanceOpaqueEdit) && editBox->isEditing ());
}
