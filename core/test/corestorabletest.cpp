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
// Filename    : core/test/corestorabletest.cpp
// Description : Core Storable Tests
//
//************************************************************************************************

#include "corestorabletest.h"

#include "core/public/corestorable.h"
#include "core/public/corememstream.h"

using namespace Core;
using namespace Test;
using namespace IO;

//************************************************************************************************
// TestStorable
//************************************************************************************************

struct TestStorable: public IStreamStorable
{
	TestStorable (int value)
	: value (value)
	{}

	// IStreamStorable
	tbool save (IByteStream& stream) const
	{
		BinaryStreamAccessor accessor (stream);
		return accessor.write (value);
	}
	
	virtual tbool load (IByteStream& stream)
	{
		BinaryStreamAccessor accessor (stream);
		return accessor.read (value);
	}

	int value;
};

//************************************************************************************************
// StorableTest
//************************************************************************************************

CORE_REGISTER_TEST (StorableTest)

//////////////////////////////////////////////////////////////////////////////////////////////////

CStringPtr StorableTest::getName () const
{
	return "Core Storable";
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool StorableTest::run (ITestContext& testContext)
{
	bool succeeded = true;

	MemoryStream stream;

	TestStorable testStorable (5);
	if(!testStorable.save (stream))
	{
		succeeded = false;
		CORE_TEST_FAILED ("Could not store a simple IStreamStorable to a stream");
	}

	testStorable.value = 0;
	
	stream.setPosition (0, kSeekSet);
	if(!testStorable.load (stream) || testStorable.value != 5)
	{
		succeeded = false;
		CORE_TEST_FAILED ("Could not restore a simple IStreamStorable from a stream");
	}

	TestStorable storables[] = { TestStorable (0), TestStorable (1), TestStorable (2), TestStorable (3), TestStorable (4) };
	ContainerStorer::Item storableItems[] = { { storables[0], 0 }, { storables[1], 1 }, { storables[2], 2 }, { storables[3], 3 }, { storables[4], 4 } };
	ContainerStorer storer (stream, storableItems, ARRAY_COUNT (storableItems));
	
	if(!storer.storeAll ())
	{
		succeeded = false;
		CORE_TEST_FAILED ("Failed to store multiple IStreamStorable's using a ContainerStorer.");
	}

	for(int i = 0; i < ARRAY_COUNT (storables); ++i)
		storables[i].value = -1;
	
	if(!storer.restore (3) || storables[3].value != 3)
	{
		succeeded = false;
		CORE_TEST_FAILED ("Failed to restore a single IStreamStorable using a ContainerStorer.");
	}

	if(!storer.restoreAll () || storables[0].value != 0 || storables[1].value != 1 || storables[2].value != 2 || storables[3].value != 3 || storables[4].value != 4)
	{
		succeeded = false;
		CORE_TEST_FAILED ("Failed to restore multiple IStreamStorable's using a ContainerStorer.");
	}

	return succeeded;
}
