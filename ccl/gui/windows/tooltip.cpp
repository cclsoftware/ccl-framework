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
// Filename    : ccl/gui/windows/tooltip.cpp
// Description : Tooltips
//
//************************************************************************************************

#define DEBUG_LOG 0
#define UPDATE_ON_IDLE 1 // optimization for OSX

#include "ccl/gui/windows/tooltip.h"
#include "ccl/gui/windows/desktop.h"
#include "ccl/gui/windows/popupwindow.h"
#include "ccl/gui/layout/anchorlayout.h"
#include "ccl/gui/theme/visualstyle.h"
#include "ccl/gui/controls/label.h"
#include "ccl/gui/controls/control.h"
#include "ccl/gui/views/imageview.h"
#include "ccl/gui/commands.h"
#include "ccl/gui/gui.h"
#include "ccl/gui/keyevent.h"

#include "ccl/app/params.h"
#include "ccl/base/trigger.h"

#include "ccl/public/gui/iparameter.h"
#include "ccl/public/systemservices.h"

using namespace CCL;

//************************************************************************************************
// ComposedTooltip
//************************************************************************************************

ComposedTooltip::ComposedTooltip (View* view)
: String (view->getTooltip ()),
  view (view)
{
	resolve (*this);

	trimWhitespace ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ComposedTooltip::resolve (String& text)
{
	if(!text.isEmpty ())
	{
		int idx = text.index ("@");
		if(idx >= 0)
		{
			int idx1 = text.index ("[");
			int idx2 = text.index ("]");
			if(idx1 > -1 && idx2 > idx1)
			{
				StringRef identifier = text.subString (idx + 1, idx1 - idx - 1);
				StringRef argument = text.subString (idx1 + 1, idx2 - idx1 - 1);

				String resolved = resolveVariable (identifier, argument);

				String remainder = text.subString (idx2 + 1);
				ASSERT (remainder.length () < text.length ())
				resolve (remainder); // recursion

				text.truncate (idx);
				text.append (resolved);
				text.append (remainder);
			}	
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

String ComposedTooltip::resolveVariable (StringRef identifier, StringRef argument)
{
	if(identifier.startsWith ("cmd"))
	{
		// command key
		MutableCString category, name;
		bool withTitle = identifier == "cmd.title";

		if(argument.isEmpty ())
		{
			// from command parameter
			if(ICommandParameter* cmdParam = getCommandParameter ())
			{
				category = cmdParam->getCommandCategory ();
				name = cmdParam->getCommandName ();
			}
		}
		else
		{
			// explicit command given as "category|name"
			int idx = argument.index ("|");
			if(idx > 1)
			{
				category = argument.subString (0, idx);
				name = argument.subString (idx + 1);
			}
		}

		String result;
		if(withTitle)
			result = resolveCommandTitle (category, name);

		String keyString = resolveCommandKey (category, name);
		if(!keyString.isEmpty ())
		{
			if(!result.isEmpty ())
				result << " ";
			result << keyString;
		}

		return result;
	}
	else if(identifier == "key")
	{
		// from key identifier to translated, platform specific name
		VirtualKey key = VKey::getKeyByName (MutableCString (argument));
		return VKey::getLocalizedKeyName (key);
	}
	else if(identifier == "value")
	{
		// parameter as string
		if(IParameter* param = getParameter ())
		{
			String value;
			param->toString (value);
			return value;
		}
	}
	else if(identifier == "property")
	{
		Variant var;
		if(Property (view, MutableCString (argument)).get (var))
		{
			String value;
			var.toString (value);
			return value;
		}
	}
	return String::kEmpty;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

String ComposedTooltip::resolveCommandKey (StringID category, StringID name)
{
	String result;
	IterForEach (CommandTable::instance ().lookupBindings (Command (category, name)), Boxed::KeyEvent, key)
		String string;
		key->toString (string, true);
		if(result.isEmpty () == false)
			result << " ";
		result << "[" << string << "]";
	EndFor
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

String ComposedTooltip::resolveCommandTitle (StringID category, StringID name)
{
	String result;
	if(KnownCommand* c = unknown_cast<KnownCommand> (CommandTable::instance ().findCommand (category, name)))
		result = c->getDisplayName ();
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IParameter* ComposedTooltip::getParameter ()
{
	Control* control = ccl_cast<Control> (view);
	return control ? control->getParameter () : nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ICommandParameter* ComposedTooltip::getCommandParameter ()
{
	UnknownPtr<ICommandParameter> cmdParam (getParameter ());
	return cmdParam;
}

//************************************************************************************************
// TooltipPopup
//************************************************************************************************

TooltipFactory* TooltipPopup::factory = nullptr;
void TooltipPopup::setFactory (TooltipFactory* _factory)
{
	factory = _factory;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ITooltipPopup* TooltipPopup::createTooltipPopup (View* view)
{
	ITooltipPopup* tooltipWindow = factory ? factory->createTooltipPopup () : nullptr;
	if(tooltipWindow)
	{
		tooltipWindow->construct (view);
		tooltipWindow->moveToMouse ();
		tooltipWindow->setText (ComposedTooltip (view));
	}
	return tooltipWindow;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_CLASS_ABSTRACT_HIDDEN (TooltipPopup, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

TooltipPopup::TooltipPopup ()
: timeToHide (0),
  savedPosition (kMinCoord, kMinCoord),
  exclusiveMode (false)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API TooltipPopup::setDuration (int64 ticks)
{
	if(ticks == kForever)
		timeToHide = 0;
	else
	{
		if(ticks == kDefaultDuration)
			ticks = kTooltipDuration;
		
		timeToHide = System::GetSystemTicks () + ticks;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API TooltipPopup::moveToMouse ()
{
	Point pos;
	GUI.getMousePosition (pos);
	
	// Move inside the monitor the mouse is in
	int monitor = Desktop.findMonitor (pos, true);
	Rect rect;
	Desktop.getMonitorSize (rect, monitor, false);
	rect.contract (48);
	
    ccl_upper_limit (pos.x, rect.right);
    ccl_upper_limit (pos.y, rect.bottom);
	
	pos.y += 28;
	pos.x += 18;
	setPosition (pos);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int64 CCL_API TooltipPopup::getTimeToHide ()
{
	return timeToHide;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringRef CCL_API TooltipPopup::getText () const
{
	return savedText;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TooltipPopup::initColors (View* view)
{
	if(view)
	{
		Theme& theme = view->getTheme ();
		Color backColor = theme.getThemeColor (ThemeElements::kTooltipBackColor);
		Color textColor = theme.getThemeColor (ThemeElements::kTooltipTextColor);
		
		setBackColor (backColor);
		setTextColor (textColor);
	}
}

//************************************************************************************************
// TooltipWindow::WindowImpl
//************************************************************************************************

class TooltipWindow::WindowImpl: public PopupWindow
{
public:
	WindowImpl (const Rect& size, StyleRef style, StringRef title, Window* parent)
	: PopupWindow (size, style, title, parent),
	  parentWindow (parent)
	{}
	
	// Window
	bool onMouseMove (const MouseEvent& event) override
	{
		if(parentWindow)
		{
			MouseEvent e2 (event);
			this->clientToScreen (e2.where);
			parentWindow->screenToClient (e2.where);
			return parentWindow->onMouseMove (e2);
		}
		return false;
	}

protected:
	Window* parentWindow;
};

//************************************************************************************************
// TooltipWindow
//************************************************************************************************

TooltipFactory& TooltipWindow::getFactory ()
{
	struct Factory: TooltipFactory
	{
		ITooltipPopup* createTooltipPopup () override { return NEW TooltipWindow; }
	};

	static Factory theFactory;
	return theFactory;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_CLASS (TooltipWindow, TooltipPopup)

//////////////////////////////////////////////////////////////////////////////////////////////////

TooltipWindow::TooltipWindow ()
: view (nullptr),
  tooltipWindow (nullptr),
  tooltipView (nullptr),
  backColor (Colors::kWhite),
  textColor (Colors::kBlack),
  needsRefresh (false),
  lastRefresh (0)
{
	CCL_PRINTF ("Construct tooltip\n", 0)
}

//////////////////////////////////////////////////////////////////////////////////////////////////

TooltipWindow::~TooltipWindow ()
{
	if(tooltipWindow)
		tooltipWindow->close ();

	CCL_PRINTF ("Destroy tooltip\n", 0)
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API TooltipWindow::construct (IView* iview)
{
	view = unknown_cast<View> (iview);
	ASSERT (view)
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TooltipWindow::setBackColor (Color color)
{		
	backColor = color;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TooltipWindow::setTextColor (Color color)
{
	textColor = color;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API TooltipWindow::show ()
{
	CCL_PRINTF ("Show tooltip\n", 0)
	
	if(tooltipWindow == nullptr)
	{
		initColors (view);
		
		Window* parentWindow = view->getWindow ();

		AutoPtr<VisualStyle> visualStyle = NEW VisualStyle ();
		visualStyle->setColor (StyleID::kTextColor, textColor);
		visualStyle->setColor (StyleID::kBackColor, backColor);
		visualStyle->setOptions (StyleID::kTextAlign, Alignment::kLeftCenter);
		visualStyle->setMetric (StyleID::kPaddingLeft, 4);
		visualStyle->setMetric (StyleID::kPaddingRight, 4);
		visualStyle->setMetric (StyleID::kPaddingTop, 2);
		visualStyle->setMetric (StyleID::kPaddingBottom, 2);

		StyleFlags flags (0, Styles::kLabelMultiLine);
		tooltipView = NEW Label (Rect (0, 0, 100, 14), flags, String::kEmpty);
		tooltipView->setVisualStyle (visualStyle);
		tooltipView->setSizeMode (IView::kAttachAll);
		
		BoxLayoutView* container = NEW BoxLayoutView (Rect (0, 0, 100, 18));
		container->setSizeMode (IView::kFitSize);
		container->setMargin (0);
		container->addView (tooltipView);
		
		ImageView* background = NEW ImageView (nullptr, Rect (0, 0, 100, 18), StyleFlags (0, Styles::kImageViewAppearanceColorize));
		background->setSizeMode (IView::kFitSize);
		background->setVisualStyle (visualStyle);
		background->addView (container);
		
		tooltipWindow = NEW WindowImpl (Rect (0, 0, 100, 18), StyleFlags (0, Styles::kWindowBehaviorFloating|Styles::kWindowAppearanceCustomFrame|Styles::kWindowBehaviorTooltip), String::kEmpty, parentWindow);
		tooltipWindow->setName ("CCL::TooltipWindow");
		tooltipWindow->setSizeMode (IView::kFitSize);
		tooltipWindow->addView (background);
		
        constrainPosition (savedPosition);
		tooltipWindow->setPosition (savedPosition);

		setText (savedText);
	}

    updateWindow ();
	
	#if UPDATE_ON_IDLE
	startTimer ();
	#endif
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API TooltipWindow::hide ()
{
	CCL_PRINTF ("Hide tooltip\n", 0)

	#if UPDATE_ON_IDLE
	stopTimer ();
	#endif
	
	if(tooltipWindow)
		tooltipWindow->showWindow (false);
	
	savedText.empty ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TooltipWindow::constrainPosition (Point& pos)
{
	if(tooltipWindow)
    {
        // keep inside monitor
        int monitor = Desktop.findMonitor (pos, true);
        Rect monitorSize;
        Desktop.getMonitorSize (monitorSize, monitor, true);
        
        ccl_upper_limit (pos.x, monitorSize.right - tooltipWindow->getWidth ());
        ccl_upper_limit (pos.y, monitorSize.bottom - tooltipWindow->getHeight ());
    }
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API TooltipWindow::setPosition (PointRef pos, IView* view)
{
	if(savedPosition == pos)
		return;
	
	savedPosition = pos;
	if(view)
		view->clientToScreen (savedPosition);
	
	constrainPosition (savedPosition);
	#if UPDATE_ON_IDLE
	needsRefresh = true;
	#else
	updateWindow ();
	#endif
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API TooltipWindow::setText (StringRef text)
{
	CCL_PRINTF ("Set tooltip text %s\n", MutableCString (text).str ())
	if(savedText == text)
		return;
		
	savedText = text;
	#if UPDATE_ON_IDLE
	needsRefresh = true;
	#else
	updateWindow ();
	#endif
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TooltipWindow::onIdleTimer ()
{
	int64 now = System::GetSystemTicks ();
	#define kUpdateDelay 20
	if(now - lastRefresh >= kUpdateDelay)
	{
		if(needsRefresh)
		{
			updateWindow ();
			
			needsRefresh = false;
			lastRefresh = now;
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TooltipWindow::updateWindow ()
{
	if(tooltipView)
	{
		Rect size (tooltipView->getSize ());
		Rect rect;
		tooltipView->setTitle (savedText);
		tooltipView->calcAutoSize (rect);

		// prevent empty size to avoid inconsistency in relation to window, which can't have empty size (for empty text, window is hidden anyway below)
		if(rect.getWidth () <= 0)
			rect.setWidth (1);
		if(rect.getHeight () <= 0)
			rect.setHeight (1);

		size.setSize (rect.getSize ());
		tooltipView->setSize (size);
		tooltipView->invalidate ();
	}

	if(tooltipWindow)
	{
		constrainPosition (savedPosition);
		tooltipWindow->setPosition (savedPosition);
		tooltipWindow->showWindow (savedText.length () > 0);
	}
}
