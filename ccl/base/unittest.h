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
// Filename    : ccl/base/unittest.h
// Description : Unit Test Suite
//
//************************************************************************************************

#ifndef _ccl_unittest_h
#define _ccl_unittest_h

#include "ccl/base/singleton.h"

#include "ccl/public/base/iunittest.h"
#include "ccl/public/collections/vector.h"
#include "ccl/public/text/cclstring.h"

namespace CCL {

//************************************************************************************************
// Test
//************************************************************************************************

class Test: public Object
{
public:
	Test ()
	: testContext (nullptr)
	{}
	
	PROPERTY_POINTER (ITestContext, testContext, TestContext)

	virtual StringRef getName () const = 0;
	virtual void testBody () = 0;
	
	virtual void setUp () {}
	virtual void tearDown () {}
};

//************************************************************************************************
// ParameterizedTest
//************************************************************************************************

template<typename ParameterType>
class ParameterizedTest: public Test
{
public:
	ParameterizedTest ()
	: index (0)
	{}
	
	void addTestValue (const ParameterType& parameter)
	{
		testValues.add (parameter);
	}
	
protected:
	const ParameterType& getTestValue () const
	{
		return testValues.at (index);
	}
	
	virtual void parameterizedTestBody () = 0;
	
private:
	Vector<ParameterType> testValues;
	int index;
	
