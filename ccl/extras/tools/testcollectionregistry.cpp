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
// Filename    : testcollectionregistry.cpp
// Description : Unit Test Registry
//
//************************************************************************************************

#include "testcollectionregistry.h"

#include "ccl/public/base/iprogress.h"
#include "ccl/public/systemservices.h"
#include "ccl/public/plugins/ipluginmanager.h"
#include "ccl/public/plugservices.h"
#include "ccl/public/text/iregexp.h"

using namespace CCL;

//************************************************************************************************
// TestRegistry
//************************************************************************************************

DEFINE_SINGLETON (TestCollectionRegistry)

//////////////////////////////////////////////////////////////////////////////////////////////////

void TestCollectionRegistry::registerTestPlugIns ()
{
	ASSERT (testCollections.isEmpty ())

	ForEachPlugInClass (PLUG_CATEGORY_UNITTEST, desc)
		ITestCollection* testCollection = ccl_new<ITestCollection> (desc.getClassID ());
		if(testCollection)
		{
			int suiteCount = testCollection->countSuites ();
			for(int i = 0; i < suiteCount; i++)
			{
				auto* suite = testCollection->getSuite (i);

				if(suite != nullptr)
					testSuites.add (suite);
			}

			testCollections.add (testCollection);
		}
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TestCollectionRegistry::unregisterTestPlugIns ()
{
	for(auto* suite : testSuites)
		testSuites.remove (suite);

	for(auto* collection : testCollections)
		ccl_release (collection);

	testCollections.removeAll ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const Vector<ITestSuite*>& TestCollectionRegistry::getTestSuites () const
{
	return testSuites;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TestCollectionRegistry::collectTests (Vector<TestDescription>& result, StringRef filterExpression) const
{
	for(auto* suite : testSuites)
	{
		String suiteName = suite->getName ();

		int testCount = suite->countTests ();
		for(int i = 0; i < testCount; i++)
		{
			String testName = suite->getTestName (i);
			if(!matches (suiteName, testName, filterExpression))
				continue;

			result.add ({suite, i});
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

String TestCollectionRegistry::toRegex (StringRef filterExpression) const
{
	String result = filterExpression;

	if(result.startsWith ("*"))
		result.prepend (".");
	else
		result.prepend ("^");

	if(result.endsWith ("*"))
		result.replace ("*", ".*");
	else
		result.append ("$");

	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool TestCollectionRegistry::matches (StringRef suiteName, StringRef testName, StringRef filterExpression) const
{
	String matchName = suiteName;
	matchName.append ("_");
	matchName.append (testName);

	String filterRegex = toRegex (filterExpression);
	AutoPtr<IRegularExpression> regExp = System::CreateRegularExpression ();
	bool expressionIsValid = regExp->construct (filterRegex, IRegularExpression::kCaseInsensitive) == kResultOk;
	ASSERT (expressionIsValid)
	if(!expressionIsValid)
		return false;

	return regExp->isFullMatch (matchName);
}
