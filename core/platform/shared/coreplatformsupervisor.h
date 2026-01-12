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
// Filename    : core/platform/shared/coreplatformsupervisor.h
// Description : System supervisor interface
//
//************************************************************************************************

#ifndef _coreplatformsupervisor_h
#define _coreplatformsupervisor_h

#include "core/public/coretypes.h"

namespace Core {
namespace Platform {

//************************************************************************************************
// ISystemSupervisor
/** 
 * Can be used to request certain privileges from the underlying OS from a non-privileged application.
 * This interface should be implemented outside of Core. 
 */
//************************************************************************************************

struct ISystemSupervisor
{
	/** Get a thread stack for a thread with given name. */
	virtual bool getThreadStack (void*& stack, int& size, CStringPtr threadName) = 0;

	/** Frees stack used by thread for other threads*/
	virtual void freeThreadStack (CStringPtr threadName) = 0;
	
	/** Request access to an existing platform/kernel object. */
	virtual bool grantObjectAccess (void* platformObject) = 0;

	/** Get Maximum Supported Threads, -1 when unlimited*/
	virtual int getMaxThreads () const = 0;
};

//////////////////////////////////////////////////////////////////////////////////////////////////

extern ISystemSupervisor& GetSystemSupervisor ();

} // namespace Platform
} // namespace Core

#endif // _coreplatformsupervisor_h
