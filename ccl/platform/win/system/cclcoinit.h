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
// Filename    : ccl/platform/win/system/cclcoinit.h
// Description : COM/WinRT Initialization
//
//************************************************************************************************

#ifndef _ccl_cclcoinit_h
#define _ccl_cclcoinit_h

#include "ccl/public/base/iunknown.h"

namespace CCL {
namespace System {

/** COM/WinRT initialization - calls Windows::Foundation::Initialize(). */
extern tresult CoWinRTInitialize ();
extern void CoWinRTUninitialize ();

} // namespace System
} // namespace COM

#endif // _ccl_cclcoinit_h
