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
// Filename    : core/public/coremath.h
// Description : Mathematical Primitives
//
//************************************************************************************************

#ifndef _coremath_h
#define _coremath_h

#include "core/public/coretypes.h"

#include <math.h>

namespace Core {

//************************************************************************************************
// static_power
/** Calculates base^exponent at compile time. */
//************************************************************************************************

template<int64 base, int exponent>
struct static_power
{
    static const int64 value = base * static_power<base, exponent-1>::value;
};

template<int64 base>
struct static_power<base, 0>
{
    static const int64 value = 1;
};

} // namespace Core

#endif // _coremath_h
