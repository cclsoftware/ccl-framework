//************************************************************************************************
//
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
// Filename    : corefixedsupervisor.h
// Description : Fixed Stack Memory System Supervisor
//
//************************************************************************************************

#ifndef _corefixedsupervisor_h
#define _corefixedsupervisor_h

#include "core/platform/shared/coreplatformsupervisor.h"
#include "core/portable/coresingleton.h"
#include "core/public/corestringbuffer.h"

namespace Core {
namespace Platform {

//************************************************************************************************
// FixedSystemSupervisor
//************************************************************************************************

template <class SupervisorConfig>
class FixedSystemSupervisor: public ISystemSupervisor,
							 public Portable::StaticSingleton<FixedSystemSupervisor<SupervisorConfig>>
{
public:
	// ISystemSupervisor
	bool getThreadStack (void*& stack, int& size, CStringPtr threadName);
	void freeThreadStack (CStringPtr threadName);
	bool grantObjectAccess (void* platformObject);
	int getMaxThreads () const;

protected:
	CString64 stackAssignments[SupervisorConfig::maxThreads];
};

//************************************************************************************************
// SystemSupervisor
//************************************************************************************************

template <class SupervisorConfig>
bool FixedSystemSupervisor<SupervisorConfig>::getThreadStack (void*& stack, int& size, CStringPtr threadName)
{
	if(threadName != nullptr)
	{
		for(int n = 0; n < SupervisorConfig::maxThreads; n++)
		{
			if(stackAssignments[n] == threadName)
			{
				stack = SupervisorConfig::getStack (n);
				size = SupervisorConfig::stackSize;
				return true;
			}
		}

		for(int n = 0; n < SupervisorConfig::maxThreads; n++)
		{
			if(stackAssignments[n].isEmpty ())
			{
				stack = SupervisorConfig::getStack (n);
				size = SupervisorConfig::stackSize;
				stackAssignments[n] = threadName;
				return true;
			}
		}
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class SupervisorConfig>
void FixedSystemSupervisor<SupervisorConfig>::freeThreadStack (CStringPtr threadName)
{
	if(threadName != nullptr)
	{
		for(int n = 0; n < SupervisorConfig::maxThreads; n++)
		{
			if(stackAssignments[n] == threadName)
			{
				stackAssignments[n].empty ();
				break;
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class SupervisorConfig>
bool FixedSystemSupervisor<SupervisorConfig>::grantObjectAccess (void* platformObject)
{
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class SupervisorConfig>
int FixedSystemSupervisor<SupervisorConfig>::getMaxThreads () const
{
	return SupervisorConfig::maxThreads;
}

} // namespace Platform
} // namespace Core

#endif // _corefixedsupervisor_h
