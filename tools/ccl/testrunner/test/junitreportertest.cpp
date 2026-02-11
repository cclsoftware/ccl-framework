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
// Filename    : junitreportertest.cpp
// Description : Flexbox Unit Tests
//
//************************************************************************************************

#include "ccl/base/unittest.h"
#include "ccl/base/storage/xmltree.h"
#include "ccl/public/base/variant.h"

#include "junitreporter.h"

using namespace CCL;
using namespace JUnit;

//************************************************************************************************
// JUnitNodeModelTest
//************************************************************************************************

CCL_TEST (JUnitNodeModelTest, RootNodeHasExpectedTextAndAttribute)
{
	RootNode rootNode;
	rootNode.setTime (10.);

	StringDictionary attributes;
	rootNode.getAttributes (attributes);

	CCL_TEST_ASSERT_EQUAL (rootNode.getTag(), "testsuites");
	CCL_TEST_ASSERT_EQUAL (attributes.countEntries (), 1);

	CCL_TEST_ASSERT_EQUAL (attributes.getKeyAt (0), "time");
	CCL_TEST_ASSERT_EQUAL (attributes.getValueAt (0), "10.000000");
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_TEST (JUnitNodeModelTest, SuiteNodeHasExpectedTextAndAttributes)
{
	SuiteNode suiteNode;
	suiteNode.setTime (10.);
	suiteNode.setSuiteName ("ExampleSuite");

	StringDictionary attributes;
	suiteNode.getAttributes (attributes);

	CCL_TEST_ASSERT_EQUAL (suiteNode.getTag (), "testsuite");
	CCL_TEST_ASSERT_EQUAL (attributes.countEntries (), 2);

	CCL_TEST_ASSERT_EQUAL (attributes.getKeyAt (0), "name");
	CCL_TEST_ASSERT_EQUAL (attributes.getValueAt (0), "ExampleSuite");
	CCL_TEST_ASSERT_EQUAL (attributes.getKeyAt (1), "time");
	CCL_TEST_ASSERT_EQUAL (attributes.getValueAt (1), "10.000000");
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_TEST (JUnitNodeModelTest, TestNodeHasExpectedTextAndAttributes)
{
	TestCaseNode testNode;
	testNode.setTime (10.);
	testNode.setTestName ("ExampleTest");
	testNode.setClassName ("ExampleClass");

	StringDictionary attributes;
	testNode.getAttributes (attributes);

	CCL_TEST_ASSERT_EQUAL (testNode.getTag (), "testcase");
	CCL_TEST_ASSERT_EQUAL (attributes.countEntries (), 3);

	CCL_TEST_ASSERT_EQUAL (attributes.getKeyAt (0), "name");
	CCL_TEST_ASSERT_EQUAL (attributes.getValueAt (0), "ExampleTest");
	CCL_TEST_ASSERT_EQUAL (attributes.getKeyAt (1), "classname");
	CCL_TEST_ASSERT_EQUAL (attributes.getValueAt (1), "ExampleClass");
	CCL_TEST_ASSERT_EQUAL (attributes.getKeyAt (2), "time");
	CCL_TEST_ASSERT_EQUAL (attributes.getValueAt (2), "10.000000");
}

//************************************************************************************************
// JUnitReporterTest
//************************************************************************************************

class JUnitReporterTest: public Test
{
	// Test
	void setUp () override
	{
		reporter = NEW TestReporter ();
		reporter->setWriteToConsole (false);
		memoryStream = reporter->getMemoryStream ();
	}

protected:
	MemoryStream* memoryStream;
	XmlTreeParser xmlTreeParser;
	AutoPtr<TestReporter> reporter;
};

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_TEST_F (JUnitReporterTest, ReporterWithoutResultsHasAnEmptyRootNode)
{
	reporter->endTestRun ();

	xmlTreeParser.parse (*memoryStream);
	XmlNode* node = xmlTreeParser.getRoot ();

	CCL_TEST_ASSERT (node != nullptr);
	CCL_TEST_ASSERT_EQUAL (node->getName (), "testsuites");
	CCL_TEST_ASSERT_EQUAL (node->countChildren (), 0);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_TEST_F (JUnitReporterTest, ReporterWithAnAddedTestContainsATestsuiteAndATest)
{
	AutoPtr<TestResult> testResult = NEW TestResult;
	testResult->setTestName ("TestName");
	testResult->setSuiteName ("TestSuiteName");
	testResult->setDuration (2.);

	reporter->addResult (testResult);
	reporter->endTestRun ();

	xmlTreeParser.parse (*memoryStream);
	XmlNode* root = xmlTreeParser.getRoot ();

	CCL_TEST_ASSERT (root != nullptr);
	CCL_TEST_ASSERT_EQUAL (root->countChildren (), 1);

	XmlNode* suiteNode = unknown_cast<XmlNode> (root->getChild (0));
	CCL_TEST_ASSERT (suiteNode != nullptr);
	CCL_TEST_ASSERT_EQUAL (suiteNode->getName (), "testsuite");
	CCL_TEST_ASSERT_EQUAL (suiteNode->getAttribute ("name"), "TestSuiteName");

	CCL_TEST_ASSERT_EQUAL (suiteNode->countChildren (), 1);

	XmlNode* testNode = unknown_cast<XmlNode> (suiteNode->getChild (0));
	CCL_TEST_ASSERT (testNode != nullptr);
	CCL_TEST_ASSERT_EQUAL (testNode->getName (), "testcase");
	CCL_TEST_ASSERT_EQUAL (testNode->getAttribute ("name"), "TestName");
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_TEST_F (JUnitReporterTest, AccumulatedTimesAreParsedCorrectly)
{
	AutoPtr<TestResult> testResult = NEW TestResult;
	testResult->setTestName ("TestName");
	testResult->setSuiteName ("TestSuiteName");
	testResult->setDuration (2.);
	reporter->addResult (testResult);

	AutoPtr<TestResult> testResult2 = NEW TestResult;
	testResult2->setTestName ("TestName2");
	testResult2->setSuiteName ("TestSuiteName2");
	testResult2->setDuration (1.5);
	reporter->addResult (testResult2);

	reporter->endTestRun ();

	xmlTreeParser.parse (*memoryStream);
	XmlNode* root = xmlTreeParser.getRoot ();
	XmlNode* suiteNode1 = unknown_cast<XmlNode> (root->getChild (0));
	XmlNode* suiteNode2 = unknown_cast<XmlNode> (root->getChild (1));

	CCL_TEST_ASSERT_EQUAL (root->getAttribute ("time"), "3.500000");
	CCL_TEST_ASSERT_EQUAL (suiteNode1->getAttribute ("time"), "2.000000");
	CCL_TEST_ASSERT_EQUAL (suiteNode2->getAttribute ("time"), "1.500000");
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_TEST_F (JUnitReporterTest, FailuresAreLoggedCorrectly)
{
	AutoPtr<TestResult> testResult = NEW TestResult;
	testResult->setTestName ("TestName");
	testResult->setSuiteName ("TestSuiteName");
	testResult->setDuration (2.);
	testResult->addFailure ("Oh no", "failedfile.h", 1337);

	reporter->addResult (testResult);
	reporter->endTestRun ();

	xmlTreeParser.parse (*memoryStream);
	XmlNode* root = xmlTreeParser.getRoot ();
	XmlNode* suiteNode = unknown_cast<XmlNode> (root->getChild (0));
	XmlNode* testNode = unknown_cast<XmlNode> (suiteNode->getChild (0));

	CCL_TEST_ASSERT_EQUAL (testNode->getAttribute ("file"), "failedfile.h");
	CCL_TEST_ASSERT_EQUAL (testNode->getAttribute ("line"), "1337");

	CCL_TEST_ASSERT_EQUAL (testNode->countChildren (), 1);
	XmlNode* failureNode = unknown_cast<XmlNode> (testNode->getChild (0));
	CCL_TEST_ASSERT_EQUAL (failureNode->getAttribute ("message"), "Oh no");
	CCL_TEST_ASSERT_EQUAL (failureNode->getAttribute ("type"), "AssertionError");
}
