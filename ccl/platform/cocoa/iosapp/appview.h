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
// Filename    : ccl/platform/cocoa/iosapp/appview.h
// Description : iOS View to CCL form bridge
//
//************************************************************************************************

#include "ccl/platform/cocoa/iosapp/contentview.h"
#include "ccl/public/gui/iapplication.h"

#include "ccl/platform/cocoa/cclcocoa.h"

#include "ccl/public/base/ccldefpush.h"

namespace CCL
{
class IView;
};

//************************************************************************************************
// AppView
//************************************************************************************************

@interface CCL_ISOLATED (AppView) : CCL_ISOLATED (ContentView)
{
	@public
	UIWindow* nativeWindow;
}
- (CCL::IView*)createApplicationView:(CCL::IApplication*)application;
- (void)setSize:(CCL::Rect&)size;

@end

#include "ccl/public/base/ccldefpop.h"
