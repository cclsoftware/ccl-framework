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
// Filename    : ccl/gui/theme/renderer/textboxrenderer.h
// Description : Text Box Renderer
//
//************************************************************************************************

#ifndef _ccl_textboxrenderer_h
#define _ccl_textboxrenderer_h

#include "ccl/gui/theme/atomrenderer.h"

namespace CCL {

//************************************************************************************************
// TextBoxRenderer
//************************************************************************************************

class TextBoxRenderer: public ControlRenderer
{
public:
	DECLARE_CLASS_ABSTRACT (TextBoxRenderer, ControlRenderer)

	TextBoxRenderer (VisualStyle* visualStyle, View* owner);
	~TextBoxRenderer ();

	bool onTextInteraction (const IMutableTextModel::InteractionInfo& info);
	const ITextLayout* getTextLayout () const;
	void setSize (RectFRef size);

	// AtomRenderer
	void updateAtoms () override;
	void applyVisualStyle (RenderAtomTransaction& transaction) override;
	bool getPartRect (Rect& rect, int partCode) override;
	void CCL_API notify (ISubject* subject, MessageRef msg) override;

protected:
	RenderAtom textAtom;
	RenderAtom backgroundAtom;
	Rect padding;
	mutable Color colorParamColor;
	ITextModel* observedTextModel = nullptr;

	virtual bool isOpaque () const;
};

DECLARE_TYPELIB_ENTRY (TextBox)

} // namespace CCL

#endif // _ccl_textboxrenderer_h
