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
#include "ccl/gui/controls/textmodel.h"

using namespace CCL;

//************************************************************************************************
// EditBoxRenderer
/** An EditBox is drawn just like a TextBox. */
//************************************************************************************************

DEFINE_TYPELIB_ENTRY_WITH_PARENT (EditBox, TextBox, "EditBoxStyle", VISUALSTYLES_TYPELIB)

//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_CLASS_ABSTRACT (EditBoxRenderer, TextBoxRenderer)

//////////////////////////////////////////////////////////////////////////////////////////////////

EditBoxRenderer::EditBoxRenderer (VisualStyle* visualStyle, View* owner)
: TextBoxRenderer (visualStyle, owner),
  placeholderTextModel (nullptr),
  selectionAtom (AtomName::kSelection),
  placeholderAtom (AtomName::kPlaceholder)
{
	addAtom (selectionAtom);
	addAtom (placeholderAtom);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

EditBoxRenderer::~EditBoxRenderer ()
{
	safe_release (placeholderTextModel);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void EditBoxRenderer::draw (const UpdateRgn& updateRgn)
{
	// Prepare text layouts so that text layout measurements can be used to update atoms.
	for(RenderAtom* atom : atoms)
		atom->updateTextLayout ();

	updateAtoms ();

	EditBox* editBox = static_cast<EditBox*> (owner);
	editBox->setCaretColor (textAtom.getBrush ().getColor ());

	StyleRef style = owner->getStyle ();
	if(style.isCustomStyle (Styles::kTextBoxAppearanceHideText) && !editBox->isFocused ())
		return;

	// updateAtoms can change positions and sizes, so prepare the text layouts again for drawing.
	for(RenderAtom* atom : atoms)
		atom->updateTextLayout ();

	editBox->updateDisplayOffset ();

	GraphicsPort port (owner);
	backgroundAtom.draw (port, updateRgn);

	Rect r (editBox->getSize ());
	r.moveTo (Point (0, 0));

	port.saveState ();
	port.addClip (r);
	Transform t;
	t.translate (-editBox->getDisplayOffset (), 0);
	port.addTransform (t);

	if(!selectionAtom.isHidden ())
	{
		for(RectRef rect : editBox->getSelection ())
		{
			selectionAtom.setSize (rectIntToF (rect).offset (textAtom.getSize ().getLeftTop ()));
			selectionAtom.updateTextLayout ();
			selectionAtom.draw (port, updateRgn);
		}
	}

	if(!placeholderAtom.isHidden ())
		placeholderAtom.draw (port, updateRgn);

	textAtom.draw (port, updateRgn);

	port.restoreState ();

	editBox->updateCaretRect ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool EditBoxRenderer::getPartRect (Rect& r, int partCode)
{
	if(partCode == EditBox::kPartNativeControlArea)
	{
		TextBoxRenderer::getPartRect (r, TextBox::kPartContentArea);

		r.left	 += nativePadding.left;
		r.right	 -= nativePadding.right;
		r.top	 += nativePadding.top;
		r.bottom -= nativePadding.bottom;
		return true;
	}
	return TextBoxRenderer::getPartRect (r, partCode);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void EditBoxRenderer::updateAtoms ()
{
	SuperClass::updateAtoms ();

	bool multiline = owner->getStyle ().isCustomStyle (Styles::kTextBoxAppearanceMultiLine);
	const ITextLayout* textLayout = getTextLayout ();
	if(!multiline && textLayout)
	{
		RectF bounds;
		textLayout->getBounds (bounds);
		int newAlignH = owner->getVisualStyle ().getTextAlignment ().getAlignH ();
		if(bounds.getWidth () > textAtom.getSize ().getWidth ())
			newAlignH = Alignment::kLeft;

		TextFormat format = textAtom.getTextFormat ();
		if(format.getAlignment ().getAlignH () != newAlignH)
		{
			Alignment alignment (format.getAlignment ());
			alignment.setAlignH (newAlignH);
			format.setAlignment (alignment);
			textAtom.setTextFormat (format);
			textAtom.needsTextLayoutRebuild (true);
		}
	}

	placeholderAtom.setSize (textAtom.getSize ());
	placeholderAtom.setTextFormat (textAtom.getTextFormat ());
	const EditBox* editBox = static_cast<const EditBox*> (owner);
	placeholderAtom.isHidden (editBox->getPlaceholderString ().isEmpty () || editBox->getTextLength () > 0 || editBox->isEditing ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void EditBoxRenderer::applyVisualStyle (RenderAtomTransaction& transaction)
{
	if(visualStyle == nullptr)
		return;

	TextBoxRenderer::applyVisualStyle (transaction);

	const EditBox* editBox = static_cast<const EditBox*> (owner);
	placeholderAtom.isHidden (true);
	if(!editBox->getPlaceholderString ().isEmpty ())
	{
		if(placeholderTextModel == nullptr)
			placeholderTextModel = NEW StringTextModel ();

		placeholderTextModel->fromParamString (editBox->getPlaceholderString ());
		placeholderAtom.setTextModel (placeholderTextModel);
	}

	#if LEGACY_STYLES_SUPPORTED
	BEGIN_LEGACY_STYLES_ENABLED_BLOCK
	Color selectionColor;
	if(visualStyle->getColor (selectionColor, LegacyStyleID::kSelectionColor))
		selectionAtom.setBrush (SolidBrush (selectionColor));

	Color placeholderColor (visualStyle->getTextColor ());
	if(visualStyle->getColor (placeholderColor, LegacyStyleID::kTextColorPlaceholder) || !placeholderAtom.getBrush ().isVisible ())
		placeholderAtom.setBrush (SolidBrush (placeholderColor));

	placeholderAtom.setTextFont (visualStyle->getFont (LegacyStyleID::kTextFontPlaceholder, visualStyle->getTextFont ()));
	placeholderAtom.needsTextLayoutRebuild (true);

	Coord np = visualStyle->getMetric<Coord> (LegacyStyleID::kNativePadding, 0);
	nativePadding.left = visualStyle->getMetric<Coord> (LegacyStyleID::kNativePaddingLeft, np);
	nativePadding.top = visualStyle->getMetric<Coord> (LegacyStyleID::kNativePaddingTop, np);
	nativePadding.right = visualStyle->getMetric<Coord> (LegacyStyleID::kNativePaddingRight, np);
	nativePadding.bottom = visualStyle->getMetric<Coord> (LegacyStyleID::kNativePaddingBottom, np);
	END_LEGACY_STYLES_ENABLED_BLOCK
	#endif

	if(!selectionAtom.getBrush ().isVisible ())
		selectionAtom.setBrush (SolidBrush (owner->getTheme ().getThemeColor (ThemeElements::kAlphaSelectionColor)));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool EditBoxRenderer::isOpaque () const
{
	const EditBox* editBox = static_cast<const EditBox*> (owner);
	StyleRef style = owner->getStyle ();
	return style.isOpaque () || (style.isCustomStyle (Styles::kEditBoxAppearanceOpaqueEdit) && editBox->isEditing ());
}
