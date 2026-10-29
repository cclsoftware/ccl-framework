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
// Filename    : ccl/gui/theme/renderer/comboboxrenderer.h
// Description : ComboBox Renderer
//
//************************************************************************************************

#ifndef _ccl_comboboxrenderer_h
#define _ccl_comboboxrenderer_h

#include "ccl/gui/theme/renderer/selectboxrenderer.h"

namespace CCL {

//************************************************************************************************
// ComboBoxRenderer
//************************************************************************************************

class ComboBoxRenderer: public SelectBoxRenderer
{
public:
	DECLARE_CLASS_ABSTRACT (ComboBoxRenderer, SelectBoxRenderer)

	using SelectBoxRenderer::SelectBoxRenderer;

	// SelectBoxRenderer
	void applyVisualStyle (RenderAtomTransaction& transaction) override;
	void updateAtoms () override;
};

DECLARE_TYPELIB_ENTRY (ComboBox)

} // namespace CCL

#endif // _ccl_comboboxrenderer_h
