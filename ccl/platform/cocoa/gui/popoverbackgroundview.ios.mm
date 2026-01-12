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
// Filename    : ccl/platform/cocoa/gui/popoverbackgroundview.ios.mm
// Description : Background views for popoverviews
//
//************************************************************************************************

#include "popoverbackgroundview.ios.h"

//************************************************************************************************
// OpaquePopoverBackground
//************************************************************************************************

@implementation CCL_ISOLATED (OpaquePopoverBackgroundView)

@synthesize arrowOffset;
@synthesize arrowDirection;

+ (UIEdgeInsets)contentViewInsets
{
    return UIEdgeInsetsMake (2, 2 ,2 ,2);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

+ (CGFloat)arrowHeight
{
    return 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

+ (CGFloat)arrowBase
{
    return 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (id)initWithFrame:(CGRect)frame
{
    if(self = [super initWithFrame:frame])
    {
        CGRect rect = CGRectMake (0, 0, 4, 4);
        UIGraphicsBeginImageContext (rect.size);
        CGContextRef context = UIGraphicsGetCurrentContext ();
        
        CGContextSetFillColorWithColor (context, [[UIColor blackColor] CGColor]);
        CGContextFillRect (context, rect);
        
        UIImage* image = UIGraphicsGetImageFromCurrentImageContext ();
        UIGraphicsEndImageContext ();
        
        self.background = [[UIImageView alloc] initWithImage:image];
        [self addSubview:self.background];
    }
    return  self;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (void)layoutSubviews
{
    self.background.frame = self.frame;
}

@end

//************************************************************************************************
// TransparentPopoverBackground
//************************************************************************************************

@implementation CCL_ISOLATED (TransparentPopoverBackgroundView)

@synthesize arrowOffset;
@synthesize arrowDirection;

+ (UIEdgeInsets)contentViewInsets
{
    return UIEdgeInsetsMake (0, 0, 0 , 0);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

+ (CGFloat)arrowHeight
{
    return 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

+ (CGFloat)arrowBase
{
    return 0;
}

@end
