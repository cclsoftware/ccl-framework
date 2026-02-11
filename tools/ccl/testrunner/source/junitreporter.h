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
// Filename    : junitreporter.h
// Description : JUnit Test Reporter
//
//************************************************************************************************

#ifndef _ccl_junitreporter_h
#define _ccl_junitreporter_h

#include "ccl/base/objectnode.h"
#include "ccl/base/collections/stringdictionary.h"
#include "ccl/extras/tools/testrunner.h"

#include "ccl/public/text/ixmlwriter.h"
#include "ccl/public/base/memorystream.h"

namespace CCL {
namespace JUnit {

//************************************************************************************************
// JUnit::ModelNode
//************************************************************************************************

class ModelNode: public ObjectNode
{
public:
	DECLARE_CLASS_ABSTRACT (ModelNode, ObjectNode)

	static StringRef kAttributeTime;
	static StringRef kAttributeName;
	static StringRef kAttributeClassName;
	static StringRef kAttributeFile;
	static StringRef kAttributeLine;
	static StringRef kAttributeMessage;
	static StringRef kAttributeType;

	virtual StringRef getTag () const = 0;
	virtual void getAttributes (StringDictionary& a) const = 0;
};

//************************************************************************************************
// JUnit::TimedNode
//************************************************************************************************

class TimedNode: public ModelNode
{
public:
	DECLARE_CLASS_ABSTRACT (TimedNode, ModelNode)

	TimedNode ();

	PROPERTY_VARIABLE (double, time, Time)

	void incrementTime (double delta);

	// ModelNode
	void getAttributes (StringDictionary& a) const override;

private:
	static const int kNumDecimals;
};

//************************************************************************************************
// JUnit::RootNode
//************************************************************************************************

class RootNode: public TimedNode
{
public:
	DECLARE_CLASS (RootNode, TimedNode)

	// TimedNode
	StringRef getTag () const override;

private:
	static StringRef kTag;
};

//************************************************************************************************
// JUnit::SuiteNode
//************************************************************************************************

class SuiteNode: public TimedNode
{
public:
	DECLARE_CLASS (SuiteNode, TimedNode)

	PROPERTY_STRING (suiteName, SuiteName)

	// TimedNode
	StringRef getTag () const override;
	void getAttributes (StringDictionary& a) const override;

private:
	static StringRef kTag;
};

//************************************************************************************************
// JUnit::TestCaseNode
//************************************************************************************************

class TestCaseNode: public TimedNode
{
public:
	DECLARE_CLASS (TestCaseNode, TimedNode)

	TestCaseNode ();

	PROPERTY_STRING (testName, TestName)
	PROPERTY_STRING (className, ClassName)
	PROPERTY_STRING (file, File)
	PROPERTY_VARIABLE (int, line, Line)

	// SuiteNode
	StringRef getTag () const override;
	void getAttributes (StringDictionary& a) const override;

private:
	static StringRef kTag;
};

//************************************************************************************************
// JUnit::FailureNode
//************************************************************************************************

class FailureNode: public ModelNode
{
public:
	DECLARE_CLASS (FailureNode, ModelNode)

	static StringRef kFailureTypeAssertionError;

	PROPERTY_STRING (message, Message)
	PROPERTY_STRING (type, Type)

	// SuiteNode
	StringRef getTag () const override;
	void getAttributes (StringDictionary& a) const override;

private:
	static StringRef kTag;
};

//************************************************************************************************
// JUnit::TestReporter
//************************************************************************************************

class TestReporter:	public Object,
					public ITestReporter
{
public:
	TestReporter ();

	PROPERTY_AUTO_POINTER (RootNode, root, Root)
	PROPERTY_AUTO_POINTER (MemoryStream, memoryStream, MemoryStream)
	PROPERTY_BOOL (writeToConsole, WriteToConsole)

	// ITestReporter
	void beginTestRun (int numTests, StringRef filter) override {};
	void endTestRun () override;
	void addResult (TestResult* testResult) override;
	bool allTestsPassed () const override;

	CLASS_INTERFACE (ITestReporter, Object)

private:
	static const CCL::Text::Encoding kEncoding;
	AutoPtr<IXmlWriter> writer;
	bool hasFailedTests;

	TestCaseNode* createTestNode (TestResult* result);
	void write (const ModelNode& node);
};

} // JUnit
} // CCL

#endif
