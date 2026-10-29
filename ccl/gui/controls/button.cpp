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
// Filename    : ccl/gui/controls/button.cpp
// Description : Button Controls
//
//************************************************************************************************

#include "ccl/gui/controls/button.h"
#include "ccl/gui/controls/swipehandler.h"
#include "ccl/gui/controls/controlaccessibility.h"
#include "ccl/gui/controls/textmodel.h"

#include "ccl/gui/theme/renderer/buttonrenderer.h"
#include "ccl/gui/touch/touchhandler.h"
#include "ccl/gui/graphics/imaging/multiimage.h"
#include "ccl/gui/layout/layoutprimitives.h"
#include "ccl/gui/popup/popupselector.h"
#include "ccl/gui/commands.h"

#include "ccl/base/message.h"
#include "ccl/base/asyncoperation.h"

#include "ccl/public/math/mathprimitives.h"
#include "ccl/public/gui/iparameter.h"
#include "ccl/public/gui/iparamobserver.h"
#include "ccl/public/gui/icontextmenu.h"
#include "ccl/public/gui/framework/imenu.h"
#include "ccl/public/gui/framework/ipalette.h"
#include "ccl/public/gui/framework/controlproperties.h"
#include "ccl/public/systemservices.h"

namespace CCL {

//************************************************************************************************
// ButtonMouseHandler
//************************************************************************************************

class ButtonMouseHandler: public MouseHandler
{
public:
	ButtonMouseHandler (Button* button = nullptr)
	: MouseHandler (button),
	  beginEditTime (0)
	{}

	void onBegin () override
	{ 
		beginEditTime = System::GetSystemTicks ();
		Button* button = (Button*)view;
		button->getParameter ()->beginEdit ();
		button->isEditingParameter (true);
		button->setMouseState (View::kMouseDown); 
	} 
	
	void onRelease (bool canceled) override
	{ 
		handleRelease (canceled, true);
	}

	bool onMove (int moveFlags) override
	{
		view->setMouseState (view->isInsideClient (current.where) ? View::kMouseDown : View::kMouseOver);
		return true;
	}

protected:
	int64 beginEditTime;
	
	void handleRelease (bool canceled, bool notifyTarget)
	{
		Button* button = (Button*)view;
		button->setMouseState ((!current.wasTouchEvent () && view->isInsideClient (current.where)) ? View::kMouseOver : View::kMouseNone);

		static constexpr int64 kDetectPushDuration = 200;
		bool wasFastPush = (System::GetSystemTicks () - beginEditTime) < kDetectPushDuration;
		
		if(!canceled && notifyTarget && (wasFastPush || view->isInsideClient (current.where)))
			button->push ();
		
		if(button->isAttached ())
		{
			button->getParameter ()->endEdit ();
			button->isEditingParameter (false);
		}
	}
};

//************************************************************************************************
// ImmediateButtonMouseHandler
//************************************************************************************************

class ImmediateButtonMouseHandler: public SwipeMouseHandler
{
public:
	ImmediateButtonMouseHandler (Button* button = nullptr)
	: SwipeMouseHandler (button, button->getStyle ().isCustomStyle (Styles::kButtonBehaviorSwipe) ? kSwipeAny : kNoSwipe),
	  silent (button->getStyle ().isCustomStyle (Styles::kButtonBehaviorSilentTracking))
	{}

	void onBegin () override
	{
		Button* button = (Button*)view;
		button->getParameter ()->beginEdit ();
		if(silent)
		{
			button->preview ();
			button->updateClient ();
		}
		else
		{
			button->setMouseState (View::kMouseDown); 
			button->push ();
		}
	} 
	
	void onRelease (bool canceled) override
	{
		Button* button = (Button*)view;
		button->setMouseState (View::kMouseNone); 
		button->getParameter ()->endEdit ();
	}

	bool onMove (int moveFlags) override
	{
		if(silent)
		{
			if(!trySwipe ())
				view->setMouseState (view->getStyle ().isCustomStyle (Styles::kButtonBehaviorSwipe) ? View::kMouseNone : View::kMouseOver);
		}
		else
		{
			if(view->isInsideClient (current.where))
				view->setMouseState (View::kMouseDown);
			else if(!trySwipe ())
				view->setMouseState (view->getStyle ().isCustomStyle (Styles::kButtonBehaviorSwipe) ? View::kMouseNone : View::kMouseOver);
		}
		return true;
	}

	bool checkCondition (const SwipeMouseHandler::SwipeCondition& c) override
	{
		return SwipeMouseHandler::checkCondition (c) && c.value == swipeCondition.value;
	}

	void onSwipeEnter (Control* newControl) override
	{
		onRelease (false); // leave old button

		view->release ();
		view = newControl;
		view->retain ();

		onBegin (); // enter new button (calls push)
	}

private:
    bool silent;
};
	
//************************************************************************************************
// MomentaryButtonMouseHandler
//************************************************************************************************

class MomentaryButtonMouseHandler : public ImmediateButtonMouseHandler
{
public:
	MomentaryButtonMouseHandler (Button* button = nullptr)
	: ImmediateButtonMouseHandler (button),
	  beginEditTime (0)
	{}
	
	void onBegin () override
	{
		beginEditTime = System::GetSystemTicks ();
	
		ImmediateButtonMouseHandler::onBegin();
	}
	
	void onRelease (bool canceled) override
	{
		Button* button = (Button*)view;
		button->setMouseState (View::kMouseNone);
		
		const int64 kLatchThreshold = 500; // milliseconds
		if(System::GetSystemTicks () - beginEditTime > kLatchThreshold && button->getParameter ()->getMax () == button->getParameter ()->getValue ())
			button->getParameter ()->setValue (button->getParameter ()->getMin (), true);
		
		button->getParameter ()->endEdit ();
	}
	
private:
	int64 beginEditTime;
};

//************************************************************************************************
// IntermediateButtonMouseHandler
//************************************************************************************************

class IntermediateButtonMouseHandler : public MouseHandler
{
	public:
	IntermediateButtonMouseHandler (Button* button = nullptr)
	: MouseHandler (button)
	{}
	
	void onBegin () override
	{
		Button* button = (Button*)view;
		button->getParameter ()->beginEdit ();
		button->setMouseState (View::kMouseDown);
		button->getParameter ()->setValue (button->getParameter ()->getMax (), true);
		button->getParameter ()->endEdit ();
	}
	
	void onRelease (bool canceled) override
	{
		Button* button = (Button*)view;
		button->getParameter ()->beginEdit ();
		button->setMouseState (View::kMouseNone);
		button->getParameter ()->setValue (button->getParameter ()->getMin (), true);
		button->getParameter ()->endEdit ();
	}
};
	
	
//************************************************************************************************
// ButtonSlideRenderer
//************************************************************************************************

class ButtonSlideRenderer: public ButtonRenderer
{
public:
	ButtonSlideRenderer (IImage* slideImage, View* owner)
	: ButtonRenderer (nullptr, owner),
	  slideImage (slideImage)
	{}

