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
// Filename    : ccl/app/presets/presetsystem.cpp
// Description : Interfaces to preset system
//
//************************************************************************************************

#include "ccl/app/presets/presetsystem.h"
#include "ccl/app/presets/presetfileregistry.h"
#include "ccl/app/presets/presetmanager.h"

#include "ccl/public/plugins/iobjecttable.h"
#include "ccl/public/plugservices.h"

namespace CCL {
namespace System {
	
//************************************************************************************************
// PresetSystem
//************************************************************************************************

static bool usingPresetHostInstances = false;
static SharedPtr<IPresetManager> presetManager;
static SharedPtr<IPresetFileRegistry> presetFileRegistry;

//////////////////////////////////////////////////////////////////////////////////////////////////

template<class Interface, class Implementation>
inline Interface& getPresetInstance (SharedPtr<Interface>& instPtr)
{
	if(instPtr == nullptr)
	{
		if(usingPresetHostInstances && System::IsInMainAppModule () == false)
		{
			UnknownPtr<Interface> hostInstance (System::GetObjectTable ().getObjectByID (ccl_iid<Interface> ()));
			ASSERT (hostInstance)
			instPtr = hostInstance;
		}
		else
			instPtr = &Implementation::instance ();

		ASSERT(instPtr)
		if(instPtr == nullptr)
			return Implementation::instance ();
	}

	return *instPtr;
}

} // namespace System
} // namespace CCL

using namespace CCL;

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_KERNEL_INIT_LEVEL (PresetSystem, kFirstRun)
{
	if(System::IsInMainAppModule ())
	{
		System::presetManager = &PresetManager::instance ();  // (add to runtime)
		System::GetObjectTable ().registerObject (System::presetManager, ccl_iid<IPresetManager> (), "PresetManager"); // (publish in object table)

		System::presetFileRegistry = &PresetFileRegistry::instance ();  // (add to runtime)
		System::GetObjectTable ().registerObject (System::presetFileRegistry, ccl_iid<IPresetFileRegistry> (), "PresetFileRegistry"); // (publish in object table)
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_KERNEL_TERM (PresetSystem)
{
	System::presetManager = nullptr;
	System::presetFileRegistry = nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void System::UsePresetHostInstances (bool state)
{
	usingPresetHostInstances = state;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IPresetManager& System::GetPresetManager ()
{
	return getPresetInstance<IPresetManager, PresetManager> (presetManager);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IPresetFileRegistry& System::GetPresetFileRegistry ()
{
	return getPresetInstance<IPresetFileRegistry, PresetFileRegistry> (presetFileRegistry);
}

