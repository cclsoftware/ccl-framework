//************************************************************************************************
//
// CCL Test Runner
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
// Filename    : junitreporter.cpp
// Description : JUnit Test Reporter
//
//************************************************************************************************

#include "junitreporter.h"

#include "ccl/public/system/iconsole.h"
#include "ccl/public/systemservices.h"

using namespace CCL;
using namespace JUnit;

//************************************************************************************************
// JUnit::ModelNode
//************************************************************************************************

DEFINE_CLASS_ABSTRACT (ModelNode, ObjectNode)

//////////////////////////////////////////////////////////////////////////////////////////////////

StringRef ModelNode::kAttributeTime ("time");
StringRef ModelNode::kAttributeName ("name");
StringRef ModelNode::kAttributeClassName ("classname");
StringRef ModelNode::kAttributeFile ("file");
StringRef ModelNode::kAttributeLine ("line");
StringRef ModelNode::kAttributeMessage ("message");
StringRef ModelNode::kAttributeType ("type");

//************************************************************************************************
// JUnit::TimedNode
//************************************************************************************************

DEFINE_CLASS_ABSTRACT (TimedNode, ModelNode)

//////////////////////////////////////////////////////////////////////////////////////////////////

const int TimedNode::kNumDecimals = 6;

//////////////////////////////////////////////////////////////////////////////////////////////////

