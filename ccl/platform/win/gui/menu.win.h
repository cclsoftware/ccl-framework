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
// Filename    : ccl/platform/win/gui/menu.win.h
// Description : Platform-specific Menu implementation
//
//************************************************************************************************

#ifndef _ccl_menu_win_h
#define _ccl_menu_win_h

#include <windows.h>

#include "ccl/gui/popup/menu.h"

namespace CCL {

//************************************************************************************************
// WindowsPopupMenu
//************************************************************************************************

class WindowsPopupMenu: public PopupMenu
{
public:
	DECLARE_CLASS (WindowsPopupMenu, PopupMenu)

	static WindowsPopupMenu* fromSystemMenu (HANDLE menu);

	WindowsPopupMenu ();
	~WindowsPopupMenu ();

	PROPERTY_VARIABLE (HMENU, handle, Handle)

	// PopupMenu
	IMenu* CCL_API createMenu () const override;
	void updateItem (MenuItem* item) override;
	void realizeItem (MenuItem* item) override;
	void unrealizeItem (MenuItem* item) override;
	IAsyncOperation* popupPlatformMenu (const Point& where, IWindow* window) override;
};

//************************************************************************************************
// WindowsMenuBar
//************************************************************************************************

class WindowsMenuBar: public MenuBar
{
public:
	DECLARE_CLASS (WindowsMenuBar, MenuBar)

	static WindowsMenuBar* fromSystemMenu (HMENU menu);

	WindowsMenuBar ();
	~WindowsMenuBar ();

	PROPERTY_VARIABLE (HMENU, handle, Handle)

	// MenuBar
	void updateMenu (Menu* menu) override;
	void insertPlatformMenu (PopupMenu* menu) override;
	void removePlatformMenu (PopupMenu* menu) override;
};

//************************************************************************************************
// WindowsVariantMenuBar
//************************************************************************************************

class WindowsVariantMenuBar: public VariantMenuBar<WindowsMenuBar>
{
public:
	DECLARE_CLASS (WindowsVariantMenuBar, WindowsMenuBar)
};

} // namespace CCL

#endif // _ccl_menu_win_h
