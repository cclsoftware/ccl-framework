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
// Filename    : ccl/gui/controls/vectorpad.cpp
// Description : Vector Pad (XY-Control)
//
//************************************************************************************************

#include "ccl/gui/controls/vectorpad.h"

#include "ccl/gui/theme/themerenderer.h"
#include "ccl/gui/views/mousehandler.h"
#include "ccl/gui/touch/touchhandler.h"

#include "ccl/public/gui/iparameter.h"

namespace CCL {

//************************************************************************************************
// VectorPadMouseHandler
//************************************************************************************************

class VectorPadMouseHandler: public MouseHandler
{
public:
	VectorPadMouseHandler (VectorPad* pad, PointRef clickOffset)
	: MouseHandler (pad),
	  clickOffset (clickOffset)
	{
		checkKeys (true);
	}

	~VectorPadMouseHandler ()
	{
		tooltipPopup.reserve (false);
	}
	
	void onBegin () override
	{ 	
		wasFine = (current.keys.getModifiers () & KeyState::kShift) != 0;
		fineWhere = current.where;
		VectorPad* pad = (VectorPad*)view;
		pad->setMouseState (View::kMouseDown);
		pad->getParameter ()->beginEdit ();
		pad->getYParameter ()->beginEdit ();
		onMove (0);
	} 
	
	void onRelease (bool canceled) override
	{
		VectorPad* pad = (VectorPad*)view;
		pad->getParameter ()->endEdit ();
		pad->getYParameter ()->endEdit ();
		pad->setMouseState (View::kMouseNone);
		tooltipPopup.reserve (false);
	}

	bool onMove (int moveFlags) override
	{
		VectorPad* pad = (VectorPad*)view;

		Point p (current.where);
		p.x -= clickOffset.x;
		p.y -= clickOffset.y;
			
		bool fine = (current.keys.getModifiers () & KeyState::kShift) != 0;
		if(wasFine != fine)
		{
			fineWhere = current.where;
			wasFine = fine;
		}

		float x = (float)p.x / pad->getWidth  ();
		float y = (float)p.y / pad->getHeight ();

		// fine mode
		if(fine)
		{
			float deltaX = float (current.where.x - fineWhere.x);
			float deltaY = float (current.where.y - fineWhere.y);
			x = ((float)fineWhere.x + 0.05f * deltaX - clickOffset.x) / pad->getWidth  ();
			y = ((float)fineWhere.y + 0.05f * deltaY - clickOffset.y) / pad->getHeight  ();
		}

		x = ccl_bound<float> (x);
		y = ccl_bound<float> (y);

		if(x != pad->getXValue ())
			pad->setXValue (x, true);
		if(y != pad->getYValue ())
			pad->setYValue (y, true);

		updateTooltip ();
		return true;
	}

	void updateTooltip () 
	{
		//TODO!
		//if(view->getStyle ().isCustomStyle (Styles::kEditTooltip))
		{
			String ytext;
			String text;
			text.append (" [");
			((VectorPad*)view)->getParameter ()->toString (ytext);
			text.append (ytext);
			text.append (" | ");
			((VectorPad*)view)->getYParameter ()->toString (ytext);
			text.append (ytext);
			text.append ("]");

			// conflicts with composed tooltips!!!
			//text = String () << view->getTooltip () << " " << text;

			tooltipPopup.setTooltip (text);
			tooltipPopup.reserve (true);
		}
	}

protected:
	Point clickOffset;
	Point fineWhere;
	bool wasFine;
};

} // namespace CCL

using namespace CCL;

//************************************************************************************************
// VectorPad
//************************************************************************************************

DEFINE_CLASS_HIDDEN (VectorPad, Control)

const Configuration::IntValue VectorPad::sliderMode ("GUI.Controls.Slider", "mode", Styles::kSliderModeTouch);

//////////////////////////////////////////////////////////////////////////////////////////////////

