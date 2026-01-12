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
// Filename    : ccl/app/componentaliasfactory.h
// Description : Component Alias Factory
//
//************************************************************************************************

#ifndef _ccl_componentaliasfactory_h
#define _ccl_componentaliasfactory_h

#include "ccl/public/plugservices.h"

namespace CCL {

//************************************************************************************************
// ComponentAliasFactory
//************************************************************************************************

template <class Interface, class Alias>
class ComponentAliasFactory
{
public:
	static Alias* createInstance (UIDRef cid)
	{
		Interface* unk = ccl_new<Interface> (cid);
		if(unk)
		{
			Alias* alias = NEW Alias;
			alias->assignAlias (unk);
			unk->release (); // shared by alias

			if(alias->verifyAlias ()) // success
				return alias;

			alias->release ();
		}
		return nullptr; // failed
	}

	static Alias* createAlternativeInstance (UIDRef cid)
	{
		if(const IClassDescription* alt = System::GetPlugInManager ().getAlternativeClass (cid))
			return createInstance (alt->getClassID ());
		else
			return nullptr;
	}
};

} // namespace CCL

#endif // _ccl_componentaliasfactory_h
