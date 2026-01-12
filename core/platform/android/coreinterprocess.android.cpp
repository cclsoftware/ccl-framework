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
// Filename    : core/platform/android/coreinterprocess.android.cpp
// Description : Android Interprocess Communication
//
//************************************************************************************************

#include "coreinterprocess.android.h"

namespace Core {
namespace Platform {

//////////////////////////////////////////////////////////////////////////////////////////////////
// Missing POSIX functions
//////////////////////////////////////////////////////////////////////////////////////////////////

int shm_open (const char *name, int oflag, mode_t mode)
{
	//TODO: implement me!
	return 0;
}

} // namespace Platform
} // namespace Core