	void testBody () override
	{
		while(index < testValues.count ())
		{
			setUp ();
			parameterizedTestBody ();
			tearDown ();
			index++;
		}
		
		index = 0;
	}
};

//************************************************************************************************
// TestSuite
//************************************************************************************************

class TestSuite: public Object,
				 public ITestSuite
{
public:
	DECLARE_CLASS (TestSuite, Object)

	TestSuite (StringRef name = nullptr);

	void addTest (Test* test);

	// ITestSuite
	StringRef CCL_API getName () const override;
	int CCL_API countTests () const override;
	StringRef CCL_API getTestName (int index) const override;

	tresult CCL_API setUp () override;
	tresult CCL_API tearDown () override;
	tresult CCL_API runTest (int index, ITestContext* context) override;

	CLASS_INTERFACE (ITestSuite, Object)

protected:
	Vector<AutoPtr<Test>> tests;
	String name;
};

//************************************************************************************************
// ITestFactory
//************************************************************************************************

interface ITestFactory: IUnknown
{
	virtual StringRef getSuiteName () const = 0;
	virtual Test* createTest () const = 0;
};

//************************************************************************************************
// TestRegistry
//************************************************************************************************

class TestRegistry: public Object,
					public Singleton<TestRegistry>
{
public:
	void registerTestFactory (ITestFactory* testFactory);
	void createTestSuites (Vector<AutoPtr<ITestSuite>>& testSuites) const;

private:
	Vector<ITestFactory*> testFactories;
};

//************************************************************************************************
// TestFactory
//************************************************************************************************

template <class TestType>
class TestFactory: public Object,
				   public ITestFactory
{
public:
	TestFactory (StringRef suiteName)
	: suiteName (suiteName)
	{
		TestRegistry::instance ().registerTestFactory (this);
	}

	// ITestFactory
	StringRef getSuiteName () const override
	{
		return suiteName;
	}

	Test* createTest () const override
	{
		return NEW TestType;
	}

	CLASS_INTERFACE (ITestFactory, Object)

private:
	String suiteName;
};

//************************************************************************************************
// TestCollection
//************************************************************************************************

class TestCollection: public Object,
					  public ITestCollection
{
public:
	void populateFrom (const TestRegistry& registry);

	// ITestCollection
	int CCL_API countSuites () const override;
	ITestSuite* CCL_API getSuite (int index) const override;

	CLASS_INTERFACE (ITestCollection, Object)

protected:
	Vector<AutoPtr<ITestSuite>> suites;
};

//************************************************************************************************
// Internal test macros
//************************************************************************************************

#define _CCL_REGISTER_TEST(TestClassName, SuiteName) 													\
	inline static TestFactory<TestClassName> factory { #SuiteName };

#define _CCL_TEST_CLASS_NAME(TestName, SuiteName) 														\
	SuiteName##_##TestName

#define _CCL_TEST_CLASS_NAME_SPECIALIZED(TestName, SuiteName, Type)										\
	SuiteName##_##TestName##_##Type

#define _CCL_TEST(SuiteName, TestName, Fixture) 														\
	class _CCL_TEST_CLASS_NAME (TestName, SuiteName): public Fixture									\
	{ 																									\
	public: 																							\
		StringRef getName () const override 															\
		{ 																								\
			return CCLSTR (#TestName); 																	\
		} 																								\
		void testBody () override;	 																	\
	private: 																							\
		_CCL_REGISTER_TEST (_CCL_TEST_CLASS_NAME (TestName, SuiteName), SuiteName) 						\
	}; 																									\
	void _CCL_TEST_CLASS_NAME (TestName, SuiteName)::testBody ()

//////////////////////////////////////////////////////////////////////////////////////////////////

#define _CCL_TEST_PARAMETERIZED(SuiteName, TestName, Fixture) 											\
	class _CCL_TEST_CLASS_NAME (TestName, SuiteName): public Fixture									\
	{ 																									\
	public: 																							\
		StringRef getName () const override 															\
		{ 																								\
			return CCLSTR (#TestName); 																	\
		} 																								\
		void parameterizedTestBody () override;	 														\
	private: 																							\
		_CCL_REGISTER_TEST (_CCL_TEST_CLASS_NAME (TestName, SuiteName), SuiteName) 						\
	}; 																									\
	void _CCL_TEST_CLASS_NAME (TestName, SuiteName)::parameterizedTestBody ()

//////////////////////////////////////////////////////////////////////////////////////////////////

#define _CCL_TEST_TEMPLATE(SuiteName, TestName, Fixture) 												\
	template <typename T>																				\
	class _CCL_TEST_CLASS_NAME (TestName, SuiteName): public Fixture<T>									\
	{ 																									\
	public:																								\
		using Fixture<T>::testContext; 																	\
		typedef T TypeParam;																			\
		StringRef getName () const override 															\
		{ 																								\
			return CCLSTR (#TestName); 																	\
		} 																								\
		void testBody () override;	 																	\
	}; 																									\
	template <typename T> void _CCL_TEST_CLASS_NAME (TestName, SuiteName)<T>::testBody ()

#define _CCL_TEST_TEMPLATE_SPECIALIZE(SuiteName, TestName, Type) 										\
	class _CCL_TEST_CLASS_NAME_SPECIALIZED (TestName, SuiteName, Type)									\
	: public _CCL_TEST_CLASS_NAME (TestName, SuiteName)<Type>											\
	{ 																									\
	private: 																							\
		_CCL_REGISTER_TEST (_CCL_TEST_CLASS_NAME_SPECIALIZED (TestName, SuiteName, Type), SuiteName) 	\
	};

//////////////////////////////////////////////////////////////////////////////////////////////////

#define _CCL_BEGIN_TEST_COLLECTION(CollectionName) 														\
	class CollectionName: public TestCollection 														\
	{ 																									\
	public: 																							\
		static IUnknown* createInstance (UIDRef cid, void* userData) 									\
		{ 																								\
			return static_cast<ITestCollection*> (NEW CollectionName); 									\
		} 																								\
																										\
		CollectionName () 																				\
		{

#define _CCL_END_TEST_COLLECTION() 																		\
		} 																								\
	};

//************************************************************************************************
// Public macros to create tests
//************************************************************************************************

/**
 * Create a test without using common initialization code in form of a test fixture.
 *
 * @arg SuiteName The name of the test suite to which the test will be added
 * @arg TestName The test name
 */
#define CCL_TEST(SuiteName, TestName) \
	_CCL_TEST (SuiteName, TestName, Test)

/**
 * Create a test based on a test fixture. A test fixture is a base class for a group of tests.
 * Note: The FixtureName will be used as the SuiteName, too.
 *
 * @arg FixtureName The name of a class inheriting from class Test which will be uses as a base class for this test
 * @arg TestName The test name
 */
#define CCL_TEST_F(FixtureName, TestName) \
	_CCL_TEST (FixtureName, TestName, FixtureName)

/**
 * Create a parameterized test based on a parameter fixture.
 * A parameterized test fixture must be provided.
 *
 * @arg FixtureName The name of the parameterized test fixture
 * @arg TestName The test name
 */
#define CCL_TEST_P(FixtureName, TestName) \
	_CCL_TEST_PARAMETERIZED (FixtureName, TestName, FixtureName)

/**
 * Create a templated test. A templated test fixture must be provided.
 * TypeParam can be used inside the test body as the template type.
 *
 * Note: It is necessary to call CCL_TEST_T_ADD in order to add the actual tests according to the types you want to test.
 * Note: The FixtureName will be used as the SuiteName, too.
 *
 * @arg FixtureName The name of a template class inheriting from class Test which will be uses as a base class for this test
 * @arg TestName The test name
 */
#define CCL_TEST_T(FixtureName, TestName) \
	_CCL_TEST_TEMPLATE (FixtureName, TestName, FixtureName)

/**
 * Adds the typed specialization of a templated test and adds it to the test registry
 *
 * @arg SuiteName The test suite name
 * @arg TestName The test name
 * @arg Type The type as referenced by TypeParam of a templated test
 */
#define CCL_TEST_T_ADD(SuiteName, TestName, Type) \
	_CCL_TEST_TEMPLATE_SPECIALIZE (SuiteName, TestName, Type)

//************************************************************************************************
// Public macros to add tests to a collection
//************************************************************************************************

/**
 * Creates a TestCollection class which is populated with all registered test suites on instantiation
 * @arg CollectionName The name of the collection
 */
#define CCL_ADD_TEST_COLLECTION(CollectionName) 								\
	_CCL_BEGIN_TEST_COLLECTION (CollectionName)									\
		auto& registry = TestRegistry::instance ();								\
		populateFrom (registry); 												\
	_CCL_END_TEST_COLLECTION ()

/**
 * Creates a custom TestCollection class managing a single test suite.
 * @arg CollectionName The name of the collection
 *
 */
#define CCL_ADD_CUSTOM_TEST_COLLECTION(CollectionName, SuiteName)				\
	_CCL_BEGIN_TEST_COLLECTION (CollectionName)									\
		suites.add (NEW SuiteName ()); 											\
	_CCL_END_TEST_COLLECTION ()

/**
 * Registers a test collection with a class factory
 * @arg classFactory The class factory for which the test collection is registered
 * @arg uid A unique identifier
 * @arg CollectionName The name of the collection
 */
#define CCL_REGISTER_TEST_COLLECTION(classFactory, uid, CollectionName) 			\
	{ 																				\
		ClassDesc __testClass (uid, PLUG_CATEGORY_UNITTEST, #CollectionName); 		\
		classFactory->registerClass (__testClass, CollectionName::createInstance); 	\
	}

//************************************************************************************************
// Internal Test Assertion Macros
//************************************************************************************************

#define _CCL_TEST_ASSERTION_EXPRESSION(name, arguments) \
	String () << CCLSTR (#name) << " (" << CCLSTR (arguments) << ")"

#define _CCL_TEST_ASSERT_INTERNAL(condition, name, arguments) \
	if(condition) \
		(testContext->addPass (_CCL_TEST_ASSERTION_EXPRESSION (name, arguments), CCL::String (__FILE__), __LINE__)); \
	else \
		(testContext->addFailure (_CCL_TEST_ASSERTION_EXPRESSION (name, arguments), CCL::String (__FILE__), __LINE__))

//************************************************************************************************
// Test Assertion Macros
//************************************************************************************************

#define CCL_TEST_ASSERT(condition) \
	_CCL_TEST_ASSERT_INTERNAL (condition, CCL_TEST_ASSERT, #condition)

#define CCL_TEST_ASSERT_FALSE(condition) \
	_CCL_TEST_ASSERT_INTERNAL (!(condition), CCL_TEST_ASSERT_FALSE, #condition)

#define CCL_TEST_ASSERT_EQUAL(expected, actual) \
	_CCL_TEST_ASSERT_INTERNAL (((expected) == (actual)), CCL_TEST_ASSERT_EQUAL, #expected ", " #actual)

#define CCL_TEST_ASSERT_NOT_EQUAL(expected, actual) \
	_CCL_TEST_ASSERT_INTERNAL (((expected) != (actual)), CCL_TEST_ASSERT_NOT_EQUAL, #expected ", " #actual)

#define CCL_TEST_ASSERT_NEAR(expected, actual, delta) \
	_CCL_TEST_ASSERT_INTERNAL ((actual > (expected - delta) && actual < (expected + delta)), CCL_TEST_ASSERT_NEAR, #expected ", " #actual ", " #delta)

} // namespace CCL

#endif //_ccl_unittest_h
