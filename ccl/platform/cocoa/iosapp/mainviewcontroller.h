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
// Filename    : ccl/platform/cocoa/iosapp/mainviewcontroller.h
// Description : iOS main view controller
//
//************************************************************************************************

#ifndef _ccl_mainviewcontroller_h
#define _ccl_mainviewcontroller_h

#include "ccl/platform/cocoa/cclcocoa.h"
#include "ccl/public/base/ccldefpush.h"

namespace CCL
{
class IView;
}

//************************************************************************************************
// MainViewController
//************************************************************************************************

@interface CCL_ISOLATED (MainViewController) : UIViewController
{
	CCL::IView* applicationView;
}

@end

#include "ccl/public/base/ccldefpop.h"

#endif // _ccl_mainviewcontroller_h
