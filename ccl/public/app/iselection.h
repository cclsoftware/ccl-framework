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
// Filename    : ccl/public/app/iselection.h
// Description : Selection Interface
//
//************************************************************************************************

#ifndef _ccl_iselection_h
#define _ccl_iselection_h

#include "ccl/public/base/iunknown.h"

namespace CCL {

interface IUnknownIterator;

//************************************************************************************************
// ISelection
/**	\ingroup app_inter */
//************************************************************************************************

interface ISelection: IUnknown
{
	virtual int32 CCL_API getEditTag () const = 0;

	virtual IUnknownIterator* CCL_API newIterator (StringID typeName) const = 0;

	virtual tbool CCL_API isObjectSelected (IUnknown* object) const = 0;

	DECLARE_IID (ISelection)
};

DEFINE_IID (ISelection, 0xd5afbba5, 0x674a, 0x41e6, 0xab, 0x9f, 0x78, 0x5a, 0x7d, 0x17, 0xe0, 0xb8)

} // namespace CCL

#endif // _ccl_iselection_h
