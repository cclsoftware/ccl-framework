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
// Filename    : basetest.cpp
// Description : Various unit tests for cclbase
//
//************************************************************************************************

#include "ccl/base/unittest.h"

#include "ccl/public/collections/bufferchain.h"
#include "ccl/public/systemservices.h"
#include "ccl/public/system/isysteminfo.h"
#include "ccl/public/system/imultiworker.h"
#include "ccl/public/collections/vector.h"

using namespace CCL;
using namespace Threading;

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_TEST (BaseTest, TestBufferChain)
{
	typedef unsigned char Data;

	BufferChain<Data> bufferChain (10);

	Data testData[] = {0xFD, 0xFD, 0xFD, 0xFD, 0xFD, 0xFD, 0xFD, 0xFD, 0xFD, 0xFD, 0xFD, 0xFD, 0xFD, 0xFD, 0xFD, 0xFD};
	Data readData[32];

	for (int i = 0; i < 255; i++)
		bufferChain.append (testData, sizeof(testData) / sizeof(Data));

	for(int i = 0, count = bufferChain.count (); i < count; i += 32)
	{
		bufferChain.read (i, readData, 32);
	}
	
	bufferChain.purge ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

constexpr int kNumberOfCycles = 100;
constexpr int kNumberOfProcesses = 100;
constexpr int kLoopCount = 10000;
constexpr int kBufferSize = 1024;

//////////////////////////////////////////////////////////////////////////////////////////////////

class WorkItem: public Work
{
public:
	void work () override
	{
		for (int i = 0; i < kLoopCount; i++)
		{
			char buffer[kBufferSize];
			memcpy (buffer, buffer, kBufferSize);
		}
	}
};

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_TEST (BaseTest, TestMultiWorker)
{
	CCL::Vector<WorkItem*> workItems (kNumberOfProcesses);
	for(int i = 0; i < kNumberOfProcesses; i++)
		workItems.add (NEW WorkItem);

	// Process threads
	AutoPtr<IMultiWorker> processor = System::CreateMultiThreadWorker ({System::GetSystem ().getNumberOfCPUs (), 0, Threading::kPriorityHigh, false, "TestWorker"});
	for(int n = 0; n < kNumberOfCycles; n++)
	{
		for(int i = 0; i < kNumberOfProcesses; i++)
			processor->push (workItems[i]);

		processor->work ();
	}

	processor->terminate ();

	for(int i = 0; i < kNumberOfProcesses; i++)
		delete workItems[i];
}

//////////////////////////////////////////////////////////////////////////////////////////////////

class MsgBase
{
	virtual char* getId () = 0;
	virtual int getSize () = 0;
	virtual void* getArgs (int i) = 0;
};

template <int argsSize>
class Msg : public MsgBase
{
	char* getId () override { return id; }
	int getSize () override { return size; }
	void* getArgs (int i) override { return args[i]; }

public:
	char* id;
	int size;
	void* args[argsSize];
};

CCL_TEST (BaseTest, TestEventHandling)
{
	Msg<5> msg;
}
