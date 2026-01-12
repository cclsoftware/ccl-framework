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
// Filename    : ccl/platform/cocoa/system/cfallocator.h
// Description : Mac allocator class
//
//************************************************************************************************

#ifndef _ccl_cfallocator_h
#define _ccl_cfallocator_h

#include <CoreFoundation/CFBase.h>

namespace CCL {
namespace MacOS {

//************************************************************************************************
// CFAlloctor
//************************************************************************************************

CFAllocatorRef GetAllocator ();
	
} // namespave MacOS
} // namespace CCL

#endif // _ccl_cfallocator_h
