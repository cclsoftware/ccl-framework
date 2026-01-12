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
// Filename    : ccl/platform/cocoa/gui/transparentwindow.ios.h
// Description : Transparent Window
//
//************************************************************************************************

#ifndef _ccl_transparentwindow_ios_h
#define _ccl_transparentwindow_ios_h

#include "ccl/platform/cocoa/cclcocoa.h"

#include "ccl/public/base/ccldefpush.h"

namespace CCL {
class IOSTransparentWindow; }

//************************************************************************************************
// TransparentWindowView
//************************************************************************************************

@interface CCL_ISOLATED (TransparentWindowView): UIView
{
	CCL::IOSTransparentWindow* transparentWindow;
}

- (id)initWithFrame:(CGRect)frameRect transparentWindow:(CCL::IOSTransparentWindow*)window;
- (void)drawRect:(CGRect)rect;
@end

//////////////////////////////////////////////////////////////////////////////////////////////////

#include "ccl/public/base/ccldefpop.h"

#endif // _ccl_transparentwindow_ios_h
