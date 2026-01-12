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
// Filename    : ccl/gui/controls/colorbox.cpp
// Description : Color Box
//
//************************************************************************************************

#include "ccl/gui/controls/colorbox.h"

#include "ccl/gui/theme/visualstyle.h"

#include "ccl/public/gui/iparameter.h"

using namespace CCL;

//************************************************************************************************
// ColorBox
//************************************************************************************************

BEGIN_STYLEDEF (ColorBox::customStyles)
	{"nowheel", Styles::kColorBoxBehaviorNoWheel},
END_STYLEDEF

//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_CLASS_HIDDEN (ColorBox, Control)

//////////////////////////////////////////////////////////////////////////////////////////////////

ColorBox::ColorBox (const Rect& size, IParameter* colorParam, StyleRef style)
: Control (size, colorParam, style),
  radius (0),
  selectParam (nullptr)
{
	setWheelEnabled (style.isCustomStyle (Styles::kColorBoxBehaviorNoWheel) == false);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ColorBox::~ColorBox ()
{
	setSelectParam (nullptr);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IParameter* ColorBox::getSelectParam () const
{
	return selectParam;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ColorBox::setSelectParam (IParameter* p)
{
	if(selectParam != p)
		share_and_observe_unknown(this, selectParam, p);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API ColorBox::notify (ISubject* s, MessageRef msg)
{
	UnknownPtr<IParameter> p (s);
	if(selectParam && p == selectParam && msg == kChanged)
	{
		updateClient ();
		return;
	}

	Control::notify (s, msg);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ColorBox::onMouseDown (const MouseEvent& event)
{
	if(View::onMouseDown (event))
		return true;

	if(selectParam && selectParam->isEnabled () && selectParam->getValue () != selectParam->getMax ())
	{
		selectParam->setValue (selectParam->getMax (), true);
		return true;
	}

	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ColorBox::onMouseWheel (const MouseWheelEvent& event)
{
	if(View::onMouseWheel (event))
		return true;

	if(isWheelEnabled ())
		return tryWheelParam (event, true); // invert direction for scrolling through palette param

	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ColorBox::draw (const UpdateRgn& updateRgn)
{
	if(style.isOpaque ())
	{
		Rect rect;
		getClientRect (rect);
		GraphicsPort port (this);
		const IVisualStyle& vs = getVisualStyle ();

		Color backColor;
		UnknownPtr<IColorParam> colorParam (param);
		if(colorParam)
		{
			colorParam->getColor (backColor);
			if(backColor.getAlphaF () == 0)
				backColor = vs.getColor ("defaultColor", backColor);
			else if(!param->isEnabled ())
				backColor.setAlphaF (.5f);
		}
		else if(param && param->getType () == IParameter::kInteger)
		{
			backColor = Color::fromInt (param->getValue ().asInt ());
			if(param->getMax ().asInt () <= 0xFFFFFF) // no alpha specified
				backColor.alpha = 0xFF;
		}
		else
			backColor = vs.getBackColor ();

		SolidBrush brush (backColor);

		if(IImage* background = vs.getBackgroundImage ())
		{
			if(!coloredBackground)
				coloredBackground = NEW ColoredBitmap (background, vs.getBackColor ());
			
			if(backColor.getAlphaF () == 0)
				backColor = vs.getBackColor ();
			
			float luminance = backColor.getLuminance ();
			float luminanceDiff = ccl_bound ((visualStyle->getMetric<float> ("color.luminance", luminance) - luminance), -0.5f, 0.5f);
			backColor.addBrightness (luminanceDiff);
			
			Color alphaBlendColor = visualStyle->getColor ("color.alphablend", Colors::kTransparentBlack);
			if(alphaBlendColor.getAlphaF () != 0)
				backColor.alphaBlend (alphaBlendColor, alphaBlendColor.getAlphaF ());
			
			coloredBackground->setColor (backColor);
			
			Rect srcRect (Point (coloredBackground->getWidth (), coloredBackground->getHeight ()));
			port.drawImage (coloredBackground, srcRect, rect);
		}
		else if(radius != 0)
		{
			bool isFrameDirty = true;
			Rect innerRect (rect);
			innerRect.contract (radius);
			if(innerRect.rectInside (updateRgn.bounds))
				isFrameDirty = false;

			if(isFrameDirty)
			{
				port.drawRoundRect (rect, radius, radius, Pen (backColor));
				port.fillRoundRect (rect, radius, radius, brush);
			}
			else
				port.fillRect (updateRgn.bounds, brush);
		}
		else
			port.fillRect (updateRgn.bounds, brush);

		if(style.isBorder ())
		{
			Pen pen (vs.getForePen ());
			if(pen.getWidth () > 2) // hmm???
				rect.contract (1);

			bool hilite = selectParam && selectParam->getValue () == selectParam->getMax ();
			if(hilite)
				pen.setColor (vs.getHiliteColor ());

			bool isFrameDirty = true;
			Rect innerRect (rect);
			innerRect.contract ((int)(pen.getWidth () + 1));
			if(innerRect.rectInside (updateRgn.bounds))
				isFrameDirty = false;

			if(isFrameDirty)
			{
				if(radius != 0)
					port.drawRoundRect (rect, radius, radius, pen);
				else
					port.drawRect (rect, pen);
			}
		}
	}

	View::draw (updateRgn);
}
