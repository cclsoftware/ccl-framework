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
// Filename    : core/test/coretestrunner.cpp
// Description : Unit Test Runner
//
//************************************************************************************************

#include "coretestbase.h"
#include "coretestrunner.h"
#include "core/system/coredebug.h"
#include "core/public/corestringbuffer.h"

using namespace Core;
using namespace Test;

#ifdef CORE_PLATFORM_WINDOWS
#define TestPrintf printf
#else
#define TestPrintf DebugPrintf
#endif

//************************************************************************************************
// TestContext
//************************************************************************************************

class TestContext: public ITestContext
{
public:
	struct FailureInfo
	{
		CString256 message;
		CString256 sourceFile;
		int lineNumber = 0;
	};

	const Vector<FailureInfo>& getFailures () const
	{
		return failures;
	}

	// ITestContext
	void addMessage (CStringPtr message, CStringPtr sourceFile, int lineNumber) override
	{
		TestPrintf ("%s:%d %s\n", sourceFile, lineNumber, message);
	}

	void addFailure (CStringPtr message, CStringPtr sourceFile, int lineNumber) override
	{
		TestPrintf ("%s:%d %s\n", sourceFile, lineNumber, message);
		failures.add ({message, sourceFile, lineNumber});
	}

private:
	Vector<FailureInfo> failures;
};

//************************************************************************************************
// coreTest
//************************************************************************************************

int Core::coreTest (int argc, char* argv[])
{
	if(argc > 1 && strcmp (argv[1], "list") == 0)
	{
		for(int i = 0; i < TestRegistry::instance ().getTests ().count (); ++i)
			TestPrintf ("%d: %s\n", i + 1, Test::TestRegistry::instance ().getTests ()[i]->getName ());
	}	
	else if(argc > 1 && strcmp (argv[1], "run") == 0)
	{
		TestContext context;
		
		int64 index = 0;
		if(argc > 2)
			index = atoi (argv[2]);
		if(index > 0 && index <= Test::TestRegistry::instance ().getTests ().count ())
			Test::TestRegistry::instance ().getTests ()[index - 1]->run (context);
		else
			Test::TestRegistry::instance ().runAllTests (context);

		int numFailed = context.getFailures ().count ();
		if(numFailed > 0)
		{
			TestPrintf ("Summary: %i tests failed!\n", numFailed);
			for(const auto& failure : context.getFailures ())
				TestPrintf ("%s:%d %s\n", failure.sourceFile.str (), failure.lineNumber, failure.message.str ());
		}
		else
		{
			TestPrintf ("Summary: All tests passed!\n");
		}
	}
	else
	{
		TestPrintf ("Usage: coretest [list|run [<id>]]\n");
	}
	TestPrintf ("done\n");
	return 0;
}
