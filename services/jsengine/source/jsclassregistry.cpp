//************************************************************************************************
//
// JavaScript Engine
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
// Filename    : jsclassregistry.cpp
// Description : JavaScript Class Registry
//
//************************************************************************************************

#include "jsclassregistry.h"
#include "jscontext.h"

using namespace CCL;
using namespace JScript;

//************************************************************************************************
// ClassRegistry
//************************************************************************************************

ClassRegistry::~ClassRegistry ()
{
	ListForEach (classList, ScriptClass*, c)
		delete c;
	EndFor

	ASSERT (classHashMapList.isEmpty () == true)
	ListForEach (classHashMapList, ClassHashMap*, m)
		delete m;
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ClassRegistry::addModule (ModuleRef module)
{
	ASSERT (moduleHashMap.lookup (module) == nullptr)
	ClassHashMap* classHashMap = NEW ClassHashMap;
	moduleHashMap.add (module, classHashMap);
	classHashMapList.append (classHashMap);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ClassRegistry::removeModule (ModuleRef module)
{
	ClassHashMap* classHashMap = moduleHashMap.lookup (module);
	ASSERT (classHashMap)
	if(!classHashMap)
		return;

	moduleHashMap.remove (module);
	classHashMapList.remove (classHashMap);
	delete classHashMap;

	// TODO: when to remove ScriptClasses???
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ScriptClass* ClassRegistry::lookupClass (const ITypeInfo& typeInfo) const
{
	ClassHashMap* classHashMap = moduleHashMap.lookup (typeInfo.getModuleReference ());
	ScriptClass* scriptClass = classHashMap ? classHashMap->lookup (&typeInfo) : nullptr;
	return scriptClass;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ClassRegistry::addClass (const ITypeInfo& typeInfo, ScriptClass* scriptClass)
{
	ModuleRef module = typeInfo.getModuleReference ();
	ClassHashMap* classHashMap = moduleHashMap.lookup (module);
	ASSERT (classHashMap)
	if(!classHashMap) // addModule() must be called first
		return false;

	classHashMap->add (&typeInfo, scriptClass);
	classList.append (scriptClass);
	return true;
}

