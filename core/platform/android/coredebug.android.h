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
// Filename    : core/platform/android/coredebug.android.h
// Description : Debugging Functions Android implementation
//
//************************************************************************************************

#ifndef _coredebug_android_h
#define _coredebug_android_h

#include "core/platform/shared/coreplatformdebug.h"

#include "core/public/corestringbuffer.h"
#include "core/public/coreversion.h"

#include <android/log.h>

namespace Core {
namespace Platform {

//************************************************************************************************
// Debugging Functions
//************************************************************************************************

inline void Debug::print (CStringPtr string)
{
	if(string && string[0] && !(string[0] == '\n' && string[1] == 0))
	{
		CString256 str (string);
		str.replace('\n', ' '); // remove newlines
		__android_log_write (ANDROID_LOG_DEBUG, "CCL Native", str);
	}
}

} // namespace Platform
} // namespace Core

#endif // _coredebug_android_h
