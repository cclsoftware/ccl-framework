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
// Filename    : ccl/public/base/iextensible.h
// Description : Extension Interface
//
//************************************************************************************************

#ifndef _ccl_iextensible_h
#define _ccl_iextensible_h

#include "ccl/public/base/iunknown.h"

namespace CCL {

//************************************************************************************************
// IExtensible
/** Extension interface. 
	\ingroup ccl_base */
//************************************************************************************************

interface IExtensible: IUnknown
{
	/** Get extension by identifier. */
	virtual IUnknown* CCL_API getExtension (StringID id) = 0;

	//////////////////////////////////////////////////////////////////////////////////////////////

	template <typename IFace> 
	IFace* getExtensionI () 
	{ 
		return UnknownPtr<IFace> (getExtension (IFace::kExtensionID)); 
	}

	template <typename IFace> 
	static IFace* getExtensionI (IUnknown* unk) 
	{ 
		UnknownPtr<IExtensible> extensible (unk);
		return extensible ? extensible->getExtensionI<IFace> () : nullptr;
	}

	DECLARE_IID (IExtensible)
};

DEFINE_IID (IExtensible, 0xe28156bc, 0x674a, 0x4663, 0x81, 0xe3, 0x77, 0x27, 0x46, 0x37, 0xf2, 0x26)

} // namespace CCL

#endif // _ccl_iextensible_h
