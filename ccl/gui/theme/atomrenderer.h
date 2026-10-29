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
// Filename    : ccl/gui/theme/atomrenderer.h
// Description : Atom Renderer
//
//************************************************************************************************

#ifndef _ccl_atomrenderer_h
#define _ccl_atomrenderer_h

#include "ccl/gui/theme/renderatom.h"
#include "ccl/gui/theme/renderer/compositedrenderer.h"

namespace CCL {

//************************************************************************************************
// AtomRenderer
//************************************************************************************************

class AtomRenderer: public CompositedRenderer,
                    public IRenderAtomClient
{
public:
	DECLARE_CLASS_ABSTRACT (AtomRenderer, CompositedRenderer)

	AtomRenderer (VisualStyle* visualStyle, View* owner);

	virtual void updateAtoms ();
	virtual void applyVisualStyle (RenderAtomTransaction& transaction);

	// CompositedRenderer
	void draw (const UpdateRgn& updateRgn) override;
	int hitTest (PointRef where, Point* clickOffset = nullptr) override;
	bool getPartRect (Rect& rect, int partCode) override;
	void invalidateStyleConditions () override;

	// IRenderAtomClient
	void invalidate (const RenderAtom& atom) override;

	CLASS_INTERFACE (IRenderAtomClient, CompositedRenderer)

protected:
	FixedSizeVector<RenderAtom*, 5> atoms;
	bool styleConditionsDirty;
	StyleConditionContext conditionContext;

	void addAtom (RenderAtom& atom, int atIndex = -1);
	bool removeAtom (int atomTag);
	RenderAtom* findAtom (int atomTag);
	RenderAtom* getAtomAt (int index);

	static void getImageSize (RectF& imageSize, const Image* image, RectFRef contentSize, float fillFactor = 0.f);

	#if LEGACY_STYLES_SUPPORTED
	void applyLegacyContrastColors (RenderAtom& atom, const LegacyStyleID::ContrastColorIDs& ids); ///< a legacy contrast color defined by the visual style replaces the one of the atom
	#endif

private:
	class VisualStyleTransaction: public RenderAtomTransaction
	{
	public:
		VisualStyleTransaction (AtomRenderer* renderer)
		: renderer (renderer)
		{}

		~VisualStyleTransaction ()
		{
			renderer->finalizeVisualStyleChanges (*this);
		}

	private:
		AtomRenderer* renderer;
	};

	void finalizeVisualStyleChanges (const RenderAtomTransaction& transaction);
};

//************************************************************************************************
// ControlRenderer
//************************************************************************************************

class ControlRenderer: public AtomRenderer
{
public:
	DECLARE_CLASS_ABSTRACT (ControlRenderer, AtomRenderer)

	using AtomRenderer::AtomRenderer;

	// AtomRenderer
	void applyVisualStyle (RenderAtomTransaction& transaction) override;
};

//************************************************************************************************
// AtomName
//************************************************************************************************

namespace AtomName
{
	const CStringPtr kText = "text";
	const CStringPtr kPlaceholder = "placeholder";
	const CStringPtr kSelection = "selection";
	const CStringPtr kIcon = "icon";
	const CStringPtr kValue = "value";
	const CStringPtr kHeader = "header";
}

} // namespace CCL

#endif // _ccl_atomrenderer_h
