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
// Filename    : core/system/coredebug.h
// Description : Debugging Functions
//
//************************************************************************************************

#ifndef _coredebug_h
#define _coredebug_h

#include "core/public/coretypes.h"

namespace Core {

//////////////////////////////////////////////////////////////////////////////////////////////////
// Debug logging macros
//
// CORE_PRINT macros can be enabled (for a debug build) by defining DEBUG_LOG as 1
//////////////////////////////////////////////// //////////////////////////////////////////////////

#if DEBUG
	
#if !defined (DEBUG_LOG)
#define DEBUG_LOG 0
#endif
	
#else // RELEASE
	
#undef DEBUG_LOG
#define DEBUG_LOG 0
	
#endif
	
#if DSP_TI32
	// The DSP_TI32 compiler doesn't support variable args...
	// So, the PRINT macros are undefined -- but you shouldn't be using them from DSP, anyway.
#else
	#if DEBUG_LOG
		#define CORE_INDENT(count)	for(int __indent = 0; __indent < (count); __indent++) Core::DebugPrint ("\t");
		#define CORE_PRINT(s)		Core::DebugPrint (s);
		#define CORE_PRINTF(s, ...)	Core::DebugPrintf (s, __VA_ARGS__);
	#else
		#define CORE_INDENT(count)	{}
		#define CORE_PRINT(s)		{}
		#define CORE_PRINTF(s, ...) {}
	#endif
#endif // DSP_TI32

//************************************************************************************************
// Debugging Functions
//************************************************************************************************

void DebugPrint (CStringPtr string);
void DebugPrintf (CStringPtr format, ...);

} // namespace Core

#endif // _coredebug_h
