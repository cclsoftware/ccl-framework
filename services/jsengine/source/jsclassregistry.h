//************************************************************************************************
//
// JavaScript Engine
//
// This file is part of Crystal Class Library (R)
// Copyright (c) 2026 CCL Software Licensing GmbH.
// All Rights Reserved.
//
// Licensed for use under either:
//  1. a Commercial License provided by CCL Software Licensing GmbH, or
//  2. GNU Affero General Public License v3.0 (AGPLv3).
//
// You must choose and comply with one of the above licensing options.
// For more information, please visit ccl.dev.
//
// Filename    : jsclassregistry.h
// Description : JavaScript Class Registry
//
//************************************************************************************************

#ifndef _jsclassregistry_h
#define _jsclassregistry_h

#include "ccl/public/base/iobject.h"
#include "ccl/public/collections/hashmap.h"
#include "ccl/public/collections/linkedlist.h"

namespace JScript {

//************************************************************************************************
// ClassRegistry
/** Backend-agnostic bookkeeping for native-class registration. */
//************************************************************************************************

template <class ScriptClass>
class ClassRegistry
{
public:
	~ClassRegistry ()
	{
		ListForEach (classList, ScriptClass*, c)
			delete c;
		EndFor

		ASSERT (classHashMapList.isEmpty () == true)
		ListForEach (classHashMapList, ClassHashMap*, m)
			delete m;
		EndFor
	}

	void addModule (CCL::ModuleRef module)
	{
		ASSERT (moduleHashMap.lookup (module) == nullptr)
		ClassHashMap* classHashMap = NEW ClassHashMap;
		moduleHashMap.add (module, classHashMap);
		classHashMapList.append (classHashMap);
	}

	void removeModule (CCL::ModuleRef module)
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

	ScriptClass* lookupClass (const CCL::ITypeInfo& typeInfo) const
	{
		ClassHashMap* classHashMap = moduleHashMap.lookup (typeInfo.getModuleReference ());
		ScriptClass* scriptClass = classHashMap ? classHashMap->lookup (&typeInfo) : nullptr;
		return scriptClass;
	}

	bool addClass (const CCL::ITypeInfo& typeInfo, ScriptClass* scriptClass)
	{
		CCL::ModuleRef module = typeInfo.getModuleReference ();
		ClassHashMap* classHashMap = moduleHashMap.lookup (module);
		ASSERT (classHashMap)
		if(!classHashMap) // addModule() must be called first
			return false;

		classHashMap->add (&typeInfo, scriptClass);
		classList.append (scriptClass);
		return true;
	}

protected:
	class ClassHashMap: public CCL::PointerHashMap<ScriptClass*>
	{
	public:
		ClassHashMap ()
		: CCL::PointerHashMap<ScriptClass*> (512)
		{}
	};

	class ModuleHashMap: public CCL::PointerHashMap<ClassHashMap*>
	{
	public:
		ModuleHashMap ()
		: CCL::PointerHashMap<ClassHashMap*> (512)
		{}
	};

	CCL::LinkedList<ClassHashMap*> classHashMapList;
	CCL::LinkedList<ScriptClass*> classList;
	ModuleHashMap moduleHashMap;
};

} // namespace JScript

#endif // _jsclassregistry_h
