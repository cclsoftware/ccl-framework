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
// Filename    : ccl/public/plugins/icomponent.h
// Description : Component Interface
//
//************************************************************************************************

#ifndef _ccl_icomponent_h
#define _ccl_icomponent_h

#include "ccl/public/base/iunknown.h"

namespace CCL {

//////////////////////////////////////////////////////////////////////////////////////////////////
// Component Categories
//////////////////////////////////////////////////////////////////////////////////////////////////

/** Generic category for components. */
#define PLUG_CATEGORY_COMPONENT_ "Component"
#define PLUG_CATEGORY_COMPONENT CCLSTR (PLUG_CATEGORY_COMPONENT_)

/** Category for User Services. */
#define PLUG_CATEGORY_USERSERVICE_ "UserService"
#define PLUG_CATEGORY_USERSERVICE CCLSTR (PLUG_CATEGORY_USERSERVICE_)

/** Category for Program Services. */
#define PLUG_CATEGORY_PROGRAMSERVICE_ "ProgramService"
#define PLUG_CATEGORY_PROGRAMSERVICE CCLSTR (PLUG_CATEGORY_PROGRAMSERVICE_)

/** Category for Framework Services. */
#define PLUG_CATEGORY_FRAMEWORKSERVICE_ "FrameworkService"
#define PLUG_CATEGORY_FRAMEWORKSERVICE CCLSTR (PLUG_CATEGORY_FRAMEWORKSERVICE_)

namespace Meta 
{
	/** Service priority class attribute. */
	DEFINE_STRINGID (kServicePriority, "servicePriority")
}

//************************************************************************************************
// IComponent
/** Basic plug-in component interface.
	\ingroup base_plug */
//************************************************************************************************

interface IComponent: IUnknown
{
	/** Initialize plug-in component within specified context. */
	virtual tresult CCL_API initialize (IUnknown* context = nullptr) = 0;

	/** Terminate plug-in component. */
	virtual tresult CCL_API terminate () = 0;

	/** Check if plug-in component can be terminated. */
	virtual tbool CCL_API canTerminate () const = 0;

	DECLARE_IID (IComponent)
};

DEFINE_IID (IComponent, 0x11688bdb, 0xf961, 0x4535, 0xac, 0xda, 0x1d, 0x29, 0x75, 0x8e, 0xc6, 0xc8)

//************************************************************************************************
// IComponentAlias
//************************************************************************************************

interface IComponentAlias: IUnknown
{
	/** Get plug-in object represented by alias component. */
	virtual IUnknown* CCL_API getPlugInUnknown () const = 0;

	/** Get original host context from alias component. */
	virtual IUnknown* CCL_API getHostContext () = 0;

	DECLARE_IID (IComponentAlias)
};

DEFINE_IID (IComponentAlias, 0x3c288d77, 0x4cd1, 0x4481, 0x86, 0x64, 0x7f, 0x42, 0xf6, 0x9b, 0x57, 0xd4)

} // namespace CCL

#endif // _ccl_icomponent_h
