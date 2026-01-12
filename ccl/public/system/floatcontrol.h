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
// Filename    : ccl/public/system/floatcontrol.h
// Description : Floating point control register access
//
//************************************************************************************************

#ifndef _ccl_floatcontrol_h
#define _ccl_floatcontrol_h

#include "ccl/public/base/platform.h"

#if defined(CCL_PLATFORM_INTEL)
#include <xmmintrin.h>
#endif

namespace CCL {

//////////////////////////////////////////////////////////////////////////////////////////////////
// Intel platforms
//////////////////////////////////////////////////////////////////////////////////////////////////

#if defined(CCL_PLATFORM_INTEL)

inline void setFloatEnv ()
{
	unsigned oldMXCSR = _mm_getcsr ();
	// enable FTZ (15), underflow mask (11), denormal mask (8), DAZ (6)
	unsigned newMXCSR = oldMXCSR | (1 << 15) | (1 << 11) | (1 << 8) | (1 << 6); 
	_mm_setcsr (newMXCSR);
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// Other platforms, not implemented
//////////////////////////////////////////////////////////////////////////////////////////////////

#else

inline void setFloatEnv () {}

#endif

} // namespace CCL

#endif // _ccl_floatcontrol_h
