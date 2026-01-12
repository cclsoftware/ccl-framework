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
// Filename    : ccl/gui/controls/valuebar.h
// Description : Value Bar
//
//************************************************************************************************

#ifndef _ccl_valuebar_h
#define _ccl_valuebar_h

#include "ccl/gui/controls/control.h"

#include "ccl/gui/theme/themerenderer.h"

namespace CCL {

//************************************************************************************************
// ValueControl
//************************************************************************************************

class ValueControl: public Control
{
public:
	DECLARE_CLASS (ValueControl, Control)

	ValueControl (const Rect& size = Rect (), IParameter* param = nullptr, StyleRef style = Styles::kVertical);
	~ValueControl ();
	
	virtual float getValue () const;
	virtual void setValue (float v, bool update = true);

	void drawTicks (IGraphics& graphics, RectRef updateRect);

	IParameter* getColorParam () const;
	void setColorParam (IParameter* colorParam);

	// Control
	void onSize (const Point& delta) override;
	void draw (const UpdateRgn& updateRgn) override;
	AccessibilityProvider* getAccessibilityProvider () override;
	
protected:
	IParameter* colorParam;
};

//************************************************************************************************
// ValueBar
/** Displays a value as a horizontal or verical bar. 
A ValueBar is a passive control that displays a value as a partially filled rectangle over a background.

If the ValueBar's visual style has an image, the frame "normal" is used for the background, and frame "normalOn" for the bar. 
Otherwise "forecolor" and "backcolor" from the visual style are used to draw the bar and the background as a filled rectangle.
An optional "colorname" parameter can be used to dynamically colorize the hilitebar.

For biplolar parameters, the bar rectangle starts in the center of the view. */
//************************************************************************************************

class ValueBar: public ValueControl
{
public:
	DECLARE_CLASS (ValueBar, ValueControl)

	ValueBar (const Rect& size = Rect (), IParameter* param = nullptr, StyleRef style = Styles::kVertical);
	
	void getRects (float value, Rect& backgroundRect, Rect& hiliteRect) const;

	// Control
	ThemeRenderer* getRenderer () override;
	void updateClient () override;

private:
	struct ValueState
	{
		bool enabled;
		int visualState;
		Rect hilite;

		ValueState ()
		: enabled (false),		 
		  visualState (0)		 
		{}

		ValueState& assign (const ValueBar&); 
		bool operator== (const ValueState&) const;
	};

	ValueState valueState;    
};

//************************************************************************************************
// ProgressBar
/** A ProgressBar is used to display the progress of an operation.
A ProgressBar is usually used to display the progress of an operation, but can also be used with any parameter.

Similar to a ValueBar, a ProgressBar displays a value as a bar. 
Two separate images, "background" and "foreground" are used to draw the background and the bar.

Additionally, an optional "indicator" image can emphasize the boundary between the background and the bar.

If the foreground image has more than 2 frames, it is used as a filmstrip that cycles through all frames. */
//************************************************************************************************

class ProgressBar: public ValueBar,
				   public Control::PhaseProperty<ProgressBar>
{
public:
	DECLARE_CLASS (ProgressBar, ValueBar)

	ProgressBar (const Rect& size = Rect (), IParameter* param = nullptr, StyleRef style = Styles::kHorizontal);

	// ValueBar
	ThemeRenderer* getRenderer () override;

protected:
	// IObject
	tbool CCL_API setProperty (MemberID propertyId, const Variant& var) override;
};

//************************************************************************************************
// ActivityIndicatorView
/** An ActivityIndicator shows activity by displaying some animation.
Shows an animation while ActivityIndicator is visible. The duration of the animation is taken from 
the "duration" attribute of the foreground image resource. */
//************************************************************************************************

class ActivityIndicatorView: public ProgressBar
{
public:
	DECLARE_CLASS (ActivityIndicatorView, ProgressBar)

	ActivityIndicatorView (const Rect& size = Rect (), StyleRef style = 0);

	// ProgressBar
	void attached (View* parent) override;
	void removed (View* parent) override;

protected:
	void startAnimation ();
	void stopAnimation ();
};

} // namespace CCL

#endif // _ccl_valuebar_h
