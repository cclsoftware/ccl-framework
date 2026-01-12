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
// Filename    : ccl/app/utilities/menubuilder.h
// Description : Menu Builder
//
//************************************************************************************************

#ifndef _ccl_menubuilder_h
#define _ccl_menubuilder_h

#include "ccl/public/gui/framework/imenu.h"

namespace CCL {

interface ITheme;

//************************************************************************************************
// MenuBuilder
//************************************************************************************************

class MenuBuilder
{
public:
	MenuBuilder (IMenu& menu, IUnknown* controller = nullptr);

	IMenu* addSubMenu (StringRef title);
	void addSubMenuWithView (StringRef title, StringID formName);
	void addViewItem (StringID formName);

	void setItemIcon (IMenuItem* menuItem, StringID iconName);

	IMenu* operator -> () { return &menu; } // enable access to IMenu methods

protected:
	IMenu& menu;
	IUnknown* controller;

	ITheme* getTheme () const;
};

} // namespace CCL

#endif // _ccl_menubuilder_h
