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
// Filename    : ccl/system/threading/multiworker.cpp
// Description : Multiworker class
//
//************************************************************************************************

#define DEBUG_LOG 0
#define TEST_ERROR_HANDLING (DEBUG && 0)

#include "core/system/corespinlock.h"

#include "ccl/system/threading/multiworker.h"

#include "ccl/public/text/cstring.h"
#include "ccl/public/system/isysteminfo.h"
#include "ccl/public/system/imediathreading.h"

namespace CCL {
namespace Threading {

//************************************************************************************************
// MultiWorkerSlave
//************************************************************************************************

class MultiWorkerSlave : public IAtomicStack::Element
{
public:
	MultiWorkerSlave (MultiWorker* master, int cpuIndex, ThreadPriority priority, CStringPtr name);
	~MultiWorkerSlave ();

	void waitDead ();
	void wakeup ();
	int getTheadErrors () const;

private:
	MultiWorker* master;
	CCL::Threading::IThread* thread;
	CCL::Threading::Signal workSignal;
	int cpuIndex;
	ThreadPriority priority;

	void run ();
	static int CCL_API entry (void* param);
};

}}

using namespace CCL;
using namespace Threading;

//////////////////////////////////////////////////////////////////////////////////////////////////
// System Threading APIs
//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_EXPORT IMultiWorker* System::CCL_ISOLATED (CreateMultiThreadWorker) (const MultiThreadWorkerDescription& description)
{
	return NEW MultiWorker (description.numberOfCPUs, 
							description.cpuOffset, 
							description.priority, 
							description.useCpuAffinity != 0, 
							description.name,
							description.workgroup);
}

//************************************************************************************************
// MultiWorkerSlave
//************************************************************************************************

MultiWorkerSlave::MultiWorkerSlave (MultiWorker* master, int cpuIndex, ThreadPriority priority, CStringPtr name)
: master (master),
  cpuIndex (cpuIndex),
  priority (priority)  
{
    next = nullptr;

	thread = System::CreateNativeThread ({entry, name, this});
	thread->setPriority (priority);
	thread->start ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

MultiWorkerSlave::~MultiWorkerSlave ()
{
	if(thread)
		thread->release ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void MultiWorkerSlave::wakeup ()
{
	workSignal.signal ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void MultiWorkerSlave::run ()
{
	if(cpuIndex >= 0)
		thread->setCPUAffinity (cpuIndex);

	while(true)
	{
		master->slaveAvailable (this);
		workSignal.wait (kWaitForever);

		if(master->done == 1)
			break;

		if(master->cycleEnd == false) // do not process outside of main processing cycle
			master->doWork (false);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int MultiWorkerSlave::getTheadErrors () const
{
	return thread->getErrors ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void MultiWorkerSlave::waitDead ()
{
#if DEBUG
	thread->join (3600000);
#else
	thread->join (10000);
#endif
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API MultiWorkerSlave::entry (void* param)
{
	MultiWorkerSlave* worker = (MultiWorkerSlave*)param;
	
	WorkgroupJoinScope workgroupScope (System::GetMediaThreadService ().getWorkgroupHandler (), worker->master->workgroup);
	if(workgroupScope.isError ())
	{
		ASSERT (0)
		return -1;
	}

	worker->run ();

	return 0;
}

//************************************************************************************************
// MultiWorker
//************************************************************************************************

MultiWorker::MultiWorker (int _numberOfCPUs, int _cpuOffset, ThreadPriority priority, bool useCpuAffinity, CStringPtr name, WorkgroupID workgroup)
: numberOfCPUs (_numberOfCPUs),
  cpuOffset (_cpuOffset),
  workList (nullptr),
  availableSlaves (nullptr),
  useCpuAffinity (useCpuAffinity),
  priority (priority),
  workgroup (workgroup)
{
	done = 0;
	cycleEnd = false;

	workList = System::CreateAtomicStack ();
	availableSlaves = System::CreateAtomicStack ();

	if(useCpuAffinity && workgroup != nullptr)
		useCpuAffinity = false;
	
	#if !TEST_ERROR_HANDLING
	if(useCpuAffinity)
	{
		int cpuCount = System::GetSystem ().getNumberOfCores ();
		while(numberOfCPUs + cpuOffset > cpuCount)
		{
			if(cpuOffset > 0)
				cpuOffset--;
			else if(numberOfCPUs > 0)
				numberOfCPUs--;
		}	
	}	
	#endif

	// slaves 
	for(int slaveIndex = 0; slaveIndex < numberOfCPUs - 1; slaveIndex++)
	{
		int cpuIndex = useCpuAffinity ? (1 + slaveIndex + cpuOffset) : -1;
		slaves.add (NEW MultiWorkerSlave (this, cpuIndex, priority, MutableCString ().appendFormat ("%s%d", name, slaveIndex + 1)));
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

MultiWorker::~MultiWorker ()
{
	workList->release ();
	availableSlaves->release ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void MultiWorker::slaveAvailable (MultiWorkerSlave* slave)
{
	slave->next = nullptr;
	availableSlaves->push (slave);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void MultiWorker::slaveWakeup (int maxWakeups)
{
	for(int i = 0; i < maxWakeups; i++)
	{
		if(IAtomicStack::Element* e = availableSlaves->pop ())
			static_cast <MultiWorkerSlave*> (e)->wakeup ();
		else
			break;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API MultiWorker::firstRun ()
{
	if(System::IsInMainThread () == false)
	{
		if(useCpuAffinity)
		{
			IThread* thread = System::CreateThreadSelf ();
			thread->setCPUAffinity (cpuOffset);
			thread->release ();
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API MultiWorker::terminate ()
{
	workList->flush ();
	done = 1;

	for(int i = 0; i < slaves.count (); i++)
	{
		MultiWorkerSlave* slave = slaves.at (i);
		slave->wakeup ();
		slave->waitDead ();
		delete slave;
	}

	slaves.removeAll ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API MultiWorker::work ()
{
	finished.assign (0);
	
	cycleEnd = false;
	while(true)
	{
		if(workList->depth () > 0)
			doWork (true);

		if(working.getValue () > 0)
			Core::CoreSpinLock::wait ();
		else
			break;			
	}	
	cycleEnd = true;

	return finished.getValue ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API MultiWorker::pushAndSignal (Work* work, tbool failWhenAllBusy)
{
	if(failWhenAllBusy && availableSlaves->depth () <= 0)
		return false;

	workList->push (work);
	slaveWakeup (1);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API MultiWorker::push (Work* work)
{
	workList->push (work);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void MultiWorker::doWork (bool isMaster)
{
	working.increment ();

	while(IAtomicStack::Element* e = workList->pop ())
	{
		if(isMaster)
			slaveWakeup (workList->depth ());

		Work* work = (Work*)e;
		work->work ();

		finished.increment ();
	}

	working.decrement ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool MultiWorker::isDone ()
{
	return done != 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API MultiWorker::getThreadErrors () const
{
	int errors = 0;
	for(MultiWorkerSlave* slave : slaves)
		errors |= slave->getTheadErrors ();
		
	return errors;
}
