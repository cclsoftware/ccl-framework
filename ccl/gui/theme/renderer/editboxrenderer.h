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
// Filename    : ccl/gui/theme/renderer/editboxrenderer.h
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
	DECLARE_CLASS_ABSTRACT (EditBoxRenderer, TextBoxRenderer)

	EditBoxRenderer (VisualStyle* visualStyle, View* owner);
	~EditBoxRenderer ();

	// TextBoxRenderer
	void draw (const UpdateRgn& updateRgn) override;

protected:
	Rect nativePadding;
	RenderAtom selectionAtom;
	RenderAtom placeholderAtom;
	IMutableTextModel* placeholderTextModel;
	
	// TextBoxRenderer
	void applyVisualStyle (RenderAtomTransaction& transaction) override;
	bool getPartRect (Rect& rect, int partCode) override;
	void updateAtoms () override;
	bool isOpaque () const override;
};

DECLARE_TYPELIB_ENTRY (EditBox)

} // namespace CCL

#endif // _ccl_editboxrenderer_h
