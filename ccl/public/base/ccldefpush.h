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
// Filename    : ccl/public/base/ccldefpush.h
// Description : Push CCL definitions
//
//************************************************************************************************

// This header file does not contain the usual include guard, as it might be needed several times per translation unit.
// Balance with ccldefpop.h when using in a header file

#include "core/public/coredefpush.h"

#pragma push_macro ("interface")
#undef interface

#if CCL_PLATFORM_MAC || CCL_PLATFORM_IOS
#define CreateThreadPool _CreateThreadPool
#define ThreadID _ThreadID
#endif
