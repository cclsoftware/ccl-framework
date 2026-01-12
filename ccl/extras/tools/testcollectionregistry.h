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
// Filename    : testcollectionregistry.h
// Description : Unit Test Registry
//
//************************************************************************************************

#ifndef _ccl_testcollectionregistry_h
#define _ccl_testcollectionregistry_h

#include "ccl/base/singleton.h"

#include "ccl/public/text/cclstring.h"
#include "ccl/public/collections/vector.h"

#include "ccl/public/base/iunittest.h"

namespace CCL {

//************************************************************************************************
// TestDescription
//************************************************************************************************

struct TestDescription
{
	ITestSuite* suite = nullptr;
	int testIndex = 0;
};

//************************************************************************************************
// TestCollectionRegistry
//************************************************************************************************

class TestCollectionRegistry: public Object,
							  public Singleton<TestCollectionRegistry>
{
public:
	void registerTestPlugIns ();
	void unregisterTestPlugIns ();

	const Vector<ITestSuite*>& getTestSuites () const;
	void collectTests (Vector<TestDescription>& result, StringRef filterExpression = "*") const;

protected:
	Vector<ITestCollection*> testCollections;
	Vector<ITestSuite*> testSuites;

	String toRegex (StringRef filterExpression) const;
	bool matches (StringRef suiteName, StringRef testName, StringRef filterExpression) const;
};

} // namespace CCL

#endif // _ccl_testcollectionregistry_h
