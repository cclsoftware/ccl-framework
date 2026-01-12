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
// Filename    : ccl/platform/win/text/cclmalloc.win.cpp
// Description : Memory Allocator Debug Info
//
//************************************************************************************************

#include "core/platform/win/coremalloc.win.h"

const char* ccl_get_debug_filename (const char* filename)
{
	return core_get_debug_filename (filename);
}
