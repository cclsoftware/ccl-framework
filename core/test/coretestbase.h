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
// Filename    : core/test/coretestbase.h
// Description : Test base class
//
//************************************************************************************************

#ifndef _coretestbase_h
#define _coretestbase_h

#include "coretestcontext.h"

#include "core/portable/coresingleton.h"

#include "core/public/corevector.h"
#include "core/public/corebasicmacros.h"

#ifndef CORE_TEST_REGISTRY_ENABLED
#define CORE_TEST_REGISTRY_ENABLED 1
#endif

namespace Core {
namespace Test {

//************************************************************************************************
// TestBase
//************************************************************************************************

class TestBase
{
public:
	virtual ~TestBase () {}

	virtual CStringPtr getName () const;
	virtual bool run (ITestContext& testContext); // default implementation runs setUp, testBody, tearDown

	virtual void testBody (ITestContext& context) {}

	virtual void setUp (ITestContext& context) {}

	virtual void tearDown (ITestContext& context) {}
};

//************************************************************************************************
// TestRegistry
//************************************************************************************************

class TestRegistry: public Portable::StaticSingleton<TestRegistry>
{
public:
	void runAllTests (ITestContext& testContext);
	const Vector<TestBase*>& getTests () const;
	void addTest (TestBase* test);

private:
	Vector<TestBase*> tests;
};

//************************************************************************************************
// Internal Test Assertion Macros
//************************************************************************************************

#define _CORE_TEST_ASSERTION_EXPRESSION(name, arguments) \
	Core::CString256 ().append (#name).append (" ").append (#arguments)

#define _CORE_TEST_ASSERT_INTERNAL(condition, name, arguments) \
	if(condition) \
		(testContext.addMessage (_CORE_TEST_ASSERTION_EXPRESSION (name, arguments), __FILE__, __LINE__)); \
	else \
		(testContext.addFailure (_CORE_TEST_ASSERTION_EXPRESSION (name, arguments), __FILE__, __LINE__));

//************************************************************************************************
// Internal test macros
//************************************************************************************************

#define _CORE_TEST_CLASS_NAME(TestName, SuiteName) \
	SuiteName##_##TestName

#define _CORE_TEST_REGISTER_TEST(NAME) CORE_REGISTER_TEST (NAME)

#define _CORE_TEST(SuiteName, TestName, Fixture) \
	class _CORE_TEST_CLASS_NAME (TestName, SuiteName) \
	: public Fixture \
	{ \
	public: \
		CStringPtr getName () const override \
		{ \
			return STRINGIFY (_CORE_TEST_CLASS_NAME (TestName, SuiteName)); \
		} \
		void testBody (ITestContext& testContext) override; \
	}; \
	_CORE_TEST_REGISTER_TEST (_CORE_TEST_CLASS_NAME (TestName, SuiteName)) \
	void _CORE_TEST_CLASS_NAME (TestName, SuiteName)::testBody (ITestContext& testContext)

//************************************************************************************************
// Public macros to create tests
//************************************************************************************************

#if CORE_TEST_REGISTRY_ENABLED

	#define CORE_REGISTER_TEST(T) \
		DEFINE_INITIALIZER (Register##T) \
		{ \
			static T theTest; \
			TestRegistry::instance ().addTest (&theTest); \
		}

	// Create a test without using common initialization code
	#define CORE_TEST(TestName) \
		_CORE_TEST (CoreTest, TestName, Core::Test::TestBase)

	// Create a test based on a test fixture. A test fixture is a base class for a group of tests.
	#define CORE_TEST_F(FixtureName, TestName) \
		_CORE_TEST (FixtureName, TestName, FixtureName)

#else
	#define CORE_REGISTER_TEST(T)
	#define CORE_TEST(SuiteName, TestName)
	#define CORE_TEST_F(FixtureName, TestName)
#endif

//************************************************************************************************
// Test Assertion Macros
//************************************************************************************************

#define CORE_TEST_ASSERT(condition) \
	_CORE_TEST_ASSERT_INTERNAL (condition, CORE_TEST_ASSERT, #condition)

#define CORE_TEST_ASSERT_FALSE(condition) \
	_CORE_TEST_ASSERT_INTERNAL (!(condition), CORE_TEST_ASSERT_FALSE, #condition)

#define CORE_TEST_MESSAGE(s) testContext.addMessage (s, __FILE__, __LINE__);
#define CORE_TEST_FAILED(s)	testContext.addFailure (s, __FILE__, __LINE__);

} // namespace Test
} // namespace Core

#endif // _coretestbase_h
