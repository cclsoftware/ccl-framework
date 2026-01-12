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
// Filename    : core/platform/win/coredebug.win.h
// Description : Debugging Functions Windows implementation
//
//************************************************************************************************

#ifndef _coredebug_win_h
#define _coredebug_win_h

#include "core/platform/shared/coreplatformdebug.h"

#include <windows.h>

namespace Core {
namespace Platform {

//************************************************************************************************
// Debugging Functions
//************************************************************************************************

inline void Debug::print (CStringPtr string)
{
	::OutputDebugStringA (string);
}

} // namespace Platform
} // namespace Core

#endif // _coredebug_win_h
