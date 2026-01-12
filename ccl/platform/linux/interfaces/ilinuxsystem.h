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
// Filename    : ccl/platform/linux/interfaces/ilinuxsystem.h
// Description : Linux System Interfaces
//
//************************************************************************************************

#ifndef _ccl_ilinuxsystem_h
#define _ccl_ilinuxsystem_h

#include "ccl/platform/linux/interfaces/idbussupport.h"

namespace CCL {

//************************************************************************************************
// ILinuxSystem
//************************************************************************************************

interface ILinuxSystem: IUnknown
{
	virtual void CCL_API setDBusSupport (IDBusSupport* dbusSupport) = 0;

	virtual IDBusSupport* CCL_API getDBusSupport () const = 0;

	DECLARE_IID (ILinuxSystem)
};

DEFINE_IID (ILinuxSystem, 0xc908ba73, 0xe81e, 0x456c, 0x84, 0xe4, 0x2e, 0x19, 0x5d, 0x0f, 0x39, 0xc0)

} // namespace CCL

#endif // _ccl_ilinuxsystem_h