VectorPad::VectorPad (RectRef size, IParameter* param, IParameter* yParam, StyleRef style)
: Control (size, param, style),
  yParam (nullptr)
{
	setYParameter (yParam);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

VectorPad::~VectorPad ()
{
	setYParameter (nullptr);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IParameter* VectorPad::getYParameter () const
{
	return yParam;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void VectorPad::setYParameter (IParameter* p)
{
	if(yParam == p)
		return;

	if(yParam)
	{
		ISubject::removeObserver (yParam, this);
		yParam->release ();
	}

	yParam = p;

	if(yParam)
	{
		ISubject::addObserver (yParam, this);
		yParam->retain ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

float VectorPad::getYValue () const
{
	if(yParam == nullptr)
		return 0;

	float v = float (NormalizedValue (yParam).get ());

	v = 1.f - v;

	return v;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void VectorPad::setYValue (float v, bool update)
{
	if(yParam == nullptr)
		return;
	if(yParam->isEnabled () == false)
		return;		

	v = 1.f - v;

	NormalizedValue (yParam).set (v, update);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

float VectorPad::getXValue () const
{
	return (float)NormalizedValue (param).get ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void VectorPad::setXValue (float v, bool update)
{
	if(param->isEnabled () == false)
		return;
	
	NormalizedValue (param).set (v, update);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void VectorPad::onSize (const Point& delta)
{
	SuperClass::onSize (delta);
	invalidate ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API VectorPad::notify (ISubject* subject, MessageRef msg)
{
	if(msg == kChanged)
	{
		UnknownPtr<IParameter> p (subject);
		if(p && p == yParam)
		{
			paramChanged ();
			return;
		}
	}
	
	SuperClass::notify (subject, msg);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void VectorPad::paramChanged ()
{
	enable ((param && param->isEnabled ()) || (yParam && yParam->isEnabled ()) ? true : false);
	propertyChanged ("value");
	propertyChanged ("visualState");

	updateClient ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ThemeRenderer* VectorPad::getRenderer ()
{
	if(renderer == nullptr)
		renderer = getTheme ().createRenderer (ThemePainter::kVectorPadRenderer, visualStyle);
	return renderer;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool VectorPad::canHandleDoubleTap () const
{
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void VectorPad::performReset ()
{
	Control::performReset ();
	
	if(yParam)
	{
		yParam->beginEdit ();
		yParam->setValue (yParam->getDefaultValue (), true);
		yParam->endEdit ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

MouseHandler* VectorPad::createMouseHandler (const MouseEvent& event)
{
	if(isResetClick (event))
	{
		performReset ();
		return NEW NullMouseHandler (this); // swallow mouse click
	}

	Rect handleRect;
	getRenderer ()->getPartRect (this, kPartHandle, handleRect);
	
	const int mode = sliderMode.getValue ();

	Point clickOffset;
	if(handleRect.pointInside (event.where) || mode == Styles::kSliderModeRelative)
	{
		clickOffset.x = event.where.x - int (0.5 * (handleRect.left + handleRect.right));
		clickOffset.y = event.where.y - int (0.5 * (handleRect.top + handleRect.bottom));
	}

	return NEW VectorPadMouseHandler (this, clickOffset);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ITouchHandler* VectorPad::createTouchHandler (const TouchEvent& event)
{
	MouseEvent mouseEvent (TouchMouseHandler::makeMouseEvent (MouseEvent::kMouseDown, event, *this));
	AutoPtr<MouseHandler> mouseHandler (createMouseHandler (mouseEvent));
	if(mouseHandler)
	{
		TouchMouseHandler* touchHandler = NEW TouchMouseHandler (mouseHandler, mouseHandler->getView ());
		touchHandler->addRequiredGesture (GestureEvent::kLongPress|GestureEvent::kPriorityHigh);
		touchHandler->addRequiredGesture (GestureEvent::kSwipe|GestureEvent::kHorizontal, GestureEvent::kPriorityHigh);
		touchHandler->addRequiredGesture (GestureEvent::kSwipe|GestureEvent::kVertical, GestureEvent::kPriorityHigh);
		return touchHandler;
	}
	return SuperClass::createTouchHandler (event);
}
