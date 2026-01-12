//************************************************************************************************
//
// JavaScript Engine
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
// Filename    : jstest.h
// Description : Test class
//
//************************************************************************************************

#ifndef _jstest_h
#define _jstest_h

#include "ccl/base/object.h"

namespace JScript {

class Engine;

//************************************************************************************************
// TestClass
//************************************************************************************************

class TestClass: public CCL::Object
{
public:
	DECLARE_CLASS (TestClass, Object)
	DECLARE_METHOD_NAMES (TestClass)

	TestClass ();
	~TestClass ();

	static bool runTest (Engine* engine);

	// IObject
	CCL::tbool CCL_API getProperty (CCL::Variant& var, MemberID propertyId) const override;
	CCL::tbool CCL_API setProperty (MemberID propertyId, const CCL::Variant& var) override;
	CCL::tbool CCL_API invokeMethod (CCL::Variant& returnValue, CCL::MessageRef msg) override;

protected:
	int width;
	TestClass* child;
};

} // namespace JScript

#endif // _jstest_h
