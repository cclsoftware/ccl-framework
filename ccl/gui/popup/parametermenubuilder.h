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
// Filename    : ccl/gui/popup/parametermenubuilder.h
// Description : Parameter Menu Builder
//
//************************************************************************************************

#ifndef _ccl_parametermenubuilder_h
#define _ccl_parametermenubuilder_h

#include "ccl/base/object.h"

#include "ccl/public/gui/iparameter.h"
#include "ccl/public/gui/iparamobserver.h"
#include "ccl/public/gui/icommandhandler.h"
#include "ccl/public/gui/framework/iparametermenu.h"

namespace CCL {

class Menu;
class MenuItem;
class MenuItemIDSet;

//************************************************************************************************
// ParameterMenuBuilder
/** Builds a menu for a given parameter, sets the parameter's value when a menu item is selected. */
//************************************************************************************************

class ParameterMenuBuilder: public Object,
							public ICommandHandler,
							public IParameterMenuBuilder
{
public:
	DECLARE_CLASS (ParameterMenuBuilder, Object)

	ParameterMenuBuilder (IParameter* param = nullptr);

	Menu* buildMenu (Menu* menu = nullptr); ///< if no menu is given, a Menu is created and owned by the caller

	PROPERTY_BOOL (defaultTitleEnabled, DefaultTitleEnabled)
	PROPERTY_BOOL (extensionEnabled, ExtensionEnabled)

	static IParameter* extractParameter (Menu& menu);
	static ParameterMenuBuilder* extractBuilder (Menu& menu);

	// ICommandHandler
	tbool CCL_API checkCommandCategory (CStringRef category) const override;
	tbool CCL_API interpretCommand (const CommandMsg& msg) override;

	CLASS_INTERFACE2 (ICommandHandler, IParameterMenuBuilder, Object)

private:
	class ParamData: public Object
	{
	public:
		DECLARE_CLASS (ParamData, Object)

		PROPERTY_SHARED_AUTO (ParameterMenuBuilder, builder, Builder)
		PROPERTY_SHARED_AUTO (IParameter, parameter, Parameter)

		// Object
		tresult CCL_API queryInterface (UIDRef iid, void** ptr) override;
	};

	MenuItemIDSet* menuIDs;
	IParameter* parameter;
	UnknownPtr<IParamPreviewHandler> previewHandler;

	~ParameterMenuBuilder ();

	void prepareMenu (Menu& menu, IParameter& param, StringRef title);
	MenuItem* addItem (Menu& menu, IParameter& param, int value);
	bool buildCustomized (Menu& menu, IParameter& param);
	bool buildMenu (Menu& menu, IParameter& param);	///< returns true if an item was checked

	// IParameterMenuBuilder
	tresult CCL_API construct (IParameter* param) override;
	IMenu* CCL_API buildIMenu (IMenu* menu = nullptr) override;
	IMenuItem* CCL_API addSubMenu (IMenu& menu, IParameter& param, StringRef title) override;
	IMenuItem* CCL_API findSubMenu (IMenu& menu, StringRef title) override;
	IMenuItem* CCL_API addValueItem (IMenu& menu, IParameter& param, int value) override;
};

} // namespace CCL

#endif // _ccl_parametermenubuilder_h
