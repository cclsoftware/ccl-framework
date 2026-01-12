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
// Filename    : ccl/app/modulecomponent.h
// Description : Module Component
//
//************************************************************************************************

#ifndef _ccl_modulecomponent_h
#define _ccl_modulecomponent_h

#include "ccl/app/component.h"

#include "ccl/main/cclmodmain.h"

namespace CCL {

//************************************************************************************************
// ModuleComponent
//************************************************************************************************

class ModuleComponent: public Component
{
public:
	DECLARE_CLASS (ModuleComponent, Component)

	ModuleComponent (StringID appID = nullptr, StringRef companyName = nullptr, StringRef appName = nullptr,
					 StringRef appVersion = nullptr, const Attributes* translationVariables = nullptr);
	~ModuleComponent ();
	
	bool loadTheme (UrlRef defaultPath);

	// Component
	tresult CCL_API initialize (IUnknown* context = nullptr) override;
	tresult CCL_API terminate () override;
	
protected:
	// IObject
	tbool CCL_API getProperty (Variant& var, MemberID propertyId) const override;
};

} // namespace CCL

#endif // _ccl_modulecomponent_h
