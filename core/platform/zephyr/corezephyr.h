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
// Filename    : core/platform/zephyr/corezephyr.h
// Description : Zephyr system includes
//
//************************************************************************************************

#ifndef _core_zephyr_h
#define _core_zephyr_h

#ifdef ZEPHYR_INCLUDE_ZEPHYR_H_
#error "include this file instead of <zephyr.h>"
#endif

//////////////////////////////////////////////////////////////////////////////////////////////////
// Preserve Core macros before including Zephyr headers
//////////////////////////////////////////////////////////////////////////////////////////////////

#ifdef INLINE
#define INLINE_DEFINED 1
#pragma push_macro ("INLINE")
#undef INLINE
#endif

#ifdef STRINGIFY
#define STRINGIFY_DEFINED 1
#pragma push_macro ("STRINGIFY")
#undef STRINGIFY
#endif

#include <zephyr/kernel.h>
#include <version.h>
#include <zephyr/logging/log.h>
#include <zephyr/logging/log_instance.h>

#ifdef STRINGIFY_DEFINED
#pragma pop_macro ("STRINGIFY")
#endif
#ifdef INLINE_DEFINED
#pragma pop_macro ("INLINE")
#endif

//////////////////////////////////////////////////////////////////////////////////////////////////
// Check for kernel version
//////////////////////////////////////////////////////////////////////////////////////////////////

#if ((KERNELVERSION <  0x2026300) || (KERNELVERSION > 0x3026300))
  #warning "Untested zephyr kernel version"
#endif 

#endif // _core_zephyr_h
 
