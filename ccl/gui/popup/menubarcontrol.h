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
// Filename    : ccl/gui/popup/menubarcontrol.h
// Description : Menu Bar Control
//
//************************************************************************************************

#ifndef _ccl_menubarcontrol_h
#define _ccl_menubarcontrol_h

#include "ccl/gui/controls/control.h"

namespace CCL {

class Menu;
class MenuBar;
class ThemeRenderer;

//************************************************************************************************
// MenuBarControl
/** Shows a menu bar, i.e. a list of menu buttons which in turn show a popup menu when activated. */
//************************************************************************************************

class MenuBarControl: public Control
{
public:
	DECLARE_CLASS (MenuBarControl, Control)

	static MenuBarControl* getActiveControl ();
	static MenuBarControl* findInWindow (Window& window);

	MenuBarControl (const Rect& size = Rect (), StyleRef style = 0);
	~MenuBarControl ();

	DECLARE_STYLEDEF (customStyles)

	DECLARE_STRINGID_MEMBER (kActivateMenu)

	void setMenuBar (MenuBar* bar);
	const MenuBar* getMenuBar () const;

	int countMenus () const;
	Menu* getMenu (int index) const;
	int findMenu (uchar character) const;
	int findMenu (const Point& loc);

	void activateMenu (int index);
	int getActiveIndex () const;

	void setFocusMenu (int index);
	int getFocusMenu () const;
	int getMouseDownMenu () const;

	StringRef getMenuTitle (String& title, int index) const;
	IImage* getMenuIcon (int index) const;

	bool isInKeyEvent () const;

	// Control
	void CCL_API notify (ISubject* subject, MessageRef msg) override;
	void onSize (const Point& delta) override;
	MouseHandler* createMouseHandler (const MouseEvent& event) override;
	bool onMouseEnter (const MouseEvent& event) override;
	bool onMouseLeave (const MouseEvent& event) override;
	bool onMouseMove (const MouseEvent& event) override;
	bool onKeyDown (const KeyEvent& event) override;
	bool onKeyUp (const KeyEvent& event) override;
	bool onFocus (const FocusEvent& event) override;
	void onActivate (bool state) override;
	void draw (const UpdateRgn& updateRgn) override;
	void calcAutoSize (Rect& r) override;

	enum MenuBarParts
	{
		kPartNone      = 0,
		kPartBar       = 1,
		kPartFirstMenu = 100,
		kPartLastMenu  = 200
	};

private:
	class TouchMouseHandler;
	static MenuBarControl* activeControl; // valid while a popup menu is open
	ThemeRenderer* renderer;
	MenuBar* menuBar;
	int focusMenu;
	int mouseDownMenu;
	int activeMenu;
	bool activatePending;
	bool inKeyEvent;
	int64 lastKeyEventTime;
	
	bool mouseDown (const MouseEvent& event);
	bool mouseUp (const MouseEvent& event);
	void setMouseDown (int index);
	int wrapAroundIndex (int index) const;
	
	bool closeActiveMenu ();
	void invalidateMenu (int index);

	// Control
	ThemeRenderer* getRenderer () override;
};

} // namespace CCL 

#endif // _ccl_menubarcontrol_h
