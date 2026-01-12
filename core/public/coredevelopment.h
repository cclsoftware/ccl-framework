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
// Filename    : core/public/coredevelopment.h
// Description : Determine locations in working copy
//
//************************************************************************************************

#ifndef _coredevelopment_h
#define _coredevelopment_h

#include "core/public/coreplatform.h"

//////////////////////////////////////////////////////////////////////////////////////////////////
// Build folder
//////////////////////////////////////////////////////////////////////////////////////////////////

#define BUILD_FOLDER_NAME "build"

// CORE_BUILD_PATH is defined by CMake, see coremacros.cmake
#ifdef CORE_BUILD_PATH
	#define RELATIVE_BUILD_PATH CORE_BUILD_PATH
#else
	#error Core build path not defined
#endif

#endif // _coredevelopment_h