	void draw (const UpdateRgn& updateRgn) override
	{
		GraphicsPort port (owner);
		Rect rect;
		owner->getClientRect (rect);
		if(slideImage)
		{
			Rect src (0, 0, slideImage->getWidth (), slideImage->getHeight ());
			port.drawImage (slideImage, src, rect);
		}
	}

protected:
	SharedPtr<IImage> slideImage;
};

//************************************************************************************************
// ButtonSlideMouseHandler
//************************************************************************************************

class ButtonSlideMouseHandler: public ButtonMouseHandler
{
public:
	typedef ButtonMouseHandler SuperClass;

	ButtonSlideMouseHandler (Button* button = nullptr)
	: ButtonMouseHandler (button), framesPerState (0), startState (0), didSlide (false)
	{}

	void onBegin () override
	{
		Button* button = (Button*)view;
		int numStates = button->getNumFrames () / ThemeElements::kNumElementStates;
		startState = button->getCurrentFrame () / ThemeElements::kNumElementStates;

		slideImage = view->getVisualStyle ().getImage ("slide");
		if(slideImage)
		{
			if(numStates < 2)
				slideImage = nullptr;
			else
			{
				// expecting 1 frame per state and an equal number of intermediate frames between them
				framesPerState = (slideImage->getFrameCount () - 1) / (numStates - 1);
				ASSERT (framesPerState * (numStates - 1) == slideImage->getFrameCount () - 1)

				slideImage->setCurrentFrame (startState * framesPerState);

				ButtonSlideRenderer* slideRenderer = NEW ButtonSlideRenderer (slideImage, button);
				button->setRenderer (slideRenderer);
			}
		}
		SuperClass::onBegin ();
	} 
	
	void onRelease (bool canceled) override
	{ 
		Button* button = (Button*)view;

		if(!canceled)
		{
			if(didSlide)
			{
				// toggle when moved at least half of the button width / height
				float phase = getPhase ();
				if(phase >= .5f)
					button->getParameter ()->increment ();
				else if(phase <= -.5f)
					button->getParameter ()->decrement ();
			}
			else
				button->push ();
		}			

		button->setRenderer (nullptr);
		button->setMouseState ((!current.wasTouchEvent () && view->isInsideClient (current.where)) ? View::kMouseOver : View::kMouseNone);
		button->getParameter ()->endEdit ();
	}

	bool onMove (int moveFlags) override
	{
		float phase = getPhase ();
		if(ccl_abs (phase) >= 0.1f)
			didSlide = true;

		if(slideImage)
		{
			int startFrame = startState * framesPerState;
			int frame = (int)ccl_round<0> (startFrame + phase * framesPerState);
			ccl_lower_limit (frame, ccl_max (startFrame - framesPerState, 0));
			ccl_upper_limit (frame, ccl_min (startFrame + framesPerState, slideImage->getFrameCount () - 1));
			
			CCL_PRINTF ("startFrame: %d, phase: %f, frame: %d\n", startFrame, phase, frame)

			if(frame != slideImage->getCurrentFrame ())
			{
				didSlide = true;
				slideImage->setCurrentFrame (frame);
				view->invalidate ();
			}
		}
		return SuperClass::onMove (moveFlags);
	}

	float getPhase ()
	{
		// target frame reached when moved full button width / height
		Point dist (current.where - first.where);
		Coord moved = view->getStyle ().isVertical () ? dist.y : dist.x;
		Coord length = view->getStyle ().isVertical () ? view->getHeight () : view->getWidth ();
		float phase = float(moved) / length;
		return ccl_bound (phase, -1.f, 1.f);
	}

private:
	SharedPtr<IImage> slideImage;
	int framesPerState;
	int startState;
	bool didSlide;
};

//************************************************************************************************
// ToolButtonMouseHandler
/** For a tool button with a mode parameter. */
//************************************************************************************************

class ToolButtonMouseHandler: public ButtonMouseHandler
{
public:
	ToolButtonMouseHandler (ToolButton* button = nullptr, bool immediate = false)
	: ButtonMouseHandler (button),
	  modePopupTime (-1),
	  immediate (immediate)
	{
		periodic (!immediate);
	}

	void onBegin () override
	{
		Button* button = (Button*)view;
		button->getParameter ()->beginEdit ();
		button->setMouseState (View::kMouseDown);
		if(immediate)
			button->push ();
	}
	
	bool onMove (int moveFlags) override
	{
		// beware of VariantView acrobatics
		if(!view->isAttached ())
			return ButtonMouseHandler::onMove (moveFlags);

		if(view->isInsideClient (current.where))
		{
			int64 now = System::GetSystemTicks ();
			if(modePopupTime == -1)
				modePopupTime = now + ToolButton::kModeMenuDelay;
			else if(now > modePopupTime || immediate)
			{
				// select tool button and show menu
				ToolButton* toolButton = (ToolButton*)view;
				if(!immediate)
					toolButton->push (); // already pushed
				(NEW Message ("showModeMenu"))->post (toolButton);
				return false; // cancel mousehandler
			}
		}
		else
			modePopupTime = -1;

		return ButtonMouseHandler::onMove (moveFlags);
	}

	void onRelease (bool canceled) override
	{
		// if immediate, the target was already notified onBegin
		handleRelease (canceled, !immediate);
	}

	int64 modePopupTime;
	bool immediate;
};

//************************************************************************************************
// ToolToggleMouseHandler
//************************************************************************************************

class ToolToggleMouseHandler: public PeriodicMouseHandler
{
public:
	
	ToolToggleMouseHandler (ToolButton* tb)
	: PeriodicMouseHandler (tb),
	  hasTriggered (false)
	{
		waitAfterFirstClick = ToolButton::kModeMenuDelay;
	}

	void onBegin () override
	{
		((ToolButton*)view)->setMouseState (View::kMouseDown);
	}
	
	bool onMove (int moveFlags) override
	{
		Point movement = current.where - first.where;
		Coord manhattanDistance = ccl_abs (movement.x) + ccl_abs (movement.y);
		static constexpr Coord kAcceptAsClickDelta = 10;
		if(manhattanDistance > kAcceptAsClickDelta)
		{
			(NEW Message ("showModeMenu"))->post ((ToolButton*)view);
			return false;
		}

		return PeriodicMouseHandler::onMove (moveFlags);
	}
	
	bool onPeriodic () override
	{
		if(hasTriggered)
		{
			(NEW Message ("showModeMenu"))->post ((ToolButton*)view);
			return false;
		}
		
		hasTriggered = true;
		return true;
	}
	
