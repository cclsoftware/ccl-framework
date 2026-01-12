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
// Filename    : ccl/platform/cocoa/gui/alert.ios.mm
// Description : Platform alert
//
//************************************************************************************************

#include "ccl/gui/dialogs/alert.h"
#include "ccl/gui/windows/desktop.h"
#include "ccl/platform/cocoa/gui/window.ios.h"
#include "ccl/base/asyncoperation.h"

#include "ccl/platform/cocoa/cclcocoa.h"

#include "ccl/public/base/ccldefpush.h"

namespace CCL {

//************************************************************************************************
// IOSAlertBox
//************************************************************************************************

class IOSAlertBox: public AlertBox
{
public:
	DECLARE_CLASS (IOSAlertBox, AlertBox)
    
    IOSAlertBox ()
    : parentController (nil)
    {}
    
    ~IOSAlertBox ()
    {
        if(parentController)
        {
            [parentController release];
        }
    }

	// AlertBox
	void closePlatform () override;
	int CCL_API run () override;
	IAsyncOperation* CCL_API runAsyncPlatform () override;

protected:
	SharedPtr<AsyncOperation> operation;
    UIViewController* parentController;
};

} // namespace CCL

using namespace CCL;

//************************************************************************************************
// Alert::ButtonMapping
//************************************************************************************************

int Alert::ButtonMapping::getResultAtButtonIndex (int buttonIndex) const
{
	if(otherResult != kUndefined) switch(buttonIndex)
	{
	case 0 : return alternateResult;	// top
	case 1 : return defaultResult;		// middle
	case 2 : return otherResult;		// bottom
	}
	else switch(buttonIndex)
	{
	case 0 : return alternateResult; // top
	case 1 : return defaultResult;   // bottom
	}				
	return kUndefined;
}

//************************************************************************************************
// IOSAlertBox
//************************************************************************************************

DEFINE_CLASS (IOSAlertBox, AlertBox)
DEFINE_CLASS_UID (IOSAlertBox, 0x9bf3ecb5, 0x5bb2, 0x4eb4, 0xaa, 0xac, 0x29, 0xaf, 0xf4, 0x66, 0x45, 0xa5) // ClassID::AlertBox

//////////////////////////////////////////////////////////////////////////////////////////////////

void IOSAlertBox::closePlatform ()
{
    if(parentController)
    {
        [parentController dismissViewControllerAnimated:YES completion:nil];
    }

	if(operation)
	{
		operation->setResult (Alert::kUndefined);
		operation->setState (AsyncOperation::kCompleted);
	}
    
	this->release ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API IOSAlertBox::run ()
{
	// only runAsyncPlatform supported on iOS
	ASSERT (0)
	return 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IAsyncOperation* IOSAlertBox::runAsyncPlatform ()
{
    IOSWindow* iosWindow = IOSWindow::cast (unknown_cast<Window> (Desktop.getDialogParentWindow ()));
	
	if(iosWindow)
	{
        parentController = (UIViewController*)(iosWindow->getTopViewController ());
        
        if(parentController)
        {
            [parentController retain];
            
            operation = NEW AsyncOperation;
            operation->setState (AsyncOperation::kStarted);
            
            NSString* title = [getTitle ().createNativeString<NSString*> () autorelease];
            NSString* message = [getText ().createNativeString<NSString*> () autorelease];
            UIAlertController* alert = [UIAlertController alertControllerWithTitle:title message:message preferredStyle:UIAlertControllerStyleAlert];
            
            auto addButton = [&] (int result, NSString* title)
            {
                void(^buttonHandler)(UIAlertAction *action) = ^void(UIAlertAction *action)
                {
                    operation->setResult (result);
                    operation->setState (AsyncOperation::kCompleted);
                    this->release ();
                };
                [alert addAction:[UIAlertAction actionWithTitle:title style:UIAlertActionStyleDefault handler:buttonHandler]];
            };
            
            // add buttons from top to bottom
            if(NSString* buttonTitle = [getFirstButton ().createNativeString<NSString*> () autorelease])
                if(buttonTitle.length > 0)
                    addButton (firstResult, buttonTitle);
            
            if(NSString* buttonTitle = [getSecondButton ().createNativeString<NSString*> () autorelease])
                if(buttonTitle.length > 0)
                    addButton (secondResult, buttonTitle);

            if(NSString* buttonTitle = [getThirdButton ().createNativeString<NSString*> () autorelease])
                if(buttonTitle.length > 0)
                    addButton (thirdResult, buttonTitle);
            
            [parentController presentViewController:alert animated:YES completion:nil];

            this->retain ();

            return operation;
        }
	}
    
    return nullptr;
}
