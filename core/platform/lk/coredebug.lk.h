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
// Filename    : core/platform/lk/coredebug.lk.h
// Description : Little Kernel Debug Primitives
//
//************************************************************************************************

#ifndef _coredebug_lk_h
#define _coredebug_lk_h

#include "core/platform/shared/coreplatformdebug.h"

namespace Core {
namespace Platform {
namespace Debug {

inline void print (CStringPtr string)
{
		printf (string);
}

} // namespace Debug
} // namespace Platform
} // namespace Core

#endif // _coredebug_lk_h