	void onRelease (bool canceled) override
	{
		if(!canceled)
		{
			((ToolButton*)view)->getParameter ()->beginEdit ();
			((ToolButton*)view)->push ();
			((ToolButton*)view)->getParameter ()->endEdit ();
		}
	}
	
private:
	bool hasTriggered;
};

//************************************************************************************************
// ToolButton::PopupTouchHandler
//************************************************************************************************

class ToolButton::PopupTouchHandler: public RemotePopupTouchHandler
{
public:
	PopupTouchHandler (ToolButton* toolButton = nullptr)
	: RemotePopupTouchHandler (toolButton),
	  toolButton (toolButton),
	  wasOn (toolButton->isOn ())
	{
		if(wasOn)
			openPopupImmediately (true);
	}

	PROPERTY_FLAG (flags, 1<<(RemotePopupTouchHandler::kLastFlag + 1), pushOnSingleTap)
	PROPERTY_FLAG (flags, 1<<(RemotePopupTouchHandler::kLastFlag + 2), pushImmediately)

	// RemotePopupTouchHandler
	void openPopup () override
	{
		toolButton->showModeMenu ();
	}

	PopupSelector* getPopupSelector () const override
	{
		return toolButton->popupSelector;
	}

	void onBegin (const TouchEvent& event) override
	{
		if(pushImmediately ())
			toolButton->push ();

		RemotePopupTouchHandler::onBegin (event);
	}

	bool onMove (const TouchEvent& event) override
	{
		if(!pushImmediately () && !pushOnSingleTap () && !toolButton->isOn ())
			toolButton->push ();

		return RemotePopupTouchHandler::onMove (event);
	}

	tbool CCL_API onGesture (const GestureEvent& event) override
	{
		if(event.getType () == GestureEvent::kSingleTap)
		{
			if(pushOnSingleTap ())
				toolButton->push ();
			else if(!popupOpened () && !toolButton->popupSelector && wasOn)
				toolButton->showModeMenu ();
			return true;
		}
		return SuperClass::onGesture (event);
	}

private:
	ToolButton* toolButton;
	bool wasOn;
	
	using SuperClass = RemotePopupTouchHandler;
};

//************************************************************************************************
// ButtonAccessibilityProvider
/** Accessibility provider shared by Button and Toggle. */
//************************************************************************************************

class ButtonAccessibilityProvider: public ValueControlAccessibilityProvider,
								   public IAccessibilityActionProvider,
								   public IAccessibilityToggleProvider
{
public:
	DECLARE_CLASS_ABSTRACT (ButtonAccessibilityProvider, ValueControlAccessibilityProvider)

	ButtonAccessibilityProvider (Button& owner);

	// ValueControlAccessibilityProvider
	AccessibilityElementRole CCL_API getElementRole () const override;

	// IAccessibilityActionProvider
	tresult CCL_API performAction () override;

	// IAccessibilityToggleProvider
	tbool CCL_API isToggleOn () const override;
	tresult CCL_API toggle () override;

	CLASS_INTERFACES (ValueControlAccessibilityProvider)

protected:
	Button& getButton () const;
	bool exposesToggleCapability () const;

	// ValueControlAccessibilityProvider
	void getDefaultElementName (String& name) const override;
};

} // namespace CCL

using namespace CCL;

//************************************************************************************************
// Button
//************************************************************************************************

BEGIN_STYLEDEF (Button::customStyles)
	{"immediate",		Styles::kButtonBehaviorImmediate},
	{"swipe",			Styles::kButtonBehaviorSwipe},
	{"slide",			Styles::kButtonBehaviorSlide},
	{"ignoreimagesize",	Styles::kButtonLayoutIgnoreImageSize},
	{"silenttracking",	Styles::kButtonBehaviorSilentTracking},
	{"momentary",		Styles::kButtonBehaviorMomentary},
	{"multiline",		Styles::kButtonAppearanceMultiLine},
	{"hidefocus",    	Styles::kButtonAppearanceHideFocus},
	{"intermediate",   	Styles::kButtonBehaviorIntermediate},
	{"leadingicon",  	Styles::kButtonAppearanceLeadingIcon},
	{"trailingicon",  	Styles::kButtonAppearanceTrailingIcon},
	{"passive",		 	Styles::kButtonBehaviorPassive},
	{"needsoptionkey",	Styles::kButtonBehaviorNeedsOptionKey},
	{"needscommandkey",	Styles::kButtonBehaviorNeedsCommandKey},
	{"needsshiftkey",	Styles::kButtonBehaviorNeedsShiftKey},
	{"scaletext",		Styles::kButtonAppearanceScaleText},
END_STYLEDEF

//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_CLASS (Button, Control)
DEFINE_CLASS_UID (Button, 0xf1f8ddfc, 0x6875, 0x437c, 0x8d, 0xda, 0x5b, 0x2b, 0x61, 0x2c, 0xf2, 0x76)

//////////////////////////////////////////////////////////////////////////////////////////////////

