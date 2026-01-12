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
// Filename    : ccl/system/plugins/typelibregistry.cpp
// Description : Type Library Registry
//
//************************************************************************************************

#include "ccl/system/plugins/typelibregistry.h"

#include "ccl/public/plugservices.h"

using namespace CCL;

//////////////////////////////////////////////////////////////////////////////////////////////////
// Plug-in Service APIs
////////////////////////////////////////////////////////////////////////////////////////////////////

CCL_EXPORT ITypeLibRegistry& CCL_API System::CCL_ISOLATED (GetTypeLibRegistry) ()
{
	return TypeLibRegistry::instance ();
}

//************************************************************************************************
// TypeLibRegistry
//************************************************************************************************

DEFINE_SINGLETON (TypeLibRegistry)
DEFINE_CLASS_HIDDEN (TypeLibRegistry, Object)

////////////////////////////////////////////////////////////////////////////////////////////////////

TypeLibRegistry::TypeLibRegistry ()
{}

////////////////////////////////////////////////////////////////////////////////////////////////////

TypeLibRegistry::~TypeLibRegistry ()
{
	ASSERT (typeLibs.isEmpty ())
}

////////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API TypeLibRegistry::registerTypeLib (ITypeLibrary& typeLib)
{
	typeLibs.add (&typeLib, true);
	return kResultOk;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API TypeLibRegistry::unregisterTypeLib (ITypeLibrary& typeLib)
{
	typeLibs.remove (&typeLib);
	typeLib.release ();
	return kResultOk;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

IUnknownIterator* CCL_API TypeLibRegistry::newIterator () const
{
	return typeLibs.createIterator ();
}

////////////////////////////////////////////////////////////////////////////////////////////////////

ITypeLibrary* CCL_API TypeLibRegistry::findTypeLib (CStringPtr _name) const
{
	CString name (_name);
	ForEachUnknown (typeLibs, unk)
		UnknownPtr<ITypeLibrary> typeLib (unk);
		if(typeLib && name == typeLib->getLibraryName ())
			return typeLib;
	EndFor
	return nullptr;
}
