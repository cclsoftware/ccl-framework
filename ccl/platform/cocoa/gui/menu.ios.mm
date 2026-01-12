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
// Filename    : ccl/platform/cocoa/gui/menu.ios.mm
// Description : platform-specific menu implementation
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "ccl/gui/gui.h"
#include "ccl/gui/popup/menu.h"
#include "ccl/gui/windows/window.h"
#include "ccl/base/asyncoperation.h"

#include "ccl/platform/cocoa/cclcocoa.h"

#include "ccl/public/base/ccldefpush.h"
#import <objc/runtime.h>

@interface CCL_ISOLATED (PopupMenuDelegate) : UIResponder
{
	CCL::PopupMenu* _popupMenu;
	UIResponder* _nextResponder;
	CCL::AsyncOperation* _asyncOperation;
}

+ (SEL)selectorForIndex:(NSInteger)index;
- (id)initWithNextResponder:(UIResponder*)responder andPopupMenu:(CCL::PopupMenu*)menu andAsyncOperation:(CCL::AsyncOperation*)operation;
- (void)selectItem:(id)sender;
- (void)onPopupDidHide:(id)sender;
- (void)dismiss;

@end

//////////////////////////////////////////////////////////////////////////////////////////////////

@implementation CCL_ISOLATED (PopupMenuDelegate)

static NSString* selectPrefix = @"select_";

