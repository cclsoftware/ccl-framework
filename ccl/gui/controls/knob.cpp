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
// Filename    : ccl/gui/controls/knob.cpp
// Description : Knob Control
//
//************************************************************************************************

#include "ccl/gui/controls/knob.h"

#include "ccl/gui/gui.h"
#include "ccl/gui/theme/visualstyle.h"
#include "ccl/gui/views/mousehandler.h"
#include "ccl/gui/touch/touchhandler.h"
#include "ccl/gui/system/mousecursor.h"

#include "ccl/public/gui/iparameter.h"

namespace CCL {

//************************************************************************************************
// KnobHandlerBase
//************************************************************************************************

class KnobHandlerBase
{
public:
	KnobHandlerBase (Knob* knob)
	: first (Point ()),
	  knob (knob),
	  startValue (knob->getValue ()),
	  wasFine (false)
	{}

	void setFirstPoint (PointRef where) { first = where; }

	float calcValue (PointRef where) const
	{
		float delta = float ((first.y - where.y) + (where.x - first.x));
		
		float fineScale = wasFine ? 0.05f : 1.0f;
		float v = startValue + (fineScale * delta) / 200.f;
		
		if(knob->getStyle ().isCustomStyle (Styles::kKnobBehaviorEndlessDial))
		{
			while(v > 1.f)
				v -= 1.f;
			while(v < 0.f)
				v += 1.f;
			
			return v;
		}
		else
			return ccl_bound<float> (v, 0.f, 1.f);
	}
	
protected:
	Point first;
	float startValue;
	Knob* knob;
	bool wasFine;
};

//************************************************************************************************
// KnobMouseHandler
//************************************************************************************************

class KnobMouseHandler: public KnobHandlerBase,
						public MouseHandler

{
public:
	KnobMouseHandler (Knob* knob)
	: KnobHandlerBase (knob),
	  MouseHandler (knob)
	{
		checkKeys (true);
	}
	
	~KnobMouseHandler ()
	{
		tooltipPopup.reserve (false);
	}
	
	// MouseHandler
	void onBegin () override
	{ 	
		setFirstPoint (current.where);
		Knob* knob = (Knob*)view;
		knob->getParameter ()->beginEdit ();
		wasFine = (current.keys.getModifiers () == KeyState::kShift);

		GUI.getMousePosition (oldMousePos);
		AutoPtr<MouseCursor> newCursor = MouseCursor::createCursor (ThemeElements::kSizeVerticalCursor);
		GUI.setCursor (newCursor);

		updateTooltip ();
	} 
	
	void onRelease (bool canceled) override
	{ 
		Knob* knob = (Knob*)view;
		knob->getParameter ()->endEdit ();
		
		// reset mouse cursor
		GUI.setCursor (nullptr);

		// TODO: mouse cursor is not restored correctly here!
		
		tooltipPopup.reserve (false);
	}

	bool onMove (int moveFlags) override
	{
		bool isShiftPressed = (current.keys.getModifiers () & KeyState::kShift) != 0;
		if(wasFine != isShiftPressed)
		{
			startValue = ((Knob*)view)->getValue ();
			setFirstPoint (current.where);
			wasFine = isShiftPressed;
		}

		((Knob*)view)->setValue (calcValue (current.where));

		updateTooltip ();
		return true;
	}

	void updateTooltip ()
	{
		if(view->getStyle ().isCustomStyle (Styles::kSliderBehaviorEditTooltip))
		{
			tooltipPopup.setTooltip (((Control*)view)->makeEditTooltip ());
			tooltipPopup.reserve (true);
		}
	}

protected:
	Point oldMousePos;
};

//************************************************************************************************
// KnobTouchHandler
//************************************************************************************************

class KnobTouchHandler: public KnobHandlerBase,
						public TouchHandler
{
public:
	KnobTouchHandler (Knob* knob)
	: KnobHandlerBase (knob),
	  TouchHandler (knob),
	  tooltipPopup (knob)
	{
		TouchMouseHandler::applyGesturePriorities (*this, knob);

		addRequiredGesture (GestureEvent::kLongPress, GestureEvent::kPriorityHigh);
	}
	
	~KnobTouchHandler ()
	{
		tooltipPopup.reserve (false);	
	}

	// TouchHandler
	tbool CCL_API onGesture (const GestureEvent& event) override
	{
		Point where (event.where);
		view->windowToClient (where);
		
		switch(event.getState ())
		{
			case GestureEvent::kBegin:
			{
				setFirstPoint (where);
				Knob* knob = (Knob*)view;
				knob->getParameter ()->beginEdit ();
				
				where.offset (40, -40);
				tooltipPosition = where;
			}
			break;
				
			case GestureEvent::kChanged:
			{
				((Knob*)view)->setValue (calcValue (where));
				
				if(view->getStyle ().isCustomStyle (Styles::kSliderBehaviorEditTooltip))
				{
					tooltipPopup.setTooltip (((Control*)view)->makeEditTooltip (), &tooltipPosition);
					tooltipPopup.reserve (true);
				}
			}
			break;
				
			case GestureEvent::kEnd:
			case GestureEvent::kFailed:
			{
				Knob* knob = (Knob*)view;
				knob->getParameter ()->endEdit ();
				tooltipPopup.reserve (false);
			}
			break;
		}
		return true;
	}
	
protected:
	UserTooltipPopup tooltipPopup;
	Point tooltipPosition;
};

} // namespace CCL

