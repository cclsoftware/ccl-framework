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
// Filename    : ccl/gui/popup/menubarcontrol.cpp
// Description : Menu Bar Control
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "menubarcontrol.h"

#include "ccl/gui/theme/renderer/menubarrenderer.h"
#include "ccl/gui/views/mousehandler.h"
#include "ccl/gui/popup/popupselector.h"
#include "ccl/gui/popup/menu.h"
#include "ccl/gui/graphics/imaging/image.h"
#include "ccl/gui/windows/desktop.h"
#include "ccl/gui/gui.h"

#include "ccl/base/message.h"

#include "ccl/public/systemservices.h"

using namespace CCL;

//************************************************************************************************
// MenuBarControl::TouchMouseHandler
//************************************************************************************************

class MenuBarControl::TouchMouseHandler: public MouseHandler
{
public:
	TouchMouseHandler (MenuBarControl* menuBar)
	: MouseHandler (menuBar)
	{}

	void onRelease (bool canceled) override
	{ 
		ASSERT (current.wasTouchEvent ())
		if(canceled)
			return;

		MenuBarControl* menuBar = (MenuBarControl*)view;
		int partCode = menuBar->getRenderer ()->hitTest (menuBar, current.where, nullptr);
		if(partCode >= MenuBarControl::kPartFirstMenu && partCode <= MenuBarControl::kPartLastMenu)
		{
			menuBar->mouseDown (current);
			menuBar->redraw ();
			menuBar->mouseUp (current);
		}
	} 
};

//************************************************************************************************
// MenuBarControl
//************************************************************************************************

DEFINE_CLASS_HIDDEN (MenuBarControl, Control)

DEFINE_STRINGID_MEMBER_ (MenuBarControl, kActivateMenu, "activateMenu")

MenuBarControl* MenuBarControl::activeControl = nullptr;

//////////////////////////////////////////////////////////////////////////////////////////////////

