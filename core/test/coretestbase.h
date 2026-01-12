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
	virtual bool run (ITestContext& testContext);
};

#if CORE_TEST_REGISTRY_ENABLED
	#define CORE_REGISTER_TEST(T) DEFINE_INITIALIZER (Register##T) { static T theTest; TestRegistry::instance ().addTest (&theTest); }
#else
	#define CORE_REGISTER_TEST(T)
#endif
#define CORE_TEST_MESSAGE(s) testContext.addMessage (s, __FILE__, __LINE__);
#define CORE_TEST_FAILED(s) testContext.addFailure (s, __FILE__, __LINE__);

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

} // namespace Test
} // namespace Core

#endif // _coretestbase_h
