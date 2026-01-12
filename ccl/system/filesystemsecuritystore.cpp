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
// Filename    : ccl/system/filesystemsecuritystore.cpp
// Description : File system security store base class
//
//************************************************************************************************

#include "ccl/system/filesystemsecuritystore.h"

#include "ccl/public/systemservices.h"

using namespace CCL;

//************************************************************************************************
// FileSystemSecurityStore
//************************************************************************************************

#if !(CCL_PLATFORM_MAC || CCL_PLATFORM_IOS)
DEFINE_EXTERNAL_SINGLETON (FileSystemSecurityStore, FileSystemSecurityStore)
#endif

//////////////////////////////////////////////////////////////////////////////////////////////////
// System Services API
//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_EXPORT IFileSystemSecurityStore& CCL_API System::CCL_ISOLATED (GetFileSystemSecurityStore) ()
{
	return FileSystemSecurityStore::instance ();
}