using namespace CCL;

//************************************************************************************************
// Knob
//************************************************************************************************

BEGIN_STYLEDEF (Knob::customStyles)
	{"filmstrip",	Styles::kKnobAppearanceFilmstrip},
	{"circle",      Styles::kKnobAppearanceCircle},
	{"indicator",   Styles::kKnobAppearanceIndicator},
	{"endless", 	Styles::kKnobBehaviorEndlessDial},
	{"centered",    Styles::kSliderAppearanceCentered},
END_STYLEDEF

//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_CLASS (Knob, Slider)
DEFINE_CLASS_UID (Knob, 0xcc6d91c2, 0x4274, 0x42ae, 0x9f, 0x45, 0x5, 0x3c, 0x3f, 0x77, 0x6c, 0xf3)

//////////////////////////////////////////////////////////////////////////////////////////////////

Knob::Knob (const Rect& size, IParameter* param, StyleRef _style)
: Slider (size, param, _style),
  offsetReference (nullptr)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

Knob::~Knob ()
{
	setOffsetReferenceParameter (nullptr);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Knob::calcAutoSize (Rect& r)
{
	r.setWidth (36);
	r.setHeight (36);

	IImage* baseImage = getVisualStyle ().getBackgroundImage ();
	
	if(baseImage == nullptr)
		baseImage = getVisualStyle ().getImage ("foreground");
	
	if(baseImage)
	{
		r.setWidth (baseImage->getWidth ());
		r.setHeight (baseImage->getHeight ());
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ThemeRenderer* Knob::getRenderer ()
{	
	if(renderer == nullptr)
		renderer = getTheme ().createRenderer (ThemePainter::kKnobRenderer, visualStyle);
	return renderer;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Knob::setOffsetReferenceParameter (IParameter* p)
{
	if(offsetReference != p)
		share_and_observe_unknown (this, offsetReference, p);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

float Knob::getOffsetReferenceValue () const
{
	return (float)NormalizedValue (offsetReference).get ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

MouseHandler* Knob::createMouseHandler (const MouseEvent& event)
{
	if(isResetClick (event))
	{
		performReset ();
		return NEW NullMouseHandler (this); // swallow mouse click
	}

	return NEW KnobMouseHandler (this);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

ITouchHandler* Knob::createTouchHandler (const TouchEvent& event)
{
	return NEW KnobTouchHandler (this);
}
