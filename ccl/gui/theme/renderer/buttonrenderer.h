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
// Description : Control Renderer
//
//************************************************************************************************

#ifndef _ccl_buttonrenderer_h
#define _ccl_buttonrenderer_h

#include "ccl/gui/theme/theme.h"
#include "ccl/gui/theme/renderer/compositedrenderer.h"

#include "ccl/public/gui/framework/styleflags.h"

namespace CCL {

class Button;
class CheckBox;
interface IColorParam;

//************************************************************************************************
// ButtonRenderer
//************************************************************************************************

class ButtonRenderer: public CompositedRenderer
{
public:
	ButtonRenderer (VisualStyle* visualStyle);
	
	// CompositedRenderer
	void draw (View* view, const UpdateRgn& updateRgn) override;
	int hitTest (View* view, const Point& loc, Point* clickOffset) override;
	bool getPartRect (const View* view, int partCode, CCL::Rect& rect) override;
	
protected:
	typedef Vector<int> IntVector;
	
	TextScaler textScaler;
	SharedPtr<IImage> image;
	int lastButtonValue;
	IntVector frameIndex;
	IntVector iconFrameIndex;
	int phaseFrameIndex;
	int phaseIconFrameIndex;
	Color textColor;
	Color textColorOn;
	Color textColorPressed;
	Color textColorPressedOn;
	Color textColorMouseOver;
	Color textColorMouseOverOn;
	Color textColorDisabled;
	Color textColorDisabledOn;
	Color textColorPhaseOn;
	Color iconColor;
	Color iconColorOn;
	Color iconColorDisabled;
	Color iconMouseoverColor;
	Color iconMouseoverColorOn;
	Color iconPressedColor;
	Color iconPressedColorOn;
	
	Color textContrastBrightColor;
	Color textContrastBrightColorOn;
	Color textContrastDarkColor;
	Color textContrastDarkColorOn;
	Color textContrastTransparentColor;
	Color textContrastTransparentColorOn;
	Color textColorAlphaBlend;
	Color iconContrastBrightColor;
	Color iconContrastBrightColorOn;
	Color iconContrastDarkColor;
	Color iconContrastDarkColorOn;
	Color iconContrastTransparentColor;
	Color iconContrastTransparentColorOn;
	Color iconColorAlphaBlend;
	mutable Color colorParamColor;
	
	Rect padding;
	Coord backcolorRadius;
	Coord iconSpacing;
	float iconFillSize;
	bool leadingIcon;
	bool trailingIcon;
	bool useModifiedIcon;
	bool drawAsTemplate;
	bool textShiftDownMode;
	float brightColorThreshold;
	bool initialized;
		
	SharedPtr<IImage> animationFilmstrip;
	float phaseForPendingAnimation;

	enum AnimationState
	{
		kAnimationStopped = 0,
		kAnimationPending = 1,
		kAnimationRunning = 2
	} animationState;

	void updateAnimationState (const Button* button);
	bool isAnimationPending () const { return animationState == kAnimationPending; }
	bool isAnimationRunning () const { return animationState == kAnimationRunning; }
	
	void setImage (IImage* image);
	virtual void initialize (StyleRef style, View* view);

	virtual void initFrames (IntVector& index, IImage& image);
	ColorRef getIconColor (IColorParam* colorParam, bool isOn, bool pressed, bool mouseover) const;
	ColorRef getTextColor (IColorParam* colorParam, bool isOn, bool pressed, bool mouseover) const;
};

DECLARE_VISUALSTYLE_CLASS (Button)

//************************************************************************************************
// MultiToggleRenderer
//************************************************************************************************

class MultiToggleRenderer: public ButtonRenderer
{
public:
	MultiToggleRenderer (VisualStyle* visualStyle);

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
	CheckBoxRenderer (VisualStyle* visualStyle);
	
	// ButtonRenderer
	void draw (View* view, const UpdateRgn& updateRgn) override;
	void initialize (StyleRef style, View* view) override;
	
protected:
	IntVector mixedIconFrameIndex;

	bool useButtonStyle;
	bool checkBoxRightSide;

	ThemeElementID getThemeId (CheckBox* checkBox) const;
};

DECLARE_VISUALSTYLE_CLASS (CheckBox)

//************************************************************************************************
// ButtonRenderer
//************************************************************************************************

class RadioButtonRenderer: public CheckBoxRenderer
{
public:
	RadioButtonRenderer (VisualStyle* visualStyle);
};

} // namespace CCL

#endif // _ccl_theme_h
