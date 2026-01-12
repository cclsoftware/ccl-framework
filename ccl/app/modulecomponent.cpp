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
// Filename    : ccl/app/modulecomponent.cpp
// Description : Module Component
//
//************************************************************************************************

#include "ccl/app/modulecomponent.h"

#include "ccl/base/storage/configuration.h"
#include "ccl/base/storage/attributes.h"

#include "ccl/public/plugins/iobjecttable.h"
#include "ccl/public/plugservices.h"

#include "ccl/public/gui/commanddispatch.h"
#include "ccl/public/gui/framework/icommandtable.h"
#include "ccl/public/guiservices.h"

using namespace CCL;

//************************************************************************************************
// ModuleComponent
//************************************************************************************************

DEFINE_CLASS_HIDDEN (ModuleComponent, Component)

//////////////////////////////////////////////////////////////////////////////////////////////////

ModuleComponent::ModuleComponent (StringID appID, StringRef companyName, StringRef appName,
								  StringRef appVersion, const Attributes* translationVariables)
: Component (CCLSTR ("Module"), appName)
{
	auto& root = RootComponent::instance ();
	root.setApplicationID (appID);
	root.setCompanyName (companyName);
	root.setApplicationVersion (appVersion);
	root.setTitle (appName);
	root.addComponent (this);

	// load strings
	Attributes variables;
	TranslationVariables::setBuiltinVariables (variables);
	if(translationVariables)
	{
		ForEachAttribute (*translationVariables, name, value)
			variables.setAttribute (name, value);
		EndFor
	}
	root.loadStrings (&variables);

	// register commands
	CommandRegistry::registerWithCommandTable ();

	System::GetCommandTable ().addHandler (&root);

	System::GetObjectTable ().registerObject (root.asUnknown (), kNullUID, appID);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ModuleComponent::~ModuleComponent ()
{
	auto& root = RootComponent::instance ();
	root.unloadTheme ();
	root.unloadStrings ();

	System::GetObjectTable ().unregisterObject (root.asUnknown ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API ModuleComponent::initialize (IUnknown* _context)
{
	return SuperClass::initialize (_context);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API ModuleComponent::terminate ()
{
	auto& root = RootComponent::instance ();
	System::GetCommandTable ().removeHandler (&root);
	
	return SuperClass::terminate ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ModuleComponent::loadTheme (UrlRef defaultPath)
{
	auto& root = RootComponent::instance ();
	bool result = root.loadTheme (defaultPath);
	if(result == false)
	{
		CCL_WARN ("ModuleComponent::loadTheme failed\n", 0)
	}
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ModuleComponent::getProperty (Variant& var, MemberID propertyId) const
{
	if(propertyId == "Configuration")
	{
		// accessible via "Host.{appID}.find ('Module').Configuration"
		var = Configuration::Registry::instance ().asUnknown ();
		return true;
	}
	return SuperClass::getProperty (var, propertyId);
}
