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
// Filename    : ccl/public/gui/framework/iparametermenu.h
// Description : Parameter Menu Interfaces
//
//************************************************************************************************

#ifndef _ccl_iparametermenu_h
#define _ccl_iparametermenu_h

#include "ccl/public/base/iunknown.h"

namespace CCL {

interface IMenu;
interface IMenuItem;
interface IParameter;
struct KeyEvent;

//////////////////////////////////////////////////////////////////////////////////////////////////
// Built-in classes
//////////////////////////////////////////////////////////////////////////////////////////////////

namespace ClassID 
{
	DEFINE_CID (ParameterMenuBuilder, 0xb56d5931, 0x2225, 0x42bf, 0x8c, 0x93, 0xe7, 0x61, 0x8e, 0xf1, 0x71, 0x35);
}

//************************************************************************************************
// IParameterMenuBuilder
/** Framework-side interface providing methods to build a customized parameter menu. 
	\ingroup gui_param */
//************************************************************************************************

interface IParameterMenuBuilder: IUnknown
{
	/** Initialize builder, required when created via ccl_new<>. */
	virtual tresult CCL_API construct (IParameter* param) = 0;
	
	/** Create parameter menu. If no menu is given, a menu is created and owned by the caller. */
	virtual IMenu* CCL_API buildIMenu (IMenu* menu = nullptr) = 0;

	/** Add sub menu with given title. */
	virtual IMenuItem* CCL_API addSubMenu (IMenu& menu, IParameter& param, StringRef title) = 0;
	
	/** Find existing sub menu with given title. */
	virtual IMenuItem* CCL_API findSubMenu (IMenu& menu, StringRef title) = 0;

	/** Add item representing the given parameter value. */
	virtual IMenuItem* CCL_API addValueItem (IMenu& menu, IParameter& param, int value) = 0;

	DECLARE_IID (IParameterMenuBuilder)
};

DEFINE_IID (IParameterMenuBuilder, 0x8e75cc94, 0x8a53, 0x4ff3, 0x87, 0xce, 0x79, 0xab, 0x5f, 0x9c, 0xd3, 0xf1)

//************************************************************************************************
// IParameterMenuCustomize
/** Can be implemented by IParameter to customize its popup menu representation. 
	\ingroup gui_param */
//************************************************************************************************

interface IParameterMenuCustomize: IUnknown
{
	/** Return type of menu presentation (see MenuPresentation identifiers). */
	virtual StringID CCL_API getMenuType () const = 0;

	/** Build popup menu for this parameter. */
	virtual tbool CCL_API buildMenu (IMenu& menu, IParameterMenuBuilder& builder) = 0;

	/** Handle keyboard input in parameter menu popup. */
	virtual tbool CCL_API onMenuKeyDown (const KeyEvent& event) = 0;

	DECLARE_IID (IParameterMenuCustomize)
};

DEFINE_IID (IParameterMenuCustomize, 0xd652e096, 0x70e0, 0x41fe, 0x97, 0xc, 0xce, 0x6, 0xd8, 0xb9, 0xa2, 0x34)

} // namespace CCL

#endif // _ccl_iparametermenu_h
