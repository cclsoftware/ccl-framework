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
// Filename    : ccl/platform/cocoa/gui/menu.cocoa.h
// Description : Platform-specific Menu implementation
//
//************************************************************************************************

#ifndef _ccl_menu_cocoa_h
#define _ccl_menu_cocoa_h

#include "ccl/gui/popup/menu.h"

@class NSMenu;
@class CCL_ISOLATED (MenuController);

namespace CCL {

//************************************************************************************************
// CocoaPopupMenu
//************************************************************************************************

class CocoaPopupMenu: public PopupMenu
{
public:
	DECLARE_CLASS (CocoaPopupMenu, PopupMenu)

	static CocoaPopupMenu* fromSystemMenu (NSMenu* nsMenu);

	CocoaPopupMenu ();
	~CocoaPopupMenu ();

	PROPERTY_VARIABLE (NSMenu*, menu, NSMenu)

	// PopupMenu
	IMenu* CCL_API createMenu () const override;
	void updateItem (MenuItem* item) override;
	void realizeItem (MenuItem* item) override;
	void unrealizeItem (MenuItem* item) override;
	IAsyncOperation* popupPlatformMenu (const Point& where, IWindow* window) override;

protected:
	CCL_ISOLATED (MenuController)* delegate;
	bool isAppMenu;

	void configureAppMenu ();
	static NSMenu* getAppMenu ();
};

//************************************************************************************************
// CocoaMenuBar
//************************************************************************************************

class CocoaMenuBar: public MenuBar
{
public:
	DECLARE_CLASS (CocoaMenuBar, MenuBar)

	CocoaMenuBar ();
	~CocoaMenuBar ();

	PROPERTY_VARIABLE (NSMenu*, menu, NSMenu)

	static CocoaMenuBar* cast (MenuBar* menu) { return (CocoaMenuBar*) menu; }

	// MenuBar
	void updateMenu (Menu* menu) override;
	void activatePlatformMenu () override;
	void insertPlatformMenu (PopupMenu* menu) override;
	void removePlatformMenu (PopupMenu* menu) override;
};

//************************************************************************************************
// CocoaVariantMenuBar
//************************************************************************************************

class CocoaVariantMenuBar: public VariantMenuBar<CocoaMenuBar>
{
public:
	DECLARE_CLASS (CocoaVariantMenuBar, CocoaMenuBar)
};

} // namespace CCL

#endif // _ccl_menu_cocoa_h
