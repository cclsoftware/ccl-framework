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
// Filename    : jsclassregistry.h
// Description : JavaScript Class Registry
//
//************************************************************************************************

#ifndef _jsclassregistry_h
#define _jsclassregistry_h

#include "ccl/public/base/uid.h"
#include "ccl/public/base/iobject.h"
#include "ccl/public/base/primitives.h"
#include "ccl/public/collections/hashmap.h"
#include "ccl/public/collections/linkedlist.h"

namespace JScript {

class ScriptClass;

//************************************************************************************************
// ClassRegistry
//************************************************************************************************

class ClassRegistry
{
public:
	~ClassRegistry ();
	
	void addModule (CCL::ModuleRef module);
	void removeModule (CCL::ModuleRef module);

	ScriptClass* lookupClass (const CCL::ITypeInfo& typeInfo) const;
	bool addClass (const CCL::ITypeInfo& typeInfo, ScriptClass* scriptClass);

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

