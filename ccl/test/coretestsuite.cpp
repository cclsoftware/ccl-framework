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
// Filename    : coretestsuite.cpp
// Description : Unit tests for Core
//
//************************************************************************************************

#include "coretestsuite.h"
#include "core/test/coretestbase.h"

using namespace CCL;

//************************************************************************************************
// CoreTestSuite
//************************************************************************************************

CoreTestSuite::CoreTestSuite ()
: context (nullptr)
{
	for(Core::Test::TestBase* test : Core::Test::TestRegistry::instance ().getTests ())
		testNames.add (test->getName ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringRef CCL_API CoreTestSuite::getName () const
{
	return CCLSTR ("CoreTestSuite");
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API CoreTestSuite::countTests () const
{
	return Core::Test::TestRegistry::instance ().getTests ().count ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringRef CCL_API CoreTestSuite::getTestName (int index) const
{
	return testNames.at (index);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API CoreTestSuite::setUp ()
{
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API CoreTestSuite::tearDown ()
{
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API CoreTestSuite::runTest (int index, CCL::ITestContext* _context)
{
	context = _context;

	Core::Test::TestBase* test = Core::Test::TestRegistry::instance ().getTests ().at (index);

	if(test == nullptr)
		return kResultOk;

	tbool succeeded = test->run (*this);

	context = nullptr;

	return succeeded ? kResultOk : kResultFailed;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CoreTestSuite::addMessage (CStringPtr message, CStringPtr sourceFile, int lineNumber)
{
	if(context)
		context->addPass (message, sourceFile, lineNumber);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CoreTestSuite::addFailure (CStringPtr message, CStringPtr sourceFile, int lineNumber)
{
	if(context)
		context->addFailure (message, sourceFile, lineNumber);
}

