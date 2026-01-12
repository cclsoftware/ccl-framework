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
// Filename    : ccl/public/system/ithreadpool.h
// Description : Thread Pool Interface
//
//************************************************************************************************

#ifndef _ccl_ithreadpool_h
#define _ccl_ithreadpool_h

#include "ccl/public/base/iunknown.h"

namespace CCL {
namespace Threading {

/** Thread work identifier. */
typedef void* WorkID;

//************************************************************************************************
// IWorkItem
/** Work item used by thread pool. 
	\ingroup ccl_system */
//************************************************************************************************

interface IWorkItem: IUnknown
{
	/** Get identifier. */
	virtual WorkID CCL_API getID () const = 0;

	/** Cancel work. */
	virtual void CCL_API cancel () = 0;

	/** Perform work. */
	virtual void CCL_API work () = 0;

	DECLARE_IID (IWorkItem)
};

DEFINE_IID (IWorkItem, 0x3b288be8, 0xa4d, 0x4ae3, 0x9e, 0x7b, 0xd6, 0x24, 0xa3, 0x64, 0xcb, 0x4)

//************************************************************************************************
// AbstractWorkItem
/** Abstract base class for work items. 
	\ingroup ccl_system */
//************************************************************************************************

class AbstractWorkItem: public IWorkItem
{
public:
	AbstractWorkItem (WorkID id = nullptr)
	: id (id)
	{}
	
	// IWorkItem
	WorkID CCL_API getID () const override { return id; }
	void CCL_API cancel () override {}

protected:
	WorkID id;
};

//************************************************************************************************
// IPeriodicItem
/** Item executed periodically by thread pool.
	\ingroup ccl_system */
//************************************************************************************************

interface IPeriodicItem: IUnknown
{
	/** Get next system time (in milliseconds) this item should be executed. */
	virtual int64 CCL_API getExecutionTime () const = 0;

	/** Execute item, 'now' holds the current system time in milliseconds. */
	virtual void CCL_API execute (int64 now) = 0;

	DECLARE_IID (IPeriodicItem)
};

DEFINE_IID (IPeriodicItem, 0x2be33e57, 0xc555, 0x4d7c, 0xaa, 0xb1, 0xf, 0x4, 0x9, 0xf9, 0xcd, 0x23)

//************************************************************************************************
// IThreadPool
/**	\ingroup ccl_system */
//************************************************************************************************

interface IThreadPool: IUnknown
{
	/** Get maximum number of threads working simultaneously. */
	virtual int CCL_API getMaxThreadCount () const = 0;

	/** Get number of threads currently active. */
	virtual int CCL_API getActiveThreadCount () const = 0;
    
	/** Set the minimum number of threads available and allocate them if neccessary. */
	virtual void CCL_API allocateThreads (int minCount) = 0;
    
	/** Schedule work item (pool takes ownership). */
	virtual void CCL_API scheduleWork (IWorkItem* item) = 0;

	/** Cancel an already scheduled work item. */
	virtual void CCL_API cancelWork (WorkID id, tbool force = false) = 0;

	/** Cancel all scheduled work items. */
	virtual void CCL_API cancelAll () = 0;

	/** Add item to be executed periodically. */
	virtual void CCL_API addPeriodic (IPeriodicItem* item) = 0;

	/** Cancel periodic item. */
	virtual void CCL_API removePeriodic (IPeriodicItem* item) = 0;

	/** Try to reduce number of active threads if idle for while. */
	virtual void CCL_API reduceThreads (tbool force = false) = 0;

	/** Exit all threads and cleanup. */
	virtual void CCL_API terminate () = 0;

	DECLARE_IID (IThreadPool)
};

DEFINE_IID (IThreadPool, 0x1602ee99, 0x3fe, 0x4f93, 0xb3, 0xc1, 0xfc, 0x41, 0xb0, 0x55, 0xab, 0x59)

} // namespace Threading
} // namespace CCL

#endif // _ccl_ithreadpool_h
