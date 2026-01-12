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
// Filename    : ccl/public/base/iunittest.h
// Description : Unit Test Interfaces
//
//************************************************************************************************

#ifndef _ccl_iunittest_h
#define _ccl_iunittest_h

#include "ccl/public/base/iunknown.h"

namespace CCL {

interface ITestSuite;
interface IProgressNotify;

//////////////////////////////////////////////////////////////////////////////////////////////////
// Component Categories
//////////////////////////////////////////////////////////////////////////////////////////////////

/** Class category for test plug-ins. */
#define PLUG_CATEGORY_UNITTEST	CCLSTR ("UnitTest")

//************************************************************************************************
// IAssertionResult
//************************************************************************************************

interface IAssertionResult: IUnknown
{
	virtual tresult CCL_API addMessage (StringRef message) = 0;

	DECLARE_IID (IAssertionResult)

	//////////////////////////////////////////////////////////////////////////////////////////////////

	tresult operator << (StringRef message) { return addMessage (message); }
};

DEFINE_IID (IAssertionResult, 0x36475B4B, 0x1AB0, 0x3242, 0xBF, 0x41, 0x85, 0x43, 0xA8, 0x4D, 0x89, 0x8B)

//************************************************************************************************
// ITestContext
//************************************************************************************************

interface ITestContext: IUnknown
{
	virtual IAssertionResult& CCL_API addPass (StringRef expression, StringRef fileName, int lineNumber) = 0;

	virtual IAssertionResult& CCL_API addFailure (StringRef expression, StringRef fileName, int lineNumber) = 0;

	DECLARE_IID (ITestContext)
};

DEFINE_IID (ITestContext, 0xfcd883b5, 0xfcb8, 0x4296, 0x88, 0x2d, 0x43, 0x3f, 0xd7, 0x90, 0xc8, 0x78)

//************************************************************************************************
// ITestSuite
//************************************************************************************************

interface ITestSuite: IUnknown
{
	virtual StringRef CCL_API getName () const = 0;
	
	virtual int CCL_API countTests () const = 0;
	
	virtual StringRef CCL_API getTestName (int index) const = 0;

	virtual tresult CCL_API setUp () = 0;
	
	virtual tresult CCL_API tearDown () = 0;
	
	virtual tresult CCL_API runTest (int index, ITestContext* context) = 0;

	DECLARE_IID (ITestSuite)
};

DEFINE_IID (ITestSuite, 0x21a3905a, 0x6e9c, 0x4f96, 0xb1, 0xfc, 0xa4, 0x58, 0xb7, 0x10, 0xd9, 0xfa)

//************************************************************************************************
// ITestCollection
//************************************************************************************************

interface ITestCollection: IUnknown
{
	virtual int CCL_API countSuites () const = 0;

	virtual ITestSuite* CCL_API getSuite (int index) const = 0;

	DECLARE_IID (ITestCollection)
};

DEFINE_IID (ITestCollection, 0x79d1e3d1, 0x26ce, 0x40e3, 0x87, 0x5c, 0xc5, 0xcc, 0xbe, 0x23, 0x82, 0x13)

} // namespace CCL

#endif // _ccl_iunittest_h
