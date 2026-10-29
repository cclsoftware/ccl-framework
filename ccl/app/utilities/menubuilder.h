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
interface IParameter;

//************************************************************************************************
// MenuBuilder
//************************************************************************************************

class MenuBuilder
{
public:
	MenuBuilder (IMenu& menu, IUnknown* controller = nullptr);

	IMenu* addSubMenu (StringRef title);
	IMenuItem* addSubMenuWithView (StringRef title, StringID formName, IAttributeList* arguments = nullptr);
	IMenuItem* addViewItem (StringID formName, IAttributeList* arguments = nullptr);

	void setItemIcon (IMenuItem* menuItem, StringID iconName);
	void setItemValueFromParam (IMenuItem* item, IParameter* param, bool closeOnChange);

	IMenu* operator -> () { return &menu; } // enable access to IMenu methods

protected:
	class ParamMenuItemObserver;

	IMenu& menu;
	IUnknown* controller;

	void addSubMenuInternal (IMenu*& subMenu, IMenuItem*& subMenuItem, StringRef title);
	ITheme* getTheme () const;
};

} // namespace CCL

#endif // _ccl_menubuilder_h
