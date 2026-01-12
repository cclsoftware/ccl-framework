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
// Filename    : ccl/public/plugins/itypelibregistry.h
// Description : Type Library Registry Interface
//
//************************************************************************************************

#ifndef _ccl_itypelibregistry_h
#define _ccl_itypelibregistry_h

#include "ccl/public/base/iunknown.h"

namespace CCL {

interface ITypeLibrary;
interface IUnknownIterator;

//************************************************************************************************
// ITypeLibRegistry
/** \ingroup base_plug */
//************************************************************************************************

interface ITypeLibRegistry: IUnknown
{
	/** Register type library. */
	virtual tresult CCL_API registerTypeLib (ITypeLibrary& typeLib) = 0;
	
	/** Unregister type library. */
	virtual tresult CCL_API unregisterTypeLib (ITypeLibrary& typeLib) = 0;
	
	/** Create iterator of ITypeLibrary objects. */
	virtual IUnknownIterator* CCL_API newIterator () const = 0;

	/** Find registered type library by name. */
	virtual ITypeLibrary* CCL_API findTypeLib (CStringPtr name) const = 0;
	
	DECLARE_IID (ITypeLibRegistry)
};

DEFINE_IID (ITypeLibRegistry, 0xa27c968a, 0x5b36, 0x44bc, 0x95, 0xe1, 0x97, 0x3, 0x96, 0x3b, 0x1c, 0x80)

} // namespace CCL

#endif // _ccl_itypelibregistry_h
