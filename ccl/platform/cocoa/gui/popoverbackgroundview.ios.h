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
// Filename    : ccl/platform/cocoa/gui/popoverbackground.ios.h
// Description : popoverBackgroundViews for popoverViewControllers
//
//************************************************************************************************

#ifndef _ccl_popoverbackgroundview_ios_h
#define _ccl_popoverbackgroundview_ios_h

#include "ccl/platform/cocoa/cclcocoa.h"

#include "ccl/public/base/ccldefpush.h"

//************************************************************************************************
// OpaquePopoverBackgroundView
//************************************************************************************************

@interface CCL_ISOLATED (OpaquePopoverBackgroundView): UIPopoverBackgroundView

+ (UIEdgeInsets)contentViewInsets;
+ (CGFloat)arrowHeight;
+ (CGFloat)arrowBase;

@property (nonatomic, strong) UIImageView* background;
@property (nonatomic, readwrite) CGFloat arrowOffset;
@property (nonatomic, readwrite) UIPopoverArrowDirection arrowDirection;
@end

//************************************************************************************************
// TransparentPopoverBackground
//************************************************************************************************

@interface CCL_ISOLATED (TransparentPopoverBackgroundView): UIPopoverBackgroundView

+ (UIEdgeInsets)contentViewInsets;
+ (CGFloat)arrowHeight;
+ (CGFloat)arrowBase;

@property (nonatomic, readwrite) CGFloat arrowOffset;
@property (nonatomic, readwrite) UIPopoverArrowDirection arrowDirection;

@end

#endif // _ccl_popoverbackgroundview_ios_h