Button::Button (const Rect& size, IParameter* param, StyleRef style, StringRef title)
: Control (size, param, style, title),
  titleParam (nullptr),
  colorParam (nullptr),
  textModel (nullptr)
{
	wantsFocus (true);
	setWheelEnabled (false);

	AutoPtr<IMutableTextModel> model = NEW StringTextModel ();
	model->fromParamString (title);
	share_and_observe_unknown<ITextModel> (this, textModel, model);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Button::~Button ()
{
	setTitleParam (nullptr);
	setColorParam (nullptr);
	share_and_observe_unknown<ITextModel> (this, textModel, nullptr);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Button::removed (View* parent)
{
	if(isEditingParameter () && param)
	{
		// our mouse handler might still be "editing" the parameter, e.g. when button is removed during push () via a VariantView
		// in this case we have to end the editing state here, see "attached" check in ButtonMouseHandler
		param->endEdit ();
		isEditingParameter (false);
	}
	SuperClass::removed (parent);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Button::setTitleParam (IParameter* p)
{
	if(titleParam)
	{
		ISubject::removeObserver (titleParam, this);
		titleParam->release ();
	}

	titleParam = p;

	if(titleParam)
	{
		titleParam->retain ();
		ISubject::addObserver (titleParam, this);

		UnknownPtr<ITextModelProvider> provider (titleParam);
		if(provider)
		{
			if(ITextModel* model = provider->getTextModel ())
				share_and_observe_unknown<ITextModel> (this, textModel, model);
		}
		else
		{
			AutoPtr<IMutableTextModel> stringModel = NEW StringTextModel ();
			share_and_observe_unknown<ITextModel> (this, textModel, stringModel);
			titleParam->toString (title);
			stringModel->fromParamString (title);
		}

		notify (UnknownPtr<ISubject> (titleParam), Message (kChanged));
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const ITextLayout* Button::getTextLayout () const
{
	if(renderer)
		return static_cast<ButtonRenderer*> (renderer)->getTextLayout ();

	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IImage* Button::getIcon () const
{
	return icon;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Button::setIcon (IImage* _icon)
{
	icon = _icon;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IParameter* Button::getColorParam () const
{
	return colorParam;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Button::setColorParam (IParameter* p)
{
	if(colorParam != p)
		share_and_observe_unknown (this, colorParam, p);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Button::calcAutoSize (Rect& r)
{
	if(getStyle ().isTransparent () && isEmpty () == false)
	{
		View::calcAutoSize (r);
		return;
	}

	ButtonRenderer* renderer = static_cast<ButtonRenderer*> (getRenderer ());
	renderer->updateAtoms ();
	r = rectFToInt (renderer->getSize ());
	r.moveTo (Point (0, 0)); // ensure that setting right and bottom works as intended
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Button::calcSizeLimits ()
{
	Control::calcSizeLimits ();

	Rect buttonSize (getSize ());
	if(buttonSize.isEmpty ())
	{
		Button::calcAutoSize (buttonSize);
		if(buttonSize.getWidth () > sizeLimits.minWidth)
			sizeLimits.minWidth = buttonSize.getWidth (); // set min sizeLimits to larger auto width
	}
	else
		sizeLimits.minWidth = buttonSize.getWidth (); // set min sizeLimits to explicit width

	// update maxWidth
	ccl_lower_limit (sizeLimits.maxWidth, sizeLimits.minWidth);
	ccl_lower_limit (sizeLimits.maxWidth, getTheme ().getThemeMetric (ThemeElements::kButtonWidth));

	sizeLimits.maxHeight = kMaxCoord;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Button::onSize (const Point& delta)
{
	SuperClass::onSize (delta);
	invalidate ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API Button::push ()
{
	SharedPtr<Unknown> lifeGuard (this);
	if(param)
	{
		param->setValue (param->getMax (), true);
		param->setValue (param->getMin (), false); // <-- target is not notified!
	}
	if(style.isTrigger ())
		signal (Message (kOnPush));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Button::preview ()
{
	if(IParamPreviewHandler* previewHandler = getPreviewHandler ())
	{
		ParamPreviewEvent e;
		e.value = param->getMax ();
		previewHandler->paramPreview (param, e);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int Button::getNumFrames () const
{
	return ThemeElements::kNumElementStates;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int Button::getCurrentFrame () const
{
	int themeElementState = getThemeElementState ();
	int buttonValue = isOn (); // even when there is second stable state for a button, it is important to show the right frame temporarily
	return buttonValue * ThemeElements::kNumElementStates + themeElementState;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Button::isOn () const
{
	return param && param->getValue () == param->getMax ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

MouseHandler* Button::createMouseHandler (const MouseEvent& event)
{
	if(getStyle ().isCustomStyle (Styles::kButtonBehaviorPassive))
		return nullptr;

	if(style.isCustomStyle (Styles::kButtonBehaviorNeedsOptionKey|Styles::kButtonBehaviorNeedsCommandKey|Styles::kButtonBehaviorNeedsShiftKey))
	{
		if((style.isCustomStyle (Styles::kButtonBehaviorNeedsOptionKey) && !event.keys.isSet (KeyState::kOption))
		|| (style.isCustomStyle (Styles::kButtonBehaviorNeedsCommandKey) && !event.keys.isSet (KeyState::kCommand))
		|| (style.isCustomStyle (Styles::kButtonBehaviorNeedsShiftKey) && !event.keys.isSet (KeyState::kShift)))
			return nullptr;
	}

	if(getStyle ().isCustomStyle (Styles::kButtonBehaviorIntermediate))
		return NEW IntermediateButtonMouseHandler (this);
	
	if(getStyle ().isCustomStyle (Styles::kButtonBehaviorImmediate|Styles::kButtonBehaviorSwipe|Styles::kButtonBehaviorSilentTracking))
		return NEW ImmediateButtonMouseHandler (this);

	if(getStyle ().isCustomStyle (Styles::kButtonBehaviorMomentary))
	   return NEW MomentaryButtonMouseHandler (this);
	
	if(getStyle ().isCustomStyle (Styles::kButtonBehaviorSlide))
		return NEW ButtonSlideMouseHandler (this);

	return NEW ButtonMouseHandler (this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ITouchHandler* Button::createTouchHandler (const TouchEvent& event)
{
	if(getStyle ().isCustomStyle (Styles::kButtonBehaviorPassive))
		return nullptr;

	MouseEvent mouseEvent (TouchMouseHandler::makeMouseEvent (MouseEvent::kMouseDown, event, *this));
	AutoPtr<MouseHandler> mouseHandler (createMouseHandler (mouseEvent));
	if(mouseHandler && getStyle ().isCustomStyle (Styles::kButtonBehaviorSwipe|Styles::kButtonBehaviorSlide))
	{
		TouchMouseHandler* touchHandler = NEW TouchMouseHandler (mouseHandler, mouseHandler->getView ());
		touchHandler->addRequiredGesture (GestureEvent::kLongPress|GestureEvent::kPriorityHigh);
		if(getStyle ().isCustomStyle (Styles::kButtonBehaviorSlide))
		{
			touchHandler->addRequiredGesture (GestureEvent::kSwipe|GestureEvent::kHorizontal, GestureEvent::kPriorityHigh);
			touchHandler->addRequiredGesture (GestureEvent::kSwipe|GestureEvent::kVertical, GestureEvent::kPriorityHigh);
		}
		return touchHandler;
	}
	return SuperClass::createTouchHandler (event);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ThemeRenderer* Button::getRenderer ()
{	
	if(renderer == nullptr)
		renderer = getTheme ().createRenderer (ThemePainter::kButtonRenderer, visualStyle, this);
	return renderer;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Button::onVisualStyleChanged ()
{
	if(hasIconFromVisualStyle ())
		icon.release ();

	SuperClass::onVisualStyleChanged ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Button::onMouseEnter (const MouseEvent& event)
{
	setMouseState (kMouseOver);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Button::onMouseLeave (const MouseEvent& event)
{
	setMouseState (kMouseNone);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Button::onMouseDown (const MouseEvent& event)
{
	if(getStyle ().isCustomStyle (Styles::kButtonBehaviorPassive))
	{
		setMouseState (View::kMouseDown);
		return false;
	}
	
	return SuperClass::onMouseDown (event);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Button::onKeyDown (const KeyEvent& event)
{
	// in dialogs, push the button with Return, Enter or Space
	if(ccl_cast<Dialog> (getWindow ()) && !getStyle ().isCustomStyle (Styles::kButtonBehaviorPassive))
		if(event.state.getModifiers () == 0)
			if(event.vKey == VKey::kReturn
				|| event.vKey == VKey::kEnter
				|| event.vKey == VKey::kSpace)
			{
				push ();
				return true;
			}

	return SuperClass::onKeyDown (event);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API Button::getProperty (Variant& var, MemberID propertyId) const
{
	if(propertyId == kButtonIcon)
	{
		var.takeShared (getIcon ());
		return true;
	}
	return SuperClass::getProperty (var, propertyId);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API Button::setProperty (MemberID propertyId, const Variant& var)
{
	if(PhaseProperty<Button>::setPhaseProperty (propertyId, var))
		return true;

	if(propertyId == kButtonIcon)
	{
		setIcon (UnknownPtr<IImage> (var.asUnknown ()));
		return true;
	}
	return SuperClass::setProperty (propertyId, var);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_METHOD_NAMES (Button)
	DEFINE_METHOD_NAME ("push")
END_METHOD_NAMES (Button)

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API Button::invokeMethod (Variant& returnValue, MessageRef msg)
{
	if(msg == "push")
	{
		push ();
		return true;
	}
	else
		return SuperClass::invokeMethod (returnValue, msg);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API Button::notify (ISubject* subject, MessageRef msg)
{
	if(titleParam && msg == kChanged && isEqualUnknown (titleParam, subject))
	{
		titleParam->toString (title);
		UnknownPtr<IMutableTextModel> model (textModel);
		if(model)
			model->fromParamString (title);

		updateClient ();
	}
	else if(colorParam && isEqualUnknown (subject, colorParam))
	{
		invalidate ();
		if(renderer)
			renderer->invalidateStyleConditions ();
	}
	else
		SuperClass::notify (subject, msg);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

AccessibilityProvider* Button::getAccessibilityProvider ()
{
	if(!accessibilityProvider)
		accessibilityProvider = NEW ButtonAccessibilityProvider (*this);
	return accessibilityProvider;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Button::getStyleConditions (StyleConditionContext& conditions) const
{
	conditions.setCondition (ThemeNames::kOn, isOn ());

	StyleRef style = getStyle ();
	conditions.setCondition (ThemeNames::kLeft, style.isCommonStyle (Styles::kLeft));
	conditions.setCondition (ThemeNames::kMiddle, style.isCommonStyle (Styles::kMiddle));
	conditions.setCondition (ThemeNames::kRight, style.isCommonStyle (Styles::kRight));

	SuperClass::getStyleConditions (conditions);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Button::setTitle (StringRef title)
{
	SuperClass::setTitle (title);
	UnknownPtr<IMutableTextModel> model (textModel);
	if(model)
		model->fromParamString (title);
}

//************************************************************************************************
// Toggle
//************************************************************************************************

BEGIN_STYLEDEF (Toggle::customStyles)
	{"invert", Styles::kToggleBehaviorInvertParam},
END_STYLEDEF

//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_CLASS (Toggle, Button)
DEFINE_CLASS_UID (Toggle, 0x72c02c75, 0x1d38, 0x44d6, 0xaa, 0x58, 0xce, 0xe0, 0x5f, 0x92, 0xc5, 0x1f)

//////////////////////////////////////////////////////////////////////////////////////////////////

Toggle::Toggle (const Rect& size, IParameter* param, StyleRef style, StringRef title)
: Button (size, param, style, title),
  accessibilityExposeAsButton (false)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API Toggle::push ()
{
	SharedPtr<Unknown> lifeGuard (this);
	if(param->getValue () == param->getMin ())
		param->setValue (param->getMax (), true);
	else
		param->setValue (param->getMin (), true);

	if(style.isTrigger ())
		signal (Message (kOnPush));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Toggle::preview ()
{
	if(IParamPreviewHandler* previewHandler = getPreviewHandler ())
	{
		ParamPreviewEvent e;
		e.value = param->getValue () == param->getMin () ? param->getMax () : param->getMin ();
		previewHandler->paramPreview (param, e);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int Toggle::getNumFrames () const
{
	return 2 * ThemeElements::kNumElementStates;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int Toggle::getCurrentFrame () const
{
	int themeElementState = getThemeElementState ();
	int buttonValue = isOn ();
	return buttonValue * ThemeElements::kNumElementStates + themeElementState;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Toggle::isOn () const
{
	if(style.isCustomStyle (Styles::kToggleBehaviorInvertParam))
		return param && (param->getValue () != param->getMax ());	
	else 
		return param && (param->getValue () != param->getMin ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ThemeRenderer* Toggle::getRenderer ()
{	
	if(renderer == nullptr)
		renderer = getTheme ().createRenderer (ThemePainter::kButtonRenderer, visualStyle, this);
	return renderer;
}

//************************************************************************************************
// MultiToggle
//************************************************************************************************

BEGIN_STYLEDEF (MultiToggle::customStyles)
	{"decrement", Styles::kMultiToggleBehaviorDecrement},
END_STYLEDEF

//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_CLASS_HIDDEN (MultiToggle, Toggle)

//////////////////////////////////////////////////////////////////////////////////////////////////

MultiToggle::MultiToggle (const Rect& size, IParameter* param, StyleRef style, StringRef title)
: Toggle (size, param, style, title)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API MultiToggle::push ()
{
	if(style.isCustomStyle (Styles::kMultiToggleBehaviorDecrement))
	{
		if(param->getValue () == param->getMin ())
			param->setValue (param->getMax (), true);
		else
			param->decrement ();
	}
	else
	{
		if(param->getValue () == param->getMax ())
			param->setValue (param->getMin (), true);
		else
			param->increment ();
	}

	if(style.isTrigger ())
		signal (Message (kOnPush));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void MultiToggle::preview ()
{
	if(IParamPreviewHandler* previewHandler = getPreviewHandler ())
	{
		ParamPreviewEvent e;
		if(param->getValue () == param->getMax ())
			e.value = param->getMin ();
		else
			e.value = param->getValue ().asInt () + 1;
		previewHandler->paramPreview (param, e);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int MultiToggle::getNumFrames () const
{
	int states = param ? param->getMax ().asInt () - param->getMin ().asInt () + 1 : 1;
	return states * ThemeElements::kNumElementStates;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int MultiToggle::getCurrentFrame () const
{
	int themeElementState = getThemeElementState ();
	int buttonValue = param ? param->getValue ().asInt () - param->getMin ().asInt () : 0;
	return buttonValue * ThemeElements::kNumElementStates + themeElementState;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ThemeRenderer* MultiToggle::getRenderer ()
{	
	if(renderer == nullptr)
	{
		VisualStyle* vs = visualStyle;
		if(!vs)
			vs = getTheme ().getStandardStyle (ThemePainter::kButtonRenderer);

		renderer = NEW MultiToggleRenderer (vs, this);
	}
	return renderer;
}

//************************************************************************************************
// CheckBox
//************************************************************************************************

BEGIN_STYLEDEF (CheckBox::customStyles)
	{"tristate", Styles::kCheckBoxBehaviorTriState},
END_STYLEDEF

//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_CLASS (CheckBox, Toggle)
DEFINE_CLASS_UID (CheckBox, 0x6A867CCE, 0xCE80, 0x438F, 0xB6, 0x93, 0x34, 0x4F, 0xC7, 0x5F, 0x67, 0xBC)

//////////////////////////////////////////////////////////////////////////////////////////////////

CheckBox::CheckBox (const Rect& size, IParameter* param, StyleRef style, StringRef title)
: Toggle (size, param, style, title)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

IImage* CheckBox::getMixedIcon () const
{
	return mixedIcon;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CheckBox::setMixedIcon (IImage* _icon)
{
	mixedIcon = _icon;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool CheckBox::isOn () const
{
	if(style.isCustomStyle (Styles::kCheckBoxBehaviorTriState))
	{
		if(style.isCustomStyle (Styles::kToggleBehaviorInvertParam))
			return param && (param->getValue () == param->getMin ());
		else
			return param && (param->getValue () == param->getMax ());
	}
	return SuperClass::isOn ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool CheckBox::isMixed () const
{
	if(style.isCustomStyle (Styles::kCheckBoxBehaviorTriState))
		return param && param->getValue () != param->getMin () && param->getValue () != param->getMax ();
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CheckBox::calcSizeLimits ()
{
	SuperClass::calcSizeLimits ();
	VisualStyle* vs = getRenderer ()->getVisualStyle ();
	if(vs && vs->getMetric<bool> ("buttonstyle", false) == false)
	{
		Rect r;
		calcAutoSize (r);

		// increase minWidth and maxWidth if necessary
		ccl_lower_limit (sizeLimits.minWidth, r.right);
		if(sizeLimits.maxWidth != -1)
			ccl_lower_limit (sizeLimits.maxWidth, sizeLimits.minWidth);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CheckBox::calcAutoSize (Rect& r)
{
	if(renderer == nullptr)
	{
		if(auto* renderer = ccl_cast<ButtonRenderer> (getRenderer ()))
			renderer->updateAtoms ();
	}

	const ITextLayout* textLayout = nullptr;
	if(auto* buttonRenderer = ccl_cast<ButtonRenderer> (renderer))
		textLayout = buttonRenderer->getTextLayout ();

	SuperClass::calcAutoSize (r);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ThemeRenderer* CheckBox::getRenderer ()
{	
	if(renderer == nullptr)
		renderer = getTheme ().createRenderer (ThemePainter::kCheckBoxRenderer, visualStyle, this);
	return renderer;
}

//************************************************************************************************
// RadioButton
//************************************************************************************************

BEGIN_STYLEDEF (RadioButton::customStyles)
	{"toggle", Styles::kRadioButtonBehaviorToggle},
END_STYLEDEF

DEFINE_CLASS (RadioButton, CheckBox)
DEFINE_CLASS_UID (RadioButton, 0xbf2208e3, 0xa4, 0x4ae2, 0x97, 0x63, 0x84, 0x9c, 0x5e, 0x17, 0x49, 0xb5)

//////////////////////////////////////////////////////////////////////////////////////////////////

RadioButton::RadioButton (const Rect& size, IParameter* param, StyleRef style, StringRef title, float value)
: CheckBox (size, param, style, title),
  value (value)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

ThemeRenderer* RadioButton::getRenderer ()
{	
	if(renderer == nullptr)
		renderer = getTheme ().createRenderer (ThemePainter::kRadioButtonRenderer, visualStyle, this);
	return renderer;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int RadioButton::getCurrentFrame () const
{
	int themeElementState = getThemeElementState ();
	int buttonValue = isOn () ? 1 : 0;
	return buttonValue * ThemeElements::kNumElementStates + themeElementState;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API RadioButton::push ()
{
	if(isOn () && style.isCustomStyle (Styles::kRadioButtonBehaviorToggle))
		param->setValue (param->getMin (), true);
	else
		param->setValue (value, true);

	if(style.isTrigger ())
		signal (Message (kOnPush));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void RadioButton::preview ()
{
	if(IParamPreviewHandler* previewHandler = getPreviewHandler ())
	{
		ParamPreviewEvent e;
		e.value = value;
		previewHandler->paramPreview (param, e);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API RadioButton::setProperty (MemberID propertyId, const Variant& var)
{
	if(propertyId == kRadioButtonValue)
	{
		value = var.asFloat ();
		return true;
	}
	return SuperClass::setProperty (propertyId, var);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool RadioButton::isEnabled () const
{
	if(value > param->getMax ().asInt () || value < param->getMin ().asInt ())
		return false;

	return SuperClass::isEnabled ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool RadioButton::isOn () const
{
	if(!param)
		return false;
	
	float epsilon = (param->getMax ().asFloat () - param->getMin ().asFloat ()) / 100000.f;
	return ccl_equals (param->getValue ().asFloat (), value, epsilon);
}

//************************************************************************************************
// ToolButton
//************************************************************************************************

BEGIN_STYLEDEF (ToolButton::customStyles)
	{"nowheel",     Styles::kToolButtonBehaviorNoWheel},
	{"fixedicon",   Styles::kToolButtonAppearanceFixedIcon},
	{"modetoggle",   Styles::kToolButtonBehaviorModeToggle},
END_STYLEDEF

DEFINE_TYPELIB_ENTRY_WITH_PARENT (ToolButton, VisualStyle, "ToolButtonStyle", VISUALSTYLES_TYPELIB)
BEGIN_TYPELIB_MEMBERS (ToolButton)
	ADD_TYPELIB_MEMBER_METRIC ("popup.offset.x")		///< an additional horizontal offset to the popup position
	ADD_TYPELIB_MEMBER_METRIC ("popup.offset.y")		///< an additional vertical offset to the popup position
	ADD_TYPELIB_MEMBER_METRIC ("popupstyle")			///< when "popupstyle" is true - the toolButton visualStyle is also used for the popup
	ADD_TYPELIB_MEMBER_STRING ("decorform")				///< an optional "decorform" name identifying the form that decorates the popup
	ADD_TYPELIB_MEMBER_METRIC ("popup.palette.right")	///< popup the palette on the right side (mode parameter must be a IPaletteProvider)
	ADD_TYPELIB_MEMBER_METRIC ("popup.palette.bottom")	///< popup the palette on the bottom (mode parameter must be a IPaletteProvider)
	ADD_TYPELIB_MEMBER_METRIC (StyleID::kExtendedPopup)	///< displays the popup as an extended menu instead of the default tree menu
END_TYPELIB_MEMBERS (ToolButton)

//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_CLASS (ToolButton, RadioButton)
DEFINE_CLASS_UID (ToolButton, 0x47115A5E, 0x3550, 0x438A, 0x94, 0x0C, 0xC4, 0x93, 0x1E, 0x17, 0x7D, 0x7B)

//////////////////////////////////////////////////////////////////////////////////////////////////

ToolButton::ToolButton (const Rect& size, IParameter* param, StyleRef style, float value)
: RadioButton (size, param, style, nullptr, value),
  modeParam (nullptr),
  popupSelector (nullptr),
  popupOptions (PopupSizeInfo::kLeft|PopupSizeInfo::kBottom),
  activateAfterModeSelection (false)
{
	setWheelEnabled (style.isCustomStyle (Styles::kToolButtonBehaviorNoWheel) == false);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ToolButton::~ToolButton ()
{
	if(modeParam)
	{
		ISubject::removeObserver (modeParam, this);
		modeParam->release ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ToolButton::setModeParam (IParameter* p)
{
	if(modeParam)
	{
		ISubject::removeObserver (modeParam, this);
		modeParam->release ();
	}

	modeParam = p;

	if(modeParam)
	{
		modeParam->retain ();
		ISubject::addObserver (modeParam, this);
		
		// modeParam can provide a icon for the current mode
		UnknownPtr<IImageProvider> iconProvider (modeParam);
		if(iconProvider && !style.isCustomStyle (Styles::kToolButtonAppearanceFixedIcon))
			setIcon (iconProvider->getImage ());
		
		if(style.isCustomStyle (Styles::kToolButtonBehaviorModeToggle))
		{
			// toggle value derived from modeparam without off-state
			ASSERT ((modeParam->getMax ().asFloat () - 1.f) != param->getMax ().asFloat ())
			value = modeParam->getValue ().asFloat () + 1.f;
			style.setCustomStyle (Styles::kRadioButtonBehaviorToggle, true);
		}
		else if(style.isCustomStyle (Styles::kRadioButtonBehaviorToggle))
		{
			if(isEqualUnknown (modeParam, param))
				if(param->getMin () != param->getValue ())
					value = param->getValue ().asFloat ();
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ThemeRenderer* ToolButton::getRenderer ()
{	
	if(renderer == nullptr)
		renderer = getTheme ().createRenderer (ThemePainter::kButtonRenderer, visualStyle, this);
	return renderer;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ToolButton::draw (const UpdateRgn& updateRgn)
{
	SharedPtr<IImage> savedIcon (icon);
	if(savedIcon)
		icon = ImageResolutionSelector::selectImage (savedIcon, getSize ().getSize ());
	else
		icon = nullptr;  // prevent drawing the icon twice
	Toggle::draw (updateRgn);
	icon = savedIcon;

	if(modeParam)
	{
		if(IImage* modeImage = getVisualStyle ().getImage ("modeButton"))
		{
			MutableCString frame;
			
			if(isOn ())
				frame = ThemeNames::kPressed;
			else if(getMouseState () == ThemeElements::kMouseOver)
				frame = ThemeNames::kMouseOver;
			else
				frame = ThemeNames::kNormal;
		
			IImage::Selector (modeImage, frame);
			//IImage::Selector (modeImage, IImage::kNormal);

			Rect bounds (0, 0, getWidth (), getHeight ());
			Rect modeBoxRect (0, 0, 10, 10);
			modeBoxRect.setSize (Point (modeImage->getWidth (), modeImage->getHeight ()));
			modeBoxRect.moveTo (bounds.getRightBottom () - modeBoxRect.getSize ());

			GraphicsPort port (this);
			port.drawImage (modeImage, modeBoxRect.getLeftTop ());
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ToolButton::showModeMenu ()
{
	if(!isAttached ())
		return;	

	if((style.isCustomStyle (Styles::kRadioButtonBehaviorToggle) && (modeParam == param)) ||
		style.isCustomStyle (Styles::kToolButtonBehaviorModeToggle))
		activateAfterModeSelection = true;
	   
	Point offset;
	bool isPopupStyle = false;
	MutableCString defaultDecorName;
	if(visualStyle)
	{
		offset.x = visualStyle->getMetric<Coord> ("popup.offset.x", 0);
		offset.y = visualStyle->getMetric<Coord> ("popup.offset.y", 0);
		isPopupStyle = visualStyle->getMetric<bool> ("popupstyle", false);
		defaultDecorName = visualStyle->getString ("decorform");
	}
	
	PopupSizeInfo sizeInfo (this, popupOptions, offset);
	sizeInfo.canFlipParentEdge (true);
	sizeInfo.sizeLimits.minWidth = getWidth ();

	AutoPtr<PopupSelector> popupSelector = NEW PopupSelector;
	popupSelector->setTheme (getTheme ());

	ASSERT (this->popupSelector == nullptr)
	this->popupSelector = popupSelector;
	SharedPtr<ToolButton> This (this);
	
	if(visualStyle && isPopupStyle)
		popupSelector->setVisualStyle (visualStyle);

	if(!defaultDecorName.isEmpty ())
		popupSelector->setDecor (defaultDecorName, nullptr);

	if(UnknownPtr<IPaletteProvider> modePaletteParam = modeParam)
	{
		if(visualStyle)
		{
			sizeInfo.flags = (PopupSizeInfo::kLeft|PopupSizeInfo::kTop);
			if(!offset.isNull ())
				sizeInfo.flags |= PopupSizeInfo::kHasOffset;
			if(visualStyle->getMetric<bool> ("popup.palette.bottom", false))
			{
				sizeInfo.flags &= ~PopupSizeInfo::kTop;
				sizeInfo.flags |= PopupSizeInfo::kBottom;
			}
			if(visualStyle->getMetric<bool> ("popup.palette.right", false))
			{
				sizeInfo.flags &= ~PopupSizeInfo::kLeft;
				sizeInfo.flags |= PopupSizeInfo::kRight;
			}
			
			if(!isPopupStyle)
				popupSelector->wantsMouseUpOutside (true);
			popupSelector->closeAfterDrag (true);

			if(isPopupStyleButton ())
				popupSelector->setBehavior (popupSelector->getBehavior ()|IPopupSelector::kAcceptsAfterSwipe);
		}
	}

	CString menuType = MenuPresentation::kTree;
	if(visualStyle && visualStyle->getMetric<bool> (StyleID::kExtendedPopup, false))
		menuType = MenuPresentation::kExtended;

	Promise promise (popupSelector->popupAsync (modeParam, sizeInfo, menuType));
	promise.then ([=] (IAsyncOperation& operation)
	{
		This->popupSelector = nullptr;
	});
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ToolButton::setStyle (StyleRef _style)
{
	SuperClass::setStyle (_style);
	setWheelEnabled (style.isCustomStyle (Styles::kToolButtonBehaviorNoWheel) == false);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ToolButton::isPopupStyleButton () const
{
	return UnknownPtr<IPaletteProvider> (modeParam).isValid ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

MouseHandler* ToolButton::createMouseHandler (const MouseEvent& event)
{
	if(getStyle ().isCustomStyle (Styles::kButtonBehaviorPassive))
		return nullptr;

	if(modeParam)
	{
		if(style.isCustomStyle (Styles::kRadioButtonBehaviorToggle))
		{
			// mousehandler shows mode menu or selects / deselects tool param
			return NEW ToolToggleMouseHandler (this);
		}
		else if(isOn ())
		{
			// show mode menu immediately
			showModeMenu ();
			return NEW NullMouseHandler (this);
		}
		else
		{
			// mousehandler shows mode menu after mouse was down long enough
			return NEW ToolButtonMouseHandler (this, isPopupStyleButton ());
		}
	}
	
	return SuperClass::createMouseHandler (event);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ToolButton::onGesture (const GestureEvent& event)
{
	if(event.getType () == GestureEvent::kSingleTap)
	{
		if(modeParam && isOn ())
			showModeMenu ();
		return true;
	}
	return SuperClass::onGesture (event);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ITouchHandler* ToolButton::createTouchHandler (const TouchEvent& event)
{
	if(getStyle ().isCustomStyle (Styles::kButtonBehaviorPassive))
		return nullptr;

	if(modeParam)
	{
		auto handler = NEW PopupTouchHandler (this);

		if(style.isCustomStyle (Styles::kRadioButtonBehaviorToggle))
		{
			handler->openPopupImmediately (false);
			handler->openPopupOnLongPress (true);
			handler->pushImmediately (false);
			handler->pushOnSingleTap (true);
		}
		else
			handler->pushImmediately (isPopupStyleButton ());
		return handler;
	}

	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ToolButton::onMouseWheel (const MouseWheelEvent& event)
{
	if(View::onMouseWheel (event))
		return true;

	if(isWheelEnabled ())
		return Control::handleMouseWheel (event, modeParam);

	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ToolButton::onContextMenu (const ContextMenuEvent& event)
{
	if(modeParam)
	{
		showModeMenu ();
		return true;
	}

	MutableCString contextID ("ToolButton:");
	if(param)
	{
		contextID += param->getName ();
		contextID += ":";
	}
	contextID.appendFormat ("%d", (int)value);
	event.contextMenu.setContextID (contextID);

	return SuperClass::onContextMenu (event);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ToolButton::setProperty (MemberID propertyId, const Variant& var)
{
	if(propertyId == kToolButtonModeParam)
	{
		setModeParam (UnknownPtr<IParameter> (var.asUnknown ()));
		return true;
	}
	return SuperClass::setProperty (propertyId, var);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ToolButton::paramChanged ()
{
	if(style.isCustomStyle (Styles::kToolButtonBehaviorModeToggle))
	{
		if(param->getValue () != param->getMin ())
		{
			value = param->getValue ();
			modeParam->setValue (value - 1.f); // balance hidden off-state in modeparam
		}
	}
	SuperClass::paramChanged ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API ToolButton::notify (ISubject* subject, MessageRef msg)
{
	if(modeParam && isEqualUnknown (modeParam, subject))
	{
		if(msg == kChanged)
		{
			UnknownPtr<IImageProvider> iconProvider (modeParam);
			if(iconProvider && !style.isCustomStyle (Styles::kToolButtonAppearanceFixedIcon))
			{
				setIcon (iconProvider->getImage ());
				if(renderer)
					renderer->invalidateStyleConditions ();
				updateClient ();
			}
		}
		else if(msg == IParameter::kEndEdit && activateAfterModeSelection)
		{
			activateAfterModeSelection = false;

			if(style.isCustomStyle (Styles::kToolButtonBehaviorModeToggle))
			{
				value = modeParam->getValue ().asFloat () + 1.f; // balance hidden off-state in modeparam
				if(!isOn ())
					push ();
			}
			else if(modeParam->getMin () != modeParam->getValue ())
			{
				value = modeParam->getValue ().asFloat ();
				if(!isOn ())
					push ();
			}
			else
				param->setValue (param->getMin (), true);
		}
	}
	else if(msg == "showModeMenu")
	{
		if(!popupSelector || !popupSelector->isOpen ())
			showModeMenu ();
	}
	else if(msg == IParameter::kUpdateMenu)
	{
		if(popupSelector && popupSelector->isOpen ())
		{
			// forward message to popup selector client
			IPopupSelectorWindow* window = popupSelector->getCurrentWindow ();
			UnknownPtr<IObserver> client (window ?  window->getClient () : nullptr);
			if(client)
				client->notify (subject, msg);
		}
	}
	else
		SuperClass::notify (subject, msg);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int ToolButton::getNumFrames () const
{
	if(style.isCustomStyle (Styles::kRadioButtonBehaviorToggle))
		return 2 * ThemeElements::kNumElementStates;
	
	return ThemeElements::kNumElementStates;
}

//************************************************************************************************
// ButtonAccessibilityProvider
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (ButtonAccessibilityProvider, ValueControlAccessibilityProvider)

//////////////////////////////////////////////////////////////////////////////////////////////////

ButtonAccessibilityProvider::ButtonAccessibilityProvider (Button& owner)
: ValueControlAccessibilityProvider (owner)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

Button& ButtonAccessibilityProvider::getButton () const
{
	return static_cast<Button&> (view);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ButtonAccessibilityProvider::exposesToggleCapability () const
{
	Toggle* toggle = ccl_cast<Toggle> (&view);
	return toggle && !toggle->isAccessibilityExposeAsButton ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API ButtonAccessibilityProvider::queryInterface (UIDRef iid, void** ptr)
{
	if(exposesToggleCapability ())
	{
		QUERY_INTERFACE (IAccessibilityToggleProvider)
		QUERY_INTERFACE (IAccessibilityValueProvider)
	}
	else
		QUERY_INTERFACE (IAccessibilityActionProvider)

	return ControlAccessibilityProvider::queryInterface (iid, ptr);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

AccessibilityElementRole CCL_API ButtonAccessibilityProvider::getElementRole () const
{
	return AccessibilityElementRole::kButton;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ButtonAccessibilityProvider::getDefaultElementName (String& name) const
{
	if(view.getTitle ().isEmpty ())
	{
		UnknownPtr<ICommandParameter> commandParam (getControl ().getParameter ());
		if(commandParam)
		{
			if(ICommand* command = CommandTable::instance ().findCommand (commandParam->getCommandCategory (), commandParam->getCommandName ()))
			{
				CommandDescription commandDescription;
				command->getDescription (commandDescription);
				name = commandDescription.displayName;
				if(!name.isEmpty ())
					return;
			}
		}
	}
	return SuperClass::getDefaultElementName (name);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API ButtonAccessibilityProvider::performAction ()
{
	getButton ().push ();
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ButtonAccessibilityProvider::isToggleOn () const
{
	return getButton ().isOn ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API ButtonAccessibilityProvider::toggle ()
{
	getButton ().push ();
	return kResultOk;
}
