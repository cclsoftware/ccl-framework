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
// Filename    : ccl/platform/cocoa/system/nativefilesearcher.ios.mm
// Description : iOS File Searcher
//
//************************************************************************************************

#include "ccl/platform/cocoa/system/nativefilesystem.cocoa.h"

#include "ccl/system/nativefilesystem.h"

using namespace CCL;

//************************************************************************************************
// NativeFileSystem
//************************************************************************************************

ISearcher* CCL_API CocoaNativeFileSystem::createSearcher (ISearchDescription& description)
{
	// TODO
	return nullptr;
}
