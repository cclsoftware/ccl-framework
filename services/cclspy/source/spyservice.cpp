//************************************************************************************************
//
// CCL Spy
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
// Filename    : spyservice.cpp
// Description : Spy Service plugin
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "spyservice.h"
#include "spymanager.h"

using namespace CCL;
using namespace Spy;

//************************************************************************************************
// SpyService
//************************************************************************************************

IUnknown* SpyService::createInstance (UIDRef, void*)
{
	return static_cast<IComponent*> (NEW SpyService);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

SpyService::SpyService ()
: manager (NEW SpyManager)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

SpyService::~SpyService ()
{
	if(manager)
		manager->release ();
}

/////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API SpyService::initialize (IUnknown* context)
{
	// Spy Manager
	if(manager)
		manager->initialize (context);

	return ServicePlugin::initialize (context);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API SpyService::terminate ()
{
	if(manager)
		manager->terminate ();

	return ServicePlugin::terminate ();
}
