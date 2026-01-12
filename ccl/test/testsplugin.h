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
// Filename    : ccl/test/testsplugin.h
// Description : Unit Tests Plugin Init
//
//************************************************************************************************

#ifndef _testsplugin_h
#define _testsplugin_h

namespace CCL {
class ClassFactory;

//************************************************************************************************
// TestsPlugin
//************************************************************************************************

namespace TestsPlugin 
{
void registerClasses (CCL::ClassFactory& factory);
}

} // namespace CCL

#endif // _testsplugin_h
