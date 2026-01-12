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
// Filename    : ccl/public/base/itrigger.h
// Description : Trigger Interfaces
//
//************************************************************************************************

#ifndef _ccl_itrigger_h
#define _ccl_itrigger_h

#include "ccl/public/base/iunknown.h"

namespace CCL {

interface IObject;

//************************************************************************************************
// ITriggerPrototype
/** Trigger prototype interface.
	\ingroup ccl_base */
//************************************************************************************************

interface ITriggerPrototype: IUnknown
{
	/** Apply trigger to target. */
	virtual void CCL_API applyTrigger (IObject* target) = 0;

	DECLARE_IID (ITriggerPrototype)
};

DEFINE_IID (ITriggerPrototype, 0x4b80f8b4, 0x9b1d, 0x42d1, 0xa6, 0x83, 0xd8, 0xd1, 0x7b, 0xd7, 0xe1, 0xb)

//************************************************************************************************
// ITriggerAction
/** Trigger action interface.
	\ingroup ccl_base */
//************************************************************************************************

interface ITriggerAction: IUnknown
{
	/** Execute action for given target. */
	virtual void CCL_API execute (IObject* target) = 0;

	DECLARE_IID (ITriggerAction)
};

DEFINE_IID (ITriggerAction, 0x3076da7, 0xa23d, 0x4fb5, 0xbe, 0xb7, 0xcc, 0x44, 0x30, 0x85, 0x8f, 0x95)

} // namespace CCL

#endif // _ccl_itrigger_h
