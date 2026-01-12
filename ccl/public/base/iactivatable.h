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
// Filename    : ccl/public/base/iactivatable.h
// Description : Activatable Interface
//
//************************************************************************************************

#ifndef _ccl_iactivatable_h
#define _ccl_iactivatable_h

#include "ccl/public/base/iunknown.h"

namespace CCL {

//************************************************************************************************
// IActivatable
/** Interface for object activation. 
	\ingroup ccl_base */
//************************************************************************************************

interface IActivatable: IUnknown
{
	/** Check if object is currently active. */
	virtual tbool CCL_API isActive () const = 0;

	/** Activate the object. */
	virtual void CCL_API activate () = 0;

	/** Deactivate the object. */
	virtual void CCL_API deactivate () = 0;

	DECLARE_STRINGID_MEMBER (kActivate) ///< Activation signal
	DECLARE_STRINGID_MEMBER (kDeactivate) ///< Deactivation signal

	DECLARE_IID (IActivatable)
};

DEFINE_IID (IActivatable, 0xa3461fe4, 0x8b36, 0x4ff0, 0x95, 0xf3, 0x2b, 0x84, 0xb, 0x7a, 0xb9, 0x26)
DEFINE_STRINGID_MEMBER (IActivatable, kActivate, "activate")
DEFINE_STRINGID_MEMBER (IActivatable, kDeactivate, "deactivate")

} // namespace CCL

#endif // _ccl_iactivatable_h
