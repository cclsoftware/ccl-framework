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
// Filename    : ccl/system/plugins/typelibregistry.h
// Description : Type Library Registry
//
//************************************************************************************************

#ifndef _ccl_typelibregistry_h
#define _ccl_typelibregistry_h

#include "ccl/base/singleton.h"

#include "ccl/public/base/itypelib.h"
#include "ccl/public/collections/unknownlist.h"
#include "ccl/public/plugins/itypelibregistry.h"

namespace CCL {

//************************************************************************************************
// TypeLibRegistry
//************************************************************************************************

class TypeLibRegistry: public Object,
					   public ITypeLibRegistry,
					   public Singleton<TypeLibRegistry>
{
public:
	DECLARE_CLASS (TypeLibRegistry, Object)

	TypeLibRegistry ();
	~TypeLibRegistry ();

	// ITypeLibRegistry
	tresult CCL_API registerTypeLib (ITypeLibrary& typeLib) override;
	tresult CCL_API unregisterTypeLib (ITypeLibrary& typeLib) override;
	IUnknownIterator* CCL_API newIterator () const override;
	ITypeLibrary* CCL_API findTypeLib (CStringPtr name) const override;

	CLASS_INTERFACE (ITypeLibRegistry, Object)

protected:
	UnknownList typeLibs;
};

} // namespace CCL

#endif // _ccl_typelibregistry_h
