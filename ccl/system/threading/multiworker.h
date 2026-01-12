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
// Filename    : ccl/system/threading/multiworker.h
// Description : Multiworker class
//
//************************************************************************************************

#ifndef _ccl_multiworker_h
#define _ccl_multiworker_h

#include "ccl/public/system/imultiworker.h"

#include "ccl/public/base/unknown.h"
#include "ccl/public/collections/vector.h"

namespace CCL {
namespace Threading {

class MultiWorkerSlave;
class MultiWorkerScheduler;

//************************************************************************************************
// MultiWorker
//************************************************************************************************

class MultiWorker: public Unknown,
				   public IMultiWorker
{
public:
	MultiWorker (int numberOfCPUs, int cpuOffset, ThreadPriority priority, bool useCpuAffinity, CStringPtr name, WorkgroupID workgroup);
	~MultiWorker ();

	// IMultiWorker
	void CCL_API terminate () override;
	void CCL_API firstRun () override;
	tbool CCL_API isDone () override;
	int CCL_API work () override;
	void CCL_API push (Work* work) override;
	tbool CCL_API pushAndSignal (Work* work, tbool failWhenAllBusy) override;
	int CCL_API getThreadErrors () const override;

	CLASS_INTERFACE (IMultiWorker, Unknown)
	
protected:
	friend class MultiWorkerSlave;
	
	int numberOfCPUs;
	int cpuOffset;
	bool useCpuAffinity;
	int priority;
	WorkgroupID workgroup;

	int done;
	CCL::Threading::AtomicInt cycleEnd;
	CCL::Threading::AtomicInt working;
	CCL::Threading::AtomicInt finished;
	CCL::Threading::IAtomicStack* workList;
	CCL::Threading::IAtomicStack* availableSlaves;

	CCL::Vector<MultiWorkerSlave*> slaves;	
	
	void doWork (bool master);
	void slaveAvailable (MultiWorkerSlave* slave);
	void slaveWakeup (int maxWakeups);
};

} // namespace Threading
} // namespace CCL

#endif // _ccl_multiworker_h
