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
// Filename    : core/test/coretestcontext.h
// Description : Test base class
//
//************************************************************************************************

#ifndef _coretestcontext_h
#define _coretestcontext_h

#include "core/public/coretypes.h"

namespace Core {
namespace Test {

//************************************************************************************************
// ITestContext
//************************************************************************************************

struct ITestContext
{
	virtual ~ITestContext () {}

	virtual void addMessage (CStringPtr message, CStringPtr sourceFile, int lineNumber) = 0;
	virtual void addFailure (CStringPtr message, CStringPtr sourceFile, int lineNumber) = 0;
};

} // namespace Test
} // namespace Core

#endif // _coretestcontext_h
