//************************************************************************************************
//
// This file is part of Crystal Class Library (R)
// Copyright (c) 2026 CCL Software Licensing GmbH.
// All Rights Reserved.
//
// Licensed for use under either:
//  1. a Commercial License provided by CCL Software Licensing GmbH, or
//  2. GNU Affero General Public License v3.0 (AGPLv3).
// 
// You must choose and comply with one of the above licensing options.
// For more information, please visit ccl.dev.
//
// Filename    : ccl/gui/theme/renderer/bivrenderer.h
// Description : Background-Icon-Value (BIV) Renderer
//
//************************************************************************************************

#ifndef _ccl_bivrenderer_h
#define _ccl_bivrenderer_h

#include "ccl/gui/theme/atomrenderer.h"

namespace CCL {

//************************************************************************************************
// BIVRenderer
//************************************************************************************************

class BIVRenderer: public ControlRenderer
{
public:
	DECLARE_CLASS_ABSTRACT (BIVRenderer, ControlRenderer)

	BIVRenderer (VisualStyle* visualStyle, View* owner);
	~BIVRenderer ();

	PROPERTY_VARIABLE (Alignment, iconAlignment, IconAlignment)
	PROPERTY_VARIABLE (Alignment, valueAlignment, ValueAlignment)

	virtual RectF getSize () const { return backgroundAtom.getSize (); }
	const ITextLayout* getTextLayout () const;

	// ControlRenderer
	void updateAtoms () override;
	void applyVisualStyle (RenderAtomTransaction& transaction) override;
	void setContentScaleFactor (float factor) override;
	void CCL_API notify (ISubject* subject, MessageRef msg) override;

protected:
	enum AtomTag
	{
		kBackgroundAtom,
		kIconAtom,
		kValueAtom
	};

	enum ElementOrder
	{
		kLeadingIcon,
		kTrailingIcon
	};

	RenderAtom backgroundAtom;
	RenderAtom iconAtom;
	RenderAtom valueAtom;

	ITextModel* observedTextModel;

	ElementOrder order;
	Rect padding;
	Coord iconValueSpacing;
	float iconFillFactor;
	bool iconOverlay;
	float contentScaleFactor;

	bool updateTextModel (ITextModel* textModel);

	static void alignVertically (RectF& rect, RectFRef contentRect, AlignmentRef alignment);

	void snapToPixelGrid (RectF& rect, RectFRef area) const;

	#if LEGACY_STYLES_SUPPORTED
	Brush getBackgroundBrush () const;
	Brush getIconBrush () const;
	Brush getValueBrush () const;
	bool getStateColor (Color& color, const LegacyStyleID::StateColorIDs& ids) const;
	void applyLegacyVisualStyle ();
	#endif

	virtual Image* getIcon () const { return nullptr; }
};

DECLARE_TYPELIB_ENTRY (BIV)

} // namespace CCL

#endif // _ccl_bivrenderer_h
