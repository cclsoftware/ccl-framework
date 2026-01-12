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
// Filename    : ccl/platform/cocoa/gui/notifyicon.cocoa.mm
// Description : Cocoa Notification Icon
//
//************************************************************************************************

#include "ccl/gui/system/notifyicon.h"
#include "ccl/gui/graphics/imaging/multiimage.h"

#include "ccl/platform/cocoa/gui/menu.cocoa.h"
#include "ccl/platform/cocoa/quartz/nshelper.h"

#include "ccl/public/base/ccldefpush.h"

using namespace CCL;

//************************************************************************************************
// NotifyIconDelegate
//************************************************************************************************

@interface CCL_ISOLATED (NotifyIconDelegate) : NSObject<NSUserNotificationCenterDelegate>
- (BOOL)userNotificationCenter:(NSUserNotificationCenter*)center shouldPresentNotification:(NSUserNotification*)notification;

@end

//************************************************************************************************
// CocoaNotifyIcon
//************************************************************************************************

class CocoaNotifyIcon: public NotifyIcon
{
public:
	DECLARE_CLASS (CocoaNotifyIcon, NotifyIcon)

	CocoaNotifyIcon ();
	~CocoaNotifyIcon ();

    void updateMenu ();
    
protected:
    NSStatusItem* item;
    CCL_ISOLATED (NotifyIconDelegate)* delegate;
	AutoPtr<PopupMenu> menu;
    
	// NotifyIcon
	void updateVisible (bool state) override;
	void updateTitle () override;
	void updateImage () override;
	void showInfo (const Alert::Event& e) override;
	void reportEvent (const Alert::Event& e) override;
	tresult CCL_API setHandler (IUnknown* handler) override;
};

//************************************************************************************************
// NotifyIconDelegate
//************************************************************************************************

@implementation CCL_ISOLATED (NotifyIconDelegate)

- (BOOL)userNotificationCenter:(NSUserNotificationCenter*)center shouldPresentNotification:(NSUserNotification*)notification
{
	return YES;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

@end

//************************************************************************************************
// CocoaNotifyIcon
//************************************************************************************************

DEFINE_CLASS_PERSISTENT (CocoaNotifyIcon, NotifyIcon, "NotifyIcon")
DEFINE_CLASS_UID (CocoaNotifyIcon, 0x6d51b752, 0xb1c9, 0x44c2, 0xb5, 0xb4, 0x88, 0x6c, 0x61, 0x10, 0xc, 0xe4)

//////////////////////////////////////////////////////////////////////////////////////////////////

CocoaNotifyIcon::CocoaNotifyIcon ()
: item (nil),
  delegate (nil)
{
	item = [[[NSStatusBar systemStatusBar] statusItemWithLength:NSVariableStatusItemLength] retain];
    delegate = [[CCL_ISOLATED (NotifyIconDelegate) alloc] init];
	[[NSUserNotificationCenter defaultUserNotificationCenter] setDelegate:delegate];
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CocoaNotifyIcon::~CocoaNotifyIcon ()
{
	setVisible (false);
	[[NSUserNotificationCenter defaultUserNotificationCenter] setDelegate:nil];
    [delegate release];
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CocoaNotifyIcon::updateVisible (bool state)
{
	if(item)
		item.visible = state;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CocoaNotifyIcon::updateTitle ()
{
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CocoaNotifyIcon::updateImage ()
{
    if(image)
    {
		Image* selectedImage = ImageResolutionSelector::selectImage (image, CCL::Point (16,16));
		if(Bitmap* bitmap = ccl_cast<Bitmap> (selectedImage))
			if(NSImage* nsImage = MacOS::nsImageFromBitmap (*bitmap))
			{
				if(image->getIsTemplate ())
					[nsImage setTemplate:YES];
				
				item.button.image = nsImage;
				[nsImage release];
			}
    }
    else
        item.button.image = nil;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CocoaNotifyIcon::showInfo (const Alert::Event& e)
{	
	NSUserNotification* notification = [[[NSUserNotification alloc] init] autorelease];
	notification.title = [title.createNativeString<NSString*> () autorelease];
	notification.subtitle = @"";
	notification.informativeText =  [e.message.createNativeString<NSString*> () autorelease];
	[[NSUserNotificationCenter defaultUserNotificationCenter] deliverNotification:notification];
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CocoaNotifyIcon::updateMenu ()
{
	menu = createContextMenu ();
	CocoaPopupMenu* cocoaMenu = ccl_cast<CocoaPopupMenu> (menu);
	if(cocoaMenu)
		[item setMenu: cocoaMenu->getNSMenu ()];
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CocoaNotifyIcon::reportEvent (const Alert::Event& e)
{	
	showInfo (e);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API CocoaNotifyIcon::setHandler (IUnknown* handler)
{
	tresult result = NotifyIcon::setHandler (handler);
	
	if(result == kResultOk)
		updateMenu ();

	return result;
}
