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
// Filename    : ccl/gui/theme/renderer/comboboxrenderer.cpp
// Description : Control Renderer
//
//************************************************************************************************

#include "ccl/gui/theme/renderer/comboboxrenderer.h"

#include "ccl/gui/controls/selectbox.h"
#include "ccl/public/gui/iparameter.h"

using namespace CCL;

//************************************************************************************************
// ComboBoxRenderer
/** If option "transparent" is not set, a ComboBox draws a background image. 
When no image is specified, it is filled in "backcolor, and a rectangular frame in color "forecolor" can be drawn with option "border".

The "button" image is drawn at the right. The remaining space shows the text field to edit the parameter value with an optional padding. */
//************************************************************************************************

BEGIN_VISUALSTYLE_CLASS (ComboBox, VisualStyle, "ComboBoxStyle")
	ADD_VISUALSTYLE_IMAGE  ("button") ///< image used to draw the button that indicates that something can popup
	ADD_VISUALSTYLE_METRIC ("padding.left")		///< left padding for the text
	ADD_VISUALSTYLE_METRIC ("padding.top")		///< top padding for the text
	ADD_VISUALSTYLE_METRIC ("padding.right")	///< right padding for the text
	ADD_VISUALSTYLE_METRIC ("padding.bottom")	///< bottom padding for the text
	ADD_VISUALSTYLE_METRIC ("padding")			///< padding for title, used if one of the paddings for left, top, right, bottom is not specified
END_VISUALSTYLE_CLASS (ComboBox)

//////////////////////////////////////////////////////////////////////////////////////////////////

ComboBoxRenderer::ComboBoxRenderer (VisualStyle* visualStyle)
: CompositedRenderer (visualStyle)
{
	background = visualStyle->getImage ("background");
	button = visualStyle->getImage ("button");
	visualStyle->getPadding (padding);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ComboBoxRenderer::draw (View* view, const UpdateRgn& updateRgn)
{
	ComboBox* comboBox = (ComboBox*)view;

	GraphicsPort port (comboBox);

	Rect r;
	comboBox->getClientRect (r);

	StyleRef style = comboBox->getStyle ();

	if(style.isOpaque ())
	{
		if(background)
		{
			port.drawImage (background, Rect (0, 0, background->getWidth (), background->getHeight ()), r);
		}
		else
		{
			port.fillRect (updateRgn.bounds, visualStyle->getBackBrush ());
			if(style.isBorder ())
				port.drawRect (r,visualStyle->getForePen ());
		}

		if(button)
		{
			Rect buttonRect (r);
			buttonRect.left = r.right - button->getWidth ();

			buttonRect.setHeight (button->getHeight ());
			buttonRect.centerV (r);
			
			port.drawImage (button, Rect (0, 0, button->getWidth (), button->getHeight ()), buttonRect);
			r.right = buttonRect.left;
		}
		else
		{
			Rect buttonRect;
			getPartRect (comboBox, SelectBox::kPartDropDownButton, buttonRect);

			enum { kIconWidth = 8 };
			Point points[3];
			Rect t (0, 0, kIconWidth, kIconWidth / 2);
			t.center (buttonRect);
			Coord cx = (t.left + t.right) / 2;
			points[0] (t.left, t.top), points[1] (cx, t.bottom), points[2](t.right, t.top);
			port.fillTriangle (points, visualStyle->getTextBrush ());
			r.right = buttonRect.left;
		}
	}

	if(comboBox->isFocused () && !style.isCustomStyle (Styles::kSelectBoxAppearanceHideFocus))
	{
		Rect focusRect;
		comboBox->getClientRect (focusRect);
		port.drawRect (focusRect, Pen (comboBox->getTheme ().getThemeColor (ThemeElements::kSelectionColor)));
	}

	String text = comboBox->getText ();
	if(text.length () > 0 && !comboBox->isEditing ())
	{
		if(style.isCustomStyle (Styles::kTextBoxBehaviorPasswordEdit))
		{
			uchar pwdChar[2] = {0x25CF, 0};
			text = String (String (pwdChar), text.length ());
		}

		Brush textBrush = visualStyle->getTextBrush ();
		// TODO:
		//if(!comboBox->isEnabled ())
		//	...

		r.left   += padding.left;
		r.right  -= padding.right;
		r.top    += padding.top;
		r.bottom -= padding.bottom;
		
		port.drawString (r, text, visualStyle->getTextFont (), textBrush, visualStyle->getTextAlignment ());	
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int ComboBoxRenderer::hitTest (View* view, const Point& loc, Point* clickOffset)
{
	Rect r;

	getPartRect (view, SelectBox::kPartContentArea, r);
	if(r.pointInside (loc))
	{
		if(clickOffset)
		{
			clickOffset->x = loc.x - r.left;
			clickOffset->y = loc.y - r.top;
		}
		return SelectBox::kPartContentArea;
	}

	getPartRect (view, SelectBox::kPartDropDownButton, r);
	if(r.pointInside (loc))
	{
		if(clickOffset)
		{
			clickOffset->x = loc.x - r.left;
			clickOffset->y = loc.y - r.top;
		}
		return SelectBox::kPartDropDownButton;
	}
	return SelectBox::kPartNone;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ComboBoxRenderer::getPartRect (const View* view, int partCode, CCL::Rect& r)
{
	view->getClientRect (r);

	if(partCode == SelectBox::kPartContentArea)
	{
		r.contract (1);
		r.right -= button ? button->getWidth () : 16;
		return true;
	}
	else if(partCode == SelectBox::kPartDisplayArea)
	{
		if(getPartRect (view, SelectBox::kPartContentArea, r))
		{
			r.left += padding.left;
			r.right -= padding.right;
			return true;
		}
		return false;
	}
	else if(partCode == SelectBox::kPartDropDownButton)
	{
		r.contract (1);
		r.left = r.right - (button ? button->getWidth () : 16);
		return true;
	}

	return false;
}