MenuBarControl* MenuBarControl::getActiveControl ()
{
	return activeControl;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

MenuBarControl* MenuBarControl::findInWindow (Window& window)
{
	AutoPtr<IRecognizer> r (Recognizer::create ([] (IUnknown* unk) { return unknown_cast<MenuBarControl> (unk) != nullptr; }));
	return ccl_cast<MenuBarControl> (window.findView (*r));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

MenuBarControl::MenuBarControl (const Rect& size, StyleRef style)
: Control (size, nullptr, style),
  renderer (nullptr),
  menuBar (nullptr),
  focusMenu (-1),
  mouseDownMenu (-1),
  activeMenu (-1),
  activatePending (false),
  inKeyEvent (false),
  lastKeyEventTime (0)
{
	setParameter (nullptr); // release parameter of base class
	enable (true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

MenuBarControl::~MenuBarControl ()
{
	cancelSignals ();

	setMenuBar (nullptr);

	if(renderer)
		renderer->release ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void MenuBarControl::setMenuBar (MenuBar* bar)
{
	share_and_observe (this, menuBar, bar);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const MenuBar* MenuBarControl::getMenuBar () const
{
	return menuBar;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void MenuBarControl::draw (const UpdateRgn& updateRgn)
{
	ThemeRenderer* renderer = getRenderer ();
	if(renderer)
		renderer->draw (this, updateRgn);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void MenuBarControl::calcAutoSize (Rect& r)
{
	ThemeRenderer* renderer = getRenderer ();
	if(renderer)
		renderer->getPartRect (this, kPartBar, r);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ThemeRenderer* MenuBarControl::getRenderer ()
{
	if(renderer == nullptr)
		renderer = getTheme ().createRenderer (ThemePainter::kMenuBarRenderer, visualStyle);
	return renderer;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void MenuBarControl::onSize (const Point& delta)
{
	invalidate ();
	SuperClass::onSize (delta);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void MenuBarControl::activateMenu (int index)
{
	if(activeMenu == index)
		return;

	if(closeActiveMenu ())
	{
		activatePending = true;
		(NEW Message (kActivateMenu, index, inKeyEvent))->post (this, 10);
		return;
	}
	
	Menu* menu = getMenu (index);
	if(menu)
	{
		activeMenu = index;
		invalidateMenu (activeMenu);

		setFocusMenu (index);
			
		PopupSizeInfo sizeInfo (this, PopupSizeInfo::kCanFlipParentEdge);
		Rect rect;
		getRenderer ()->getPartRect (this, kPartFirstMenu + index, rect);
		sizeInfo.where = rect.getLeftBottom ();

		ScopedVar<MenuBarControl*> scope (activeControl, this);
		
		PopupSelector popupSelector;
		popupSelector.setTheme (getTheme ());
		popupSelector.setVisualStyle (getTheme ().getStandardStyle (ThemePainter::kPopupMenuStyle));
		popupSelector.setMenuMode (true);
		popupSelector.popup (menu, sizeInfo);
		
		invalidateMenu (activeMenu);
		activeMenu = -1;

		if(activatePending)
			setFocusMenu (-1); // avoid drawing old menu as focused while kActivateMenu message is pending
		else
			setFocusMenu (index); // keep focus on closed menu
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool MenuBarControl::closeActiveMenu ()
{
	if(activeMenu >= 0)
	{
		if(IWindow* topModal = Desktop.getTopWindow (kPopupLayer))
		{
			UnknownPtr<IPopupSelectorClient> popupClient (topModal);
			if(popupClient)
				return topModal->close ();
		}
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Menu* MenuBarControl::getMenu (int index) const
{
	return menuBar ? unknown_cast<Menu> (menuBar->getMenu (index)) : nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringRef MenuBarControl::getMenuTitle (String& title, int index) const
{
	Menu* menu = getMenu (index);
	if(menu)
		title = menu->getTitle ();
	return title;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IImage* MenuBarControl::getMenuIcon (int index) const
{
	Menu* menu = getMenu (index);
	if(menu)
		return menu->getIcon ();
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int MenuBarControl::countMenus () const
{
	return menuBar ? menuBar->countMenus () : 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int MenuBarControl::getActiveIndex () const
{
	return activeMenu;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int MenuBarControl::findMenu (const Point& loc)
{
	int partCode = getRenderer ()->hitTest (this, loc, nullptr);
	if(partCode >= kPartFirstMenu && partCode <= kPartLastMenu)
		return partCode - kPartFirstMenu;
	return -1;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int MenuBarControl::findMenu (uchar character) const
{
	if(menuBar)
	{
		character = Unicode::toUppercase (character);

		for(int index = 0, num = menuBar->countMenus (); index < num; index++)
		{
			auto* menu = unknown_cast<Menu> (menuBar->getMenu (index));
			if(menu && Unicode::toUppercase (menu->getTitle ().firstChar ()) == character)
				return index;
		}
	}
	return -1;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int MenuBarControl::getFocusMenu () const
{
	return focusMenu;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int MenuBarControl::getMouseDownMenu () const
{
	return mouseDownMenu;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool MenuBarControl::onMouseEnter (const MouseEvent& event)
{
	return onMouseMove (event);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool MenuBarControl::onMouseLeave (const MouseEvent& event)
{
	setMouseState (IView::kMouseNone);

	if(!isFocused ())
		setFocusMenu (-1);

	setMouseDown (-1);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool MenuBarControl::onMouseMove (const MouseEvent& event)
{
	setMouseState (IView::kMouseOver);

	// ignore synthetic mouse events triggered from Window::onKeyUp ()
	int64 now = System::GetSystemTicks ();
	if(now - lastKeyEventTime < 100)
		return true;

	int partCode = getRenderer ()->hitTest (this, event.where, nullptr);
	bool isOverMenu = partCode >= kPartFirstMenu && partCode <= kPartLastMenu;

	if(isOverMenu)
	{
		int index = partCode - kPartFirstMenu;
		if(event.keys.isSet (KeyState::kLButton))
			setMouseDown (index);
		else
			setFocusMenu (index);
		return true;
	}

	if(!isFocused ())
		setFocusMenu (-1);

	setMouseDown (-1);
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void MenuBarControl::setFocusMenu (int index)
{
	if(index != focusMenu)
	{
		invalidateMenu (index);
		invalidateMenu (focusMenu);
		focusMenu = index;
	}
	
	if(activeMenu >= 0 && index >= 0 && index != activeMenu)
		activateMenu (index);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void MenuBarControl::setMouseDown (int index)
{
	if(index != mouseDownMenu)
	{
		invalidateMenu (index);
		invalidateMenu (mouseDownMenu);
		mouseDownMenu = index;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int MenuBarControl::wrapAroundIndex (int index) const
{
	int numMenus = countMenus ();
	return numMenus > 0 ? (index + numMenus) % countMenus () : -1;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void MenuBarControl::invalidateMenu (int index)
{
	if(!isAttached ())
		return;
	
	Rect rect;
	getRenderer ()->getPartRect (this, kPartFirstMenu + index, rect);
	invalidate (rect);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool MenuBarControl::mouseDown (const MouseEvent& event)
{
	int partCode = getRenderer ()->hitTest (this, event.where, nullptr);
	if(partCode >= kPartFirstMenu && partCode <= kPartLastMenu)
	{
		onMouseMove (event);
		takeFocus ();
		return true;
	}
	return SuperClass::onMouseDown (event);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool MenuBarControl::mouseUp (const MouseEvent& event)
{
	int partCode = getRenderer ()->hitTest (this, event.where, nullptr);
	if(partCode >= kPartFirstMenu && partCode <= kPartLastMenu)
	{
		activateMenu (partCode - kPartFirstMenu);
		setMouseDown (-1);
		return true;
	}
	return SuperClass::onMouseUp (event);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

MouseHandler* MenuBarControl::createMouseHandler (const MouseEvent& event)
{
	int partCode = getRenderer ()->hitTest (this, event.where, nullptr);
	if(partCode >= kPartFirstMenu && partCode <= kPartLastMenu)
	{
		SharedPtr<MenuBarControl> keeper (this);
		
		if(event.wasTouchEvent ())
			return NEW TouchMouseHandler (this);
		
		mouseDown (event);
		redraw ();
		mouseUp (event);
		
		return NEW NullMouseHandler (this);
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool MenuBarControl::onKeyDown (const KeyEvent& event)
{
	ScopedVar scope (inKeyEvent, true);
	lastKeyEventTime = System::GetSystemTicks ();

	switch(event.vKey)
	{
	case VKey::kLeft:
	case VKey::kRight:
		if(countMenus () > 0)
		{
			if(getActiveIndex () >= 0)
			{
				// navigate through open menus 
				int delta = event.vKey == VKey::kLeft ? -1 : +1;
				activateMenu (wrapAroundIndex (getActiveIndex () + delta));
				return true;
			}
			else if(getFocusMenu () >= 0)
			{
				// navigate through closed menus
				int delta = event.vKey == VKey::kLeft ? -1 : +1;
				setFocusMenu (wrapAroundIndex (getFocusMenu () + delta));
				return true;
			}
		}
		break;

	case VKey::kReturn:
	case VKey::kUp:
	case VKey::kDown:
		// open current focus menu
		if(focusMenu >= 0)
		{
			activateMenu (focusMenu);
			return true;
		}
		break;

	case VKey::kEscape:
		// give up focus if no menu is open (MenuControl would close an open menu on the first kEscape)
		ASSERT (getActiveIndex () < 0)
		killFocus ();
		return true;

	case VKey::kOption:
		// give up focus
		killFocus ();
		return true;

	case VKey::kUnknown:
		// character key activates the matching menu (if none is open already)
		if(Unicode::isAlpha (event.character) && getActiveIndex () < 0)
		{
			int index = findMenu (event.character);
			if(index >= 0)	
			{
				// activating the menu has priority when character was pressed alone; with option modifier, key commands are preferred
				if(event.state.isSet (KeyState::kOption))
					if(GUI.translateKey (event))
						return true;

				activateMenu (index);
				return true;
			}
		}
		break;
	}

	return SuperClass::onKeyDown (event);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool MenuBarControl::onKeyUp (const KeyEvent& event)
{
	ScopedVar scope (inKeyEvent, true);
	lastKeyEventTime = System::GetSystemTicks ();

	if(event.vKey == VKey::kOption)
	{
		if(!isFocused ())
		{
			// focus first menu
			setFocusMenu (0);
			takeFocus ();
			return true;
		}
	}
	return SuperClass::onKeyUp (event);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool MenuBarControl::isInKeyEvent () const
{
	return inKeyEvent;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool MenuBarControl::onFocus (const FocusEvent& event)
{
	bool setFocus = event.eventType == FocusEvent::kSetFocus;
	if(setFocus != isFocused ())
	{
		isFocused (setFocus);
		if(!setFocus)
			setFocusMenu (-1);

		invalidate ();
	}
	return SuperClass::onFocus (event);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void MenuBarControl::onActivate (bool state)
{
	setFocusMenu (-1); // discard when window is deactivated

	Window* window = getWindow ();
	if(window && window->getFocusView () == this)
		window->killFocusView (true);

	SuperClass::onActivate (state);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API MenuBarControl::notify (ISubject* subject, MessageRef msg)
{
	if(msg == kActivateMenu && msg.getArgCount() > 0)
	{
		ScopedVar<bool> scope (inKeyEvent, msg.getArgCount () > 1 ? msg.getArg (1).asBool () : inKeyEvent);
		activateMenu (msg.getArg (0).asInt ());
		activatePending = false;
	}
	else if(msg == kChanged && menuBar && isEqualUnknown (subject, menuBar->asUnknown ()))
	{
		invalidate ();
	}
	SuperClass::notify (subject, msg);
}
