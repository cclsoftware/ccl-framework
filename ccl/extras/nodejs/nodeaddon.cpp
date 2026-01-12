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
// Filename    : ccl/extras/nodejs/nodeaddon.cpp
// Description : Node addon base class
//
//************************************************************************************************

#include "ccl/extras/nodejs/nodeaddon.h"

#include "ccl/app/component.h"

#include "ccl/base/storage/attributes.h"

#include "ccl/public/plugins/iobjecttable.h"
#include "ccl/public/plugservices.h"

#include "ccl/public/system/isysteminfo.h"
#include "ccl/public/systemservices.h"

using namespace CCL;
using namespace NodeJS;

//*************************************************************************************************
// NodeAddon
//*************************************************************************************************

NodeAddon* NodeAddon::theInstance = nullptr;
NodeAddon& NodeAddon::getInstance ()
{
	ASSERT (theInstance != nullptr)
	return *theInstance;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

NodeAddon::NodeAddon (CStringPtr _moduleId)
: environment (nullptr),
  moduleId (_moduleId)
{
	ASSERT (theInstance == nullptr)
	theInstance = this;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

NodeAddon::~NodeAddon ()
{
	theInstance = nullptr;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

bool NodeAddon::startup (napi_env environment)
{
	this->environment = environment;
	return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

void NodeAddon::shutdown ()
{
	auto& root = RootComponent::instance ();
	root.unloadStrings ();
	root.unloadTheme ();
}

///////////////////////////////////////////////////////////////////////////////////////////////////

void NodeAddon::initApp (StringID appID, StringRef companyName, StringRef appName, StringRef appVersion, int versionInt)
{
	System::SetInMainAppModule (true);

	auto& root = RootComponent::instance ();
	root.setApplicationID (appID);
	root.setTitle (appName);
	root.setCompanyName (companyName);
	root.setApplicationVersion (appVersion);

	Attributes variables;
	TranslationVariables::setBuiltinVariables (variables);
	root.loadStrings (&variables);

	System::GetSystem ().setApplicationName (appName, appName, versionInt);
	System::GetObjectTable ().registerObject (root.asUnknown (), kNullUID, appID, IObjectTable::kIsHostApp);
}
