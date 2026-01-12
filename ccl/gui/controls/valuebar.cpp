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
// Filename    : ccl/gui/controls/valuebar.cpp
// Description : Value Bar
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "ccl/gui/controls/valuebar.h"
#include "ccl/gui/controls/controlaccessibility.h"

#include "ccl/gui/windows/window.h"
#include "ccl/gui/system/animation.h"

#include "ccl/gui/graphics/imaging/filmstrip.h"
#include "ccl/gui/theme/renderer/valuebarrenderer.h"

#include "ccl/public/gui/iparameter.h"
#include "ccl/public/gui/framework/controlscalepainter.h"

using namespace CCL;

//************************************************************************************************
// ValueControl
//************************************************************************************************

DEFINE_CLASS_HIDDEN (ValueControl, Control)

//////////////////////////////////////////////////////////////////////////////////////////////////

ValueControl::ValueControl (const Rect& size, IParameter* param, StyleRef style)
: Control (size, param, style),
  colorParam (nullptr)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

ValueControl::~ValueControl ()
{
	setColorParam (nullptr);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IParameter* ValueControl::getColorParam () const
{
	return colorParam;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ValueControl::setColorParam (IParameter* p)
{
	if(colorParam != p)
		share_and_observe_unknown (this, colorParam, p);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

float ValueControl::getValue () const
{
	return (float)NormalizedValue (param).get ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ValueControl::setValue (float v, bool update)
{
	CCL_PRINTF ("ValueControl set %f\n", v)

	NormalizedValue (param).set (v, update);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ValueControl::drawTicks (IGraphics& graphics, RectRef updateRect)
{
	UnknownPtr<ITickScale> scale (param->getCurve ());
	if(scale)
	{
		Rect clientRect;
		getClientRect (clientRect);
		ControlScalePainter painter (scale);
		painter.updateStyle (getVisualStyle ());
		painter.drawScaleGrid (graphics, clientRect, getStyle ().common);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ValueControl::onSize (const Point& delta)
{
	SuperClass::onSize (delta);
	invalidate ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ValueControl::draw (const UpdateRgn& updateRgn)
{
	Control::draw (updateRgn);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

AccessibilityProvider* ValueControl::getAccessibilityProvider ()
{
	if(!accessibilityProvider)
		accessibilityProvider = NEW ValueControlAccessibilityProvider (*this);
	return accessibilityProvider;
}

//************************************************************************************************
// ValueBar::ValueState
//************************************************************************************************

ValueBar::ValueState& ValueBar::ValueState::assign (const ValueBar& bar)
{
	enabled = bar.isEnabled ();
	visualState = bar.getParameter ()->getVisualState ();
	Rect back;
	bar.getRects (bar.getValue (), back, hilite);
	return *this;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ValueBar::ValueState::operator== (const ValueState& otherState) const
{
	return otherState.enabled == enabled
		&& otherState.visualState == visualState
		&& otherState.hilite == hilite; 	  // compare graphic results not value
}

//************************************************************************************************
// ValueBar
//************************************************************************************************

DEFINE_CLASS_HIDDEN (ValueBar, ValueControl)

//////////////////////////////////////////////////////////////////////////////////////////////////

ValueBar::ValueBar (const Rect& size, IParameter* param, StyleRef style)
: ValueControl (size, param, style)
{
	setWheelEnabled (false);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ValueBar::getRects (float value, Rect& backgroundRect, Rect& hiliteRect) const
{
	Rect rect, rect2;
	rect.bottom = rect2.bottom = getHeight ();
	rect.right = rect2.right = getWidth ();

	if(getParameter ()->isBipolar ())
	{
		if(style.isHorizontal ())
		{
			float range = (float)getWidth ();
			rect.left = rect.right = getWidth () / 2;
			if(value < 0.5f)
				rect.left = rect.right - (Coord)((0.5f - value) * range);
			else
				rect.right = rect.left + (Coord)((value - 0.5f) * range);
		}
		else
		{
			float range = (float)getHeight ();
			rect.top = rect.bottom = getHeight () / 2;
			if(value < 0.5f)
				rect.bottom = rect.top - (Coord)((value - 0.5f) * range);
			else
				rect.top = rect.bottom + (Coord)((0.5f - value) * range);
		}
	}
	else
	{
		if(style.isVertical ())
		{
			rect.top = (int)((1.f - value) * (float)getHeight ());
			rect2.bottom = rect.top;
		}
		else
		{
			rect.right = (int)(value * (float)rect2.right);
			rect2.left = rect.right;
		}
	}

	if(param->isReverse ())
	{
		backgroundRect = rect;
		hiliteRect = rect2;
	}
	else
	{
		backgroundRect = rect2;
		hiliteRect = rect;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ValueBar::updateClient ()
{
	if(!hasBeenDrawn ())
		return;

	ValueState newState;
	newState.assign (*this);

	if(newState == valueState) // redraw bar only if graphical hilite rect has changed
		return;

	valueState = newState;
	SuperClass::updateClient ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ThemeRenderer* ValueBar::getRenderer ()
{
	if(renderer == nullptr)
		renderer = getTheme ().createRenderer (ThemePainter::kValueBarRenderer, visualStyle);
	return renderer;
}

//************************************************************************************************
// ProgressBar
//************************************************************************************************

DEFINE_CLASS (ProgressBar, ValueBar)
DEFINE_CLASS_UID (ProgressBar, 0x98765af4, 0x87ad, 0x4d93, 0xbf, 0xdb, 0xb7, 0x24, 0xde, 0xcd, 0xc5, 0x69)

//////////////////////////////////////////////////////////////////////////////////////////////////

ProgressBar::ProgressBar (const Rect& size, IParameter* param, StyleRef style)
: ValueBar (size, param, style)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

ThemeRenderer* ProgressBar::getRenderer ()
{
	if(renderer == nullptr)
		renderer = getTheme ().createRenderer (ThemePainter::kProgressBarRenderer, visualStyle);
	return renderer;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ProgressBar::setProperty (MemberID propertyId, const Variant& var)
{
	if(PhaseProperty<ProgressBar>::setPhaseProperty (propertyId, var))
		return true;
	return SuperClass::setProperty (propertyId, var);
}

//************************************************************************************************
// ActivityIndicatorView
//************************************************************************************************

DEFINE_CLASS_HIDDEN (ActivityIndicatorView, ProgressBar)

//////////////////////////////////////////////////////////////////////////////////////////////////

ActivityIndicatorView::ActivityIndicatorView (const Rect& size, StyleRef style)
: ProgressBar (size, nullptr, StyleFlags (style.common|Styles::kHorizontal, style.custom))
{
	ASSERT (param != nullptr)
	param->setValue (1);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ActivityIndicatorView::attached (View* parent)
{
	SuperClass::attached (parent);

	startAnimation ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ActivityIndicatorView::removed (View* parent)
{
	stopAnimation ();

	SuperClass::removed (parent);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ActivityIndicatorView::startAnimation ()
{
	double duration = 1.;
	if(Image* foregroundImage = unknown_cast<Image> (getVisualStyle ().getImage ("foreground")))
	{
		Rect unused;
		Filmstrip* filmstrip = ccl_cast<Filmstrip> (foregroundImage);
		if(filmstrip == nullptr)
			filmstrip = ccl_cast<Filmstrip> (foregroundImage->getOriginalImage (unused));

		if(filmstrip && filmstrip->getDuration () != 0.)
			duration = filmstrip->getDuration ();
	}

	BasicAnimation a;
	a.setStartValue (0.);
	a.setEndValue (1.);
	a.setDuration (duration);
	a.setRepeatCount (Animation::kRepeatForever);
	AnimationManager::instance ().addAnimation (this, "phase", a.asInterface ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ActivityIndicatorView::stopAnimation ()
{
	AnimationManager::instance ().removeAnimation (this, "phase");
}
