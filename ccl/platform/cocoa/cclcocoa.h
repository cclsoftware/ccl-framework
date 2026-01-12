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
// Filename    : ccl/platform/cocoa/cclcocoa.h
// Description : Cocoa includes
//
//************************************************************************************************

#ifndef _ccl_cocoa_h
#define _ccl_cocoa_h

#include "ccl/public/base/platform.h"
#include "ccl/public/cclexports.h"

#include "ccl/public/base/ccldefpush.h"

#if CCL_PLATFORM_IOS
#import <UIKit/UIKit.h>
#else
#import <Cocoa/Cocoa.h>
#endif

#include "ccl/public/base/ccldefpop.h"

#endif // _ccl_cocoa_h
