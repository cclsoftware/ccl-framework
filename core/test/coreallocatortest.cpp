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
// Filename    : core/test/coreallocatortest.cpp
// Description : Core Allocator Tests
//
//************************************************************************************************

#include "coreallocatortest.h"

#include "core/public/corepoolallocator.h"

using namespace Core;
using namespace Test;

//************************************************************************************************
// AllocatorTest
//************************************************************************************************

CORE_REGISTER_TEST (AllocatorTest)

//////////////////////////////////////////////////////////////////////////////////////////////////

CStringPtr AllocatorTest::getName () const
{
	return "Core Allocator";
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool AllocatorTest::run (ITestContext& testContext)
{
	Allocator& defaultAllocator = Allocator::getDefault ();

	uint32 testBufferSize = 513 * sizeof(float);
	float* testBuffer = static_cast<float*> (defaultAllocator.allocate (testBufferSize));

	if(testBuffer == nullptr)
	{
		CORE_TEST_FAILED ("Failed to allocate a buffer from default allocator.")
		return false;
	}

	testBuffer[0] = 5.f;
	testBuffer[512] = 7.f;
	
	testBufferSize = 1021 * sizeof(float);
	testBuffer = static_cast<float*> (defaultAllocator.reallocate (testBuffer, testBufferSize));
	
	if(testBuffer == nullptr)
	{
		CORE_TEST_FAILED ("Failed to reallocate a buffer from default allocator.")
		return false;
	}

	defaultAllocator.deallocate (testBuffer);

	PoolAllocator<float, 64, 8> poolAllocator;

	testBuffer = static_cast<float*> (poolAllocator.allocate (testBufferSize));
	
	if(testBuffer != nullptr)
	{
		CORE_TEST_FAILED ("PoolAllocator allocated a chunk that is too large to fit in the pool.")
		return false;
	}
	
	testBufferSize = 512 * sizeof(float);
	testBuffer = static_cast<float*> (poolAllocator.allocate (testBufferSize));
	
	if(testBuffer == nullptr)
	{
		CORE_TEST_FAILED ("Failed to allocate a buffer from PoolAllocator.")
		return false;
	}
	
	
	testBufferSize = 64 * sizeof(float);
	testBuffer = static_cast<float*> (poolAllocator.reallocate (testBuffer, testBufferSize));
	
	if(testBuffer == nullptr)
	{
		CORE_TEST_FAILED ("Failed to reallocate a buffer from PoolAllocator.")
		return false;
	}

	float* buffers[7];
	for(int i = 0; i < ARRAY_COUNT (buffers); ++i)
	{
		buffers[i] = static_cast<float*> (poolAllocator.allocate (testBufferSize));
		if(buffers[i] == nullptr)
		{
			CORE_TEST_FAILED ("Failed to allocate a buffer from PoolAllocator.")
			return false;
		}
		if(i > 0 && buffers[i] - buffers[i - 1] < 64)
		{
			CORE_TEST_FAILED ("Allocated buffers overlap.")
			return false;
		}
	}
	
	for(int i = 0; i < ARRAY_COUNT (buffers); ++i)
		poolAllocator.deallocate (buffers[i]);

	poolAllocator.deallocate (testBuffer);

	return true;
}
