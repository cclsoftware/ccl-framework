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
// Filename    : ccl/gui/theme/renderer/buttonrenderer.cpp
// Description : Button Renderer
//
//************************************************************************************************

#ifndef _ccl_buttonrenderer_h
#define _ccl_buttonrenderer_h

#include "ccl/gui/theme/renderer/bivrenderer.h"

#include "ccl/public/gui/framework/themeelements.h"

namespace CCL {

//************************************************************************************************
// ButtonRenderer
//************************************************************************************************

class ButtonRenderer: public BIVRenderer
{
public:
	DECLARE_CLASS_ABSTRACT (ButtonRenderer, BIVRenderer)

	ButtonRenderer (VisualStyle* visualStyle, View* owner);

	// BIVRenderer
	void applyVisualStyle (RenderAtomTransaction& transaction) override;
	void draw (const UpdateRgn& updateRgn) override;
	void updateAtoms () override;
	RectF getSize () const override;

protected:
	typedef Vector<int> IntVector;

	bool initialized;
	Image* backgroundImage;
	IntVector frameIndex;
	IntVector iconFrameIndex;

	virtual void initialize ();
	virtual void initFrames (IntVector& index, IImage& image);

	void updateIconFrames (Image& icon);
	bool getColorParamColor (Color& color) const;
	void applyColorParam ();

	#if LEGACY_STYLES_SUPPORTED
	Image* animationFilmstrip;
	int lastButtonValue;
	int phaseIconFrameIndex;
	float phaseForPendingAnimation;
	
	enum AnimationState
	{
		kAnimationStopped = 0,
		kAnimationPending = 1,
		kAnimationRunning = 2
	} animationState;

	void updateAnimationState ();
	bool isAnimationPending () const { return animationState == kAnimationPending; }
	bool isAnimationRunning () const { return animationState == kAnimationRunning; }
	#endif

	// BIVRenderer
	Image* getIcon () const override { return const_cast<RenderAtom*> (&iconAtom)->getImage (); }
};

DECLARE_TYPELIB_ENTRY (Button)

//************************************************************************************************
// MultiToggleRenderer
//************************************************************************************************

class MultiToggleRenderer: public ButtonRenderer
{
public:
	DECLARE_CLASS_ABSTRACT (MultiToggleRenderer, ButtonRenderer)

	MultiToggleRenderer (VisualStyle* visualStyle, View* owner);

protected:
	// ButtonRenderer
	void initFrames (IntVector& index, IImage& image) override;
};

//************************************************************************************************
// CheckBoxRenderer
//************************************************************************************************

class CheckBoxRenderer: public ButtonRenderer
{
public:
	DECLARE_CLASS_ABSTRACT (CheckBoxRenderer, ButtonRenderer)

	CheckBoxRenderer (VisualStyle* visualStyle, View* owner);
	
	// ButtonRenderer
	void applyVisualStyle (RenderAtomTransaction& transaction) override;

protected:
	IntVector mixedIconFrameIndex;
	Image* mixedIcon;

	ThemeElementID getThemeId () const;

	// ButtonRenderer
	void initialize () override;
};

DECLARE_TYPELIB_ENTRY (CheckBox)

//************************************************************************************************
// ButtonRenderer
//************************************************************************************************

class RadioButtonRenderer: public CheckBoxRenderer
{
public:
	DECLARE_CLASS_ABSTRACT (RadioButtonRenderer, CheckBoxRenderer)

	using CheckBoxRenderer::CheckBoxRenderer;
};

} // namespace CCL

#endif // _ccl_theme_h