+ (SEL)selectorForIndex:(NSInteger)index;
{
	NSString* selectorString = [selectPrefix stringByAppendingFormat:@"%ld:", (long)index];
	SEL selector = sel_registerName([selectorString UTF8String]);
	class_addMethod ([self class], selector, [[self class] instanceMethodForSelector:@selector (selectItem:)], "v@:@");
	return selector;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (id)initWithNextResponder:(UIResponder*)responder andPopupMenu:(CCL::PopupMenu*)menu andAsyncOperation:(CCL::AsyncOperation*)operation
{
	if(self = [super init])
	{
		_nextResponder = responder;
		_popupMenu = menu;
		ASSERT(_popupMenu)
		_asyncOperation = operation;
		ASSERT(_asyncOperation)
		_asyncOperation->retain ();	
		[[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(onPopupDidHide:) name:UIMenuControllerWillHideMenuNotification object:nil]; 
		[self becomeFirstResponder];
	}
	return self;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (void)dealloc
{
	[[NSNotificationCenter defaultCenter] removeObserver:self]; 
	[super dealloc];
}

////////////////////////////////////////////////////////////////////////////////////////////////////

- (void)selectItem:(id)sender
{
	NSString* selectorString = NSStringFromSelector (_cmd);
	NSScanner* scanner = [NSScanner scannerWithString:selectorString];
	[scanner setScanLocation:[selectPrefix length]];
	NSInteger index = 0;
	[scanner scanInteger:&index];
	[self finishOperation:index];
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (void)onPopupDidHide:(id)sender
{
	[self resignFirstResponder];
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (void)dismiss
{
	[super resignFirstResponder];
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (void)finishOperation:(NSInteger)index
{
	if(!_asyncOperation)
		return;
	
	if(index >= 0)
	{
		if(CCL::MenuItem* item = _popupMenu->at ((int)index))
		{
			_asyncOperation->setResult (item->getItemID ());
			_asyncOperation->setState (CCL::AsyncOperation::kCompleted);
		}			
	}
	else
		_asyncOperation->setState (CCL::AsyncOperation::kCanceled);
	_asyncOperation->release ();
	_asyncOperation = nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (BOOL)canBecomeFirstResponder
{
	return YES;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (UIResponder*)nextResponder
{
	return _nextResponder;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

- (BOOL)resignFirstResponder
{
	BOOL result = [super resignFirstResponder];
	[self finishOperation: -1];
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (BOOL)canPerformAction:(SEL)action withSender:(id)sender
{
	if(action == @selector(selectItem:))
		return YES;
	NSString* selectorString = NSStringFromSelector (action);
	if([selectorString hasPrefix:selectPrefix])
		return YES;
	return NO;
}

@end

namespace CCL {

//************************************************************************************************
// IOSPopupMenu
//************************************************************************************************

void linkIOSPopupMenu ();

class IOSPopupMenu: public PopupMenu
{
public:
	DECLARE_CLASS (IOSPopupMenu, PopupMenu)

	IOSPopupMenu ();
	~IOSPopupMenu ();

	PROPERTY_VARIABLE (CCL_ISOLATED (PopupMenuDelegate)*, delegate, Delegate)
	
	// PopupMenu
	IMenu* CCL_API createMenu () const override;
	void updateItem (MenuItem* item) override;
	void realizeItem (MenuItem* item) override;
	void unrealizeItem (MenuItem* item) override;
	IAsyncOperation* popupPlatformMenu (const Point& where, IWindow* window) override;
};

} // namespace CCL

using namespace CCL;

//************************************************************************************************
// IOSPopupMenu
//************************************************************************************************

void linkIOSPopupMenu () {} // call to link this file

int getPlatformIndex (const Menu& menu, MenuItem* menuItem)
{
	int platformIndex = -1;
	for(int i = 0, num = menu.countItems (); i < num; i++)
	{
		MenuItem* item = menu.at (i);
		if(item && !item->isSeparator () && !item->isSubMenu ())
		{
			platformIndex++;
		
			if(item == menuItem)
				return platformIndex;
		}
	}
	return -1;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_CLASS_PERSISTENT (IOSPopupMenu, PopupMenu, "Menu")
DEFINE_CLASS_UID (IOSPopupMenu, 0x1c1ff2c7, 0xeabe, 0x4b0c, 0xab, 0x94, 0xc2, 0x72, 0x8b, 0xfb, 0xc8, 0x12) // ClassID::Menu

//////////////////////////////////////////////////////////////////////////////////////////////////

IOSPopupMenu::IOSPopupMenu ()
: delegate (nil)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

IOSPopupMenu::~IOSPopupMenu ()
{
	if(delegate)
	{
		[[NSNotificationCenter defaultCenter] removeObserver:delegate]; 
		[delegate dismiss];
		[delegate release];
	}
	delegate = nil;
	UIMenuController* menuController = [UIMenuController sharedMenuController];
	[menuController setMenuVisible:NO animated:YES];
	[menuController setMenuItems:nil];
	[menuController update];
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IMenu* CCL_API IOSPopupMenu::createMenu () const
{
	return NEW IOSPopupMenu;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void IOSPopupMenu::realizeItem (MenuItem* item)
{
	if((item->isSeparator () || item->isSubMenu ()))
		return;

	NSString* title = item->getTitle ().createNativeString<NSString*> ();
	UIMenuController* menuController = [UIMenuController sharedMenuController];
	NSMutableArray* newMenuItems = nil;
	if(NSArray* presentMenuItems = [menuController menuItems])
		newMenuItems = [NSMutableArray arrayWithArray:presentMenuItems];
	else
		newMenuItems = [[[NSMutableArray alloc] init] autorelease];

	int index = getItemIndex (item);
	UIMenuItem* menuItem = [[UIMenuItem alloc] initWithTitle:title action:[CCL_ISOLATED (PopupMenuDelegate) selectorForIndex:index]];
	[title release];

#if 1
	[newMenuItems addObject:menuItem];
#else
	while(index >= [newMenuItems count])
		[newMenuItems addObject:[[[UIMenuItem alloc] initWithTitle:@"" action:@selector(selectItem:)] autorelease]];
	[newMenuItems replaceObjectAtIndex:index withObject:menuItem];
#endif
	[menuItem release];
	[menuController setMenuItems:newMenuItems];
}	

//////////////////////////////////////////////////////////////////////////////////////////////////

void IOSPopupMenu::unrealizeItem (MenuItem* item)
{
	UIMenuController* menuController = [UIMenuController sharedMenuController];
	if(NSArray* presentMenuItems = [menuController menuItems])
	{
		NSMutableArray* newMenuItems = [NSMutableArray arrayWithArray:presentMenuItems];
		int platformIndex = getPlatformIndex (*this, item);
		if(platformIndex < [newMenuItems count])
		{
			[newMenuItems removeObjectAtIndex:platformIndex];
			[menuController setMenuItems:newMenuItems];
		}	
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void IOSPopupMenu::updateItem (MenuItem* item)
{
	UIMenuController* menuController = [UIMenuController sharedMenuController];
	if(NSArray* presentMenuItems = [menuController menuItems])
	{
		NSMutableArray* newMenuItems = [NSMutableArray arrayWithArray:presentMenuItems];
		int index = getItemIndex (item);
		int platformIndex = getPlatformIndex (*this, item);
		
		if(platformIndex < [newMenuItems count])
		{
			NSString* title = item->getTitle ().createNativeString<NSString*> ();
			UIMenuItem* menuItem = [[UIMenuItem alloc] initWithTitle:title action:[CCL_ISOLATED (PopupMenuDelegate) selectorForIndex:index]];
			[title release];	
			[newMenuItems replaceObjectAtIndex:platformIndex withObject:menuItem];
			[menuItem release];
			[menuController setMenuItems:newMenuItems];			
		}	
	}	
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IAsyncOperation* IOSPopupMenu::popupPlatformMenu (const CCL::Point& where, IWindow* window)
{
	UIMenuController* menuController = [UIMenuController sharedMenuController];
	UIViewController* viewController = nil;
	if(window)
		viewController = (UIViewController*)window->getSystemWindow ();
	else
		viewController = [(UIWindow*)[[[UIApplication sharedApplication] windows] objectAtIndex:0] rootViewController];
	if(!viewController)
		return nil;

	AsyncOperation* asyncOperation = NEW AsyncOperation;
	asyncOperation->setState (AsyncOperation::kStarted);
	delegate = [[CCL_ISOLATED (PopupMenuDelegate) alloc] initWithNextResponder:viewController andPopupMenu:this andAsyncOperation:asyncOperation];
	[menuController setTargetRect:CGRectMake (where.x, where.y, 0.0, 0.0) inView:[viewController view]];	
	[menuController setMenuVisible:YES animated:YES];
	return asyncOperation;
}

//************************************************************************************************
// MenuBar
//************************************************************************************************

// no MenuBar on iOS (classes ClassID::MenuBar, ClassID::VariantMenuBar not available)
