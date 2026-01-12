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
// Filename    : core/test/corefiletest.cpp
// Description : Core File Tests
//
//************************************************************************************************

#include "corefiletest.h"
#include "core/portable/corefile.h"

using namespace Core;
using namespace Portable;
using namespace Test;

const char kTestString[] = "Core File Test";

//************************************************************************************************
// FileTest
//************************************************************************************************

CORE_REGISTER_TEST (FileTest)

//////////////////////////////////////////////////////////////////////////////////////////////////

CStringPtr FileTest::getName () const
{
	return "Core File";
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool FileTest::run (ITestContext& testContext)
{
	bool succeeded = true;

	FileName tempDir;
	FileUtils::getTempDir (tempDir);

	if(tempDir.isEmpty ())
	{
		CORE_TEST_FAILED ("Default file system does not have a temp directory.")
		return false;
	}

	if(!FileUtils::dirExists (tempDir))
		FileUtils::makeDirectory (tempDir);

	FileName tempFile = tempDir;
	tempFile.descend ("corefiletest.txt");

	if(tempFile == tempDir)
	{
		CORE_TEST_FAILED ("FileName::descend does not work.")
		return false;
	}

	IO::MemoryStream data;
	data.writeBytes (kTestString, ARRAY_COUNT (kTestString));

	if(FileUtils::saveFile (tempFile, data) == false)
	{
		CORE_TEST_FAILED ("Saving a temporary file failed.")
		return false;
	}

	if(FileUtils::fileExists (tempFile) == false)
	{
		CORE_TEST_FAILED ("FileUtils::fileExists does not work.")
		succeeded = false;
	}

	IO::MemoryStream* fileData = FileUtils::loadFile (tempFile);
	if(fileData  == nullptr)
	{
		CORE_TEST_FAILED ("Loading a previously saved file failed.")
		return false;
	}

	if(strcmp (kTestString, fileData->getBuffer ().as<const char> ()) != 0)
	{
		CORE_TEST_FAILED ("File has unexpected content.")
		succeeded = false;
	}
	
	delete fileData;

	bool fileFound = false;
	FileIterator iterator (tempDir);
	while(const FileIterator::Entry* entry = iterator.next ())
	{
		if(entry->name == tempFile)
		{
			fileFound = true;
			break;
		}
	}

	if(fileFound == false)
	{
		CORE_TEST_FAILED ("Could not find the temporary file using a FileIterator.")
		succeeded = false;
	}

	if(FileUtils::deleteFile (tempFile) == false)
	{
		CORE_TEST_FAILED ("Could not delete the temporary file.")
		succeeded = false;
	}

	return succeeded;
}
