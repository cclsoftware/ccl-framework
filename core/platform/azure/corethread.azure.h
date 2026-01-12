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
// Filename    : core/platform/azure/corethread.azure.h
// Description : Azure RTOS Thread Primitives
//
//************************************************************************************************

#ifndef _corethread_azure_h
#define _corethread_azure_h

#include "core/platform/shared/coreplatformthread.h"

// Thread X
#include "common/inc/tx_api.h"

namespace Core {
namespace Platform {

//************************************************************************************************
// AzureThread
//************************************************************************************************

class AzureThread: public IThread
{
public:
	AzureThread ();
	~AzureThread ();

	// IThread
	bool open (Threads::ThreadID id) override;
	void start (const ThreadInfo& info) override;
	bool join (uint32 milliseconds) override;
	void terminate () override;
	int getPriority () const override;
	void setPriority (int priority) override;
	void setCPUAffinity (int affinity) override;
	int getPlatformPriority () const override;
	int64 getUserModeTime () const override;
	Threads::ThreadID getID () const override;
	int getErrors () const override { return 0; }

	static void entryWrapper (unsigned long arg);

protected:
	TX_THREAD txThread;
	IThreadEntry* entry;

	static void joinCallback (TX_THREAD* _azureThread, unsigned int type);
};

typedef AzureThread Thread;

const CStringPtr kThreadName = "Azure Thread"; 

//************************************************************************************************
// AzureLock
//************************************************************************************************

class AzureLock: public ILock
{
public:
	AzureLock ();
	~AzureLock ();

	// ILock
	void lock () override;
	bool tryLock () override;
	void unlock () override;

protected:
	TX_MUTEX txMutex;
};

typedef AzureLock Lock;

//************************************************************************************************
// AzureSignal
//************************************************************************************************

class AzureSignal: public ISignal
{
public:
	AzureSignal (bool _manualReset);
	~AzureSignal ();

	// ISignal
	void signal () override;
	void reset () override;
	bool wait (uint32 milliseconds) override;

protected:
	TX_EVENT_FLAGS_GROUP txEvent;
	bool manualReset;
};

typedef AzureSignal Signal;

//************************************************************************************************
// AzureReadWriteLock
//************************************************************************************************

class AzureReadWriteLock: public IReadWriteLock
{
public:
	AzureReadWriteLock ();
	~AzureReadWriteLock ();

	void lockWrite () override;
	void unlockWrite () override;
	void lockRead () override;
	void unlockRead () override;

protected:
	TX_THREAD* owner;
	TX_SEMAPHORE writeSemaphore;
	TX_SEMAPHORE readSemaphore;
	TX_SEMAPHORE activeReadSemaphore;
};

typedef AzureReadWriteLock ReadWriteLock;

} // namespace Platform
} // namespace Core

#endif // _corethread_azure_h