TimedNode::TimedNode ()
: time (0.)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TimedNode::incrementTime (double delta)
{
	time += delta;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TimedNode::getAttributes (StringDictionary& a) const
{
	a.setEntry (kAttributeTime, String ().appendFloatValue (time, kNumDecimals));
}

//************************************************************************************************
// JUnit::RootNode
//************************************************************************************************

DEFINE_CLASS (RootNode, TimedNode)

//////////////////////////////////////////////////////////////////////////////////////////////////

StringRef RootNode::kTag ("testsuites");

//////////////////////////////////////////////////////////////////////////////////////////////////

StringRef RootNode::getTag () const
{
	return kTag;
}

//************************************************************************************************
// JUnit::SuiteNode
//************************************************************************************************

DEFINE_CLASS (SuiteNode, ModelNode)

//////////////////////////////////////////////////////////////////////////////////////////////////

StringRef SuiteNode::kTag ("testsuite");

//////////////////////////////////////////////////////////////////////////////////////////////////

StringRef SuiteNode::getTag () const
{
	return kTag;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SuiteNode::getAttributes (StringDictionary& a) const
{
	a.setEntry (kAttributeName, suiteName);
	SuperClass::getAttributes (a);
}

//************************************************************************************************
// JUnit::TestCaseNode
//************************************************************************************************

DEFINE_CLASS (TestCaseNode, ModelNode)

//////////////////////////////////////////////////////////////////////////////////////////////////

StringRef TestCaseNode::kTag ("testcase");

//////////////////////////////////////////////////////////////////////////////////////////////////

TestCaseNode::TestCaseNode ()
: line (0)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringRef TestCaseNode::getTag () const
{
	return kTag;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TestCaseNode::getAttributes (StringDictionary& a) const
{
	a.setEntry (kAttributeName, testName);
	a.setEntry (kAttributeClassName, className);
	SuperClass::getAttributes (a);

	if(!file.isEmpty ())
		a.setEntry (kAttributeFile, file);

	if(line != 0)
		a.setEntry (kAttributeLine, String ().appendIntValue (line));
}

//************************************************************************************************
// JUnit::FailureNode
//************************************************************************************************

DEFINE_CLASS (FailureNode, ModelNode)

//////////////////////////////////////////////////////////////////////////////////////////////////

StringRef FailureNode::kFailureTypeAssertionError ("AssertionError");
StringRef FailureNode::kTag ("failure");

//////////////////////////////////////////////////////////////////////////////////////////////////

StringRef FailureNode::getTag () const
{
	return kTag;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void FailureNode::getAttributes (StringDictionary& a) const
{
	a.setEntry (kAttributeMessage, message);
	a.setEntry (kAttributeType, type);
}

//************************************************************************************************
// JUnit::TestReporter
//************************************************************************************************

const Text::Encoding TestReporter::kEncoding = Text::kUTF8;

//////////////////////////////////////////////////////////////////////////////////////////////////

TestReporter::TestReporter ()
: root (NEW RootNode ()),
  memoryStream (NEW MemoryStream ()),
  writeToConsole (true),
  writer (System::CreateXmlWriter ()),
  hasFailedTests (false)
{
	writer->setShouldIndent (true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TestReporter::endTestRun ()
{
	if(!memoryStream || !root)
		return;

	if(writer->beginDocument (*memoryStream.as_plain (), kEncoding) != kResultOk)
		return;

	write (*root.as_plain ());
	writer->endDocument ();
	memoryStream->rewind ();

	if(isWriteToConsole ())
	{
		void* xmlStreamStart = memoryStream->getMemoryAddress ();
		String lines (kEncoding, static_cast<CStringPtr> (xmlStreamStart));
		System::GetConsole ().writeLine (lines);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TestReporter::addResult (TestResult* testResult)
{
	if(!testResult)
		return;

	if(!root)
		return;

	TestCaseNode* testNode = createTestNode (testResult);
	root->incrementTime (testNode->getTime ());

	String suiteName = testResult->getSuiteName ();

	bool testAddedToExistingSuite = false;
	root->visitChildren ([&] (ObjectNode* child) -> bool
	{
		if(auto* suiteNode = ccl_cast<SuiteNode> (child))
		{
			StringDictionary attributes;
			suiteNode->getAttributes (attributes);
			String existingSuiteName = attributes.lookupValue (ModelNode::kAttributeName);

			if(existingSuiteName == suiteName)
			{
				suiteNode->incrementTime (testNode->getTime ());
				suiteNode->addChild (testNode);
				testAddedToExistingSuite = true;
			}
		}
		return true;
	}, false);

	if(!testAddedToExistingSuite)
	{
		SuiteNode* suiteNode = NEW SuiteNode;
		suiteNode->setSuiteName (suiteName);
		suiteNode->setTime (testNode->getTime ());
		suiteNode->addChild (testNode);
		root->addChild (suiteNode);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool TestReporter::allTestsPassed () const
{
	return !hasFailedTests;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

TestCaseNode* TestReporter::createTestNode (TestResult* result)
{
	TestCaseNode* testNode = NEW TestCaseNode;
	testNode->setTestName (result->getTestName ());
	testNode->setClassName (result->getSuiteName ());
	testNode->setTime (result->getDuration ());

	if(result->hasFailed ())
	{
		hasFailedTests = true;

		const Vector<AutoPtr<AssertionResult>>& assertions = result->getAssertionResults ();
		if(assertions.count () > 0)
		{
			const AssertionInfo& info = assertions.at (0)->getInfo ();
			testNode->setFile (info.fileName);
			testNode->setLine (info.lineNumber);

			auto* failureNode = NEW FailureNode ();
			String message = info.expression;
			if(!info.message.isEmpty ())
				message << ": " << info.message;

			failureNode->setMessage (message);
			failureNode->setType (FailureNode::kFailureTypeAssertionError);

			testNode->addChild (failureNode);
		}
	}

	return testNode;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TestReporter::write (const ModelNode& node)
{
	StringDictionary attributes;
	node.getAttributes (attributes);

	if(node.countChildren () == 0)
	{
		tresult result = writer->writeElement (node.getTag (), &attributes);
		ASSERT (result == kResultOk)
		return;
	}

	writer->startElement (node.getTag (), &attributes);
	node.visitChildren ([this] (ObjectNode* child) -> bool
	{
		if(auto* childNode = ccl_cast<ModelNode> (child))
			write (*childNode);
		return true;
	}, false);
	writer->endElement (node.getTag ());
}
