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
// Filename    : core/platform/zephyr/corestaticthread.zephyr.h
// Description : Zephyr Static Threads
//
//************************************************************************************************

#ifndef _corestaticthread_zephyr_h
#define _corestaticthread_zephyr_h

//////////////////////////////////////////////////////////////////////////////////////////////////
// Thread Helper Macros
//////////////////////////////////////////////////////////////////////////////////////////////////

#define PLATFORM_THREAD(name) (_k_thread_obj__##name)
#define PLATFORM_THREAD_STACK(name) (_k_thread_stack__##name)
	
/*!
 * \brief Declare static core thread and stack storage.
 * \details Default priority is the lowest available in the system.
 * Note that actual stack size may vary from declaration.
 * 
 * \param partition memory partition the Core::Threads::Thread instance is allocated from
 * \param owner name of thread who will start() and own this thread (eg. fwapp)
 * \param classname name of custom thread derived from Core::Thread
 * \param name name of thread instance
 * \param stackSize target stack size
 * \param options options passed to K_THREAD_DEFINE (e.g. K_USER)
 */
#define DECLARE_STATIC_THREAD(partition, owner, classname, name, stackSize, options) \
	K_APP_DMEM (partition) classname name; \
	DECLARE_STATIC_THREAD_ (owner, name, stackSize, options)
	
#define DECLARE_STATIC_THREAD_(owner, name, stackSize, options) \
	extern k_thread _k_thread_obj_##owner; \
	K_THREAD_DEFINE (_##name, stackSize, \
					Core::Platform::ThreadEntry, &name.getPlatformThread (), 0, 0, \
					K_LOWEST_APPLICATION_THREAD_PRIO, options, K_FOREVER); \
	DEFINE_INITIALIZER (name##_thread_initializer) \
	{ \
		k_thread_access_grant (&_k_thread_obj_##owner, &PLATFORM_THREAD (name)); \
		name.getPlatformThread ().setKernelThread (&PLATFORM_THREAD (name)); \
		name.getPlatformThread ().setKernelStack (PLATFORM_THREAD_STACK (name)); \
	}

//////////////////////////////////////////////////////////////////////////////////////////////////

extern struct _static_thread_data __static_thread_data_list_start[];
extern struct _static_thread_data __static_thread_data_list_end[];	

//////////////////////////////////////////////////////////////////////////////////////////////////
	
namespace Core {
namespace Platform {
namespace StaticThreads {
	
template<typename T>
inline void forEach (T callback)
{
	for(_static_thread_data* data = __static_thread_data_list_start; data < __static_thread_data_list_end; ++data)
		if(data && data->init_entry == ThreadEntry)
			callback (data->init_thread);
}

//////////////////////////////////////////////////////////////////////////////////////////////////
	
inline void setMemoryDomain (k_mem_domain* domain)
{
	forEach ([domain](k_thread* thread)
	{
		k_mem_domain_add_thread (domain, thread);
	});
}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline void setResourcePool (k_mem_pool* pool)
{
	forEach ([pool](k_thread* thread)
	{
		k_thread_resource_pool_assign (thread, pool);
	});
}

} // namespace StaticThreads
} // namespace Platform
} // namespace Core
	 
#endif // _corestaticthread_zephyr_h
