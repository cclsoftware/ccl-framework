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
// Filename    : ccl/gui/theme/renderer/comboboxrenderer.cpp
// Description : ComboBox Renderer
//
//************************************************************************************************

#include "ccl/gui/theme/renderer/comboboxrenderer.h"

#include "ccl/gui/controls/selectbox.h"

using namespace CCL;

//************************************************************************************************
// ComboBoxRenderer
/** A ComboBox is drawn like a SelectBox - see the SelectBoxRenderer for the atoms and the layout,
including the "button" image that indicates the popup.

Only the value text differs: it comes from the edit parameter instead of the select parameter,
and it is hidden while the ComboBox is edited, because the edit control draws it then. */
//************************************************************************************************

DEFINE_TYPELIB_ENTRY_WITH_PARENT (ComboBox, SelectBox, "ComboBoxStyle", VISUALSTYLES_TYPELIB)

//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_CLASS_ABSTRACT (ComboBoxRenderer, SelectBoxRenderer)

//////////////////////////////////////////////////////////////////////////////////////////////////

void ComboBoxRenderer::applyVisualStyle (RenderAtomTransaction& transaction)
{
	SuperClass::applyVisualStyle (transaction);

	// the text of a ComboBox comes from its edit parameter, so it is shown even when the select
	// parameter has no matching value - unlike the value text of a SelectBox
	showValueText = !hideText;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ComboBoxRenderer::updateAtoms ()
{
	SuperClass::updateAtoms ();

	// while the ComboBox is edited, the text is drawn by the edit control on top of it
	if(static_cast<ComboBox*> (owner)->isEditing ())
		valueAtom.isHidden (true);
}
