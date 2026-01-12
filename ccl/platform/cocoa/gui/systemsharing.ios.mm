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
// Filename    : ccl/platform/cocoa/gui/systemsharing.ios.mm
// Description : Platform-specific system sharing handler for iOS
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "ccl/public/gui/framework/isystemsharing.h"

#include "ccl/base/storage/url.h"
#include "ccl/base/asyncoperation.h"
#include "ccl/gui/windows/desktop.h"
#include "ccl/platform/cocoa/macutils.h"
#include "ccl/platform/cocoa/gui/window.ios.h"
#include "ccl/platform/cocoa/gui/popoverbackgroundview.ios.h"
#include "ccl/platform/cocoa/quartz/cghelper.h"

#include "ccl/public/base/ccldefpush.h"

namespace CCL {

//************************************************************************************************
// IOSSystemSharingHandler
//************************************************************************************************

class IOSSystemSharingHandler: public Object,
                               public ISystemSharingHandler
{
public:
    DECLARE_CLASS (IOSSystemSharingHandler, Object)

	// ISystemSharingHandler
    IAsyncOperation* CCL_API shareFile (UrlRef url, IWindow* window = nullptr) override;
    IAsyncOperation* CCL_API shareText (StringRef text, IWindow* window = nullptr) override;

    CLASS_INTERFACE (ISystemSharingHandler, Object)

private:
    IAsyncOperation* shareInternal (UIActivityViewController* activityViewController, IWindow* window);
};

} // namespace CCL

using namespace CCL;

//************************************************************************************************
// IOSSystemSharingHandler
//************************************************************************************************

DEFINE_CLASS_PERSISTENT (IOSSystemSharingHandler, Object, "SystemSharingHandler")
DEFINE_CLASS_UID (IOSSystemSharingHandler, 0x3421790e, 0x33c8, 0x430a, 0xa4, 0x98, 0x97, 0x1f, 0x0d, 0xb2, 0x56, 0x22) // ClassID::SystemSharingHandler

//////////////////////////////////////////////////////////////////////////////////////////////////

IAsyncOperation* CCL_API IOSSystemSharingHandler::shareFile (UrlRef url, IWindow* window)
{
    NSURL* nsUrl = [NSURL alloc];
    MacUtils::urlToNSUrl (url, nsUrl);
    UIActivityViewController* activityViewController = [[UIActivityViewController alloc] initWithActivityItems:@[nsUrl] applicationActivities:nil];
    return shareInternal (activityViewController, window);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IAsyncOperation* CCL_API IOSSystemSharingHandler::shareText (StringRef text, IWindow* window)
{   
	NSString* nsString = [text.createNativeString<NSString*> () autorelease];
    UIActivityViewController* activityViewController = [[UIActivityViewController alloc] initWithActivityItems:@[nsString] applicationActivities:nil];
    return shareInternal (activityViewController, window);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IAsyncOperation* IOSSystemSharingHandler::shareInternal (UIActivityViewController* activityViewController, IWindow* window)
{
    if(!window)
        window = Desktop.getDialogParentWindow ();
    
    IOSWindow* iosWindow = IOSWindow::cast (unknown_cast<Window> (window));
    
    if(iosWindow)
    {
        AutoPtr<AsyncOperation> asyncOperation (NEW AsyncOperation);
        asyncOperation->setState (AsyncOperation::kStarted);
        
        UIViewController* parentController = (UIViewController*)(iosWindow->getTopViewController ());
        activityViewController.completionWithItemsHandler = ^(UIActivityType __nullable activityType, BOOL completed, NSArray* __nullable returnedItems, NSError* __nullable activityError)
        {
            CCL_PRINTF ("systemsharing.ios.mm: Completed: %d; # returnedItems: %d\n", completed, [returnedItems count]);
            ASSERT (activityError == nullptr)

            if(activityError)
            {
                if(activityError.code == NSUserCancelledError)
                    asyncOperation->setState (AsyncOperation::kCanceled);
                else
                    asyncOperation->setState (AsyncOperation::kFailed);
            }
            else
            {
                if(completed)
                    asyncOperation->setState (AsyncOperation::kCompleted);
                else
                    asyncOperation->setState (AsyncOperation::kCanceled);
            }
            
            [parentController dismissViewControllerAnimated:YES completion:nil];
            [activityViewController release];
        };

        
        if([[UIDevice currentDevice] userInterfaceIdiom] == UIUserInterfaceIdiomPad)
        {
            CGRect sourceRect = parentController.view.bounds;
            CGRect anchorRect = CGRectMake (CGRectGetMidX(sourceRect), CGRectGetMaxY (sourceRect) - 16, 1.0, 1.0);
            
            activityViewController.popoverPresentationController.popoverBackgroundViewClass = [CCL_ISOLATED (TransparentPopoverBackgroundView) class];
            activityViewController.popoverPresentationController.sourceView = parentController.view;
            activityViewController.popoverPresentationController.sourceRect = anchorRect;
        }
            
        [parentController presentViewController:activityViewController animated:YES completion:nil];
        
        return return_shared<IAsyncOperation> (asyncOperation);
    }
     
    return nullptr;
}
