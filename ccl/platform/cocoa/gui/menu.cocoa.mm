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
// Filename    : ccl/platform/cocoa/gui/menu.cocoa.mm
// Description : platform-specific menu implementation
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "ccl/platform/cocoa/gui/menu.cocoa.h"

#include "ccl/gui/gui.h"
#include "ccl/gui/commands.h"
#include "ccl/gui/keyevent.h"
#include "ccl/gui/windows/desktop.h"
#include "ccl/public/text/translation.h"
#include "ccl/public/gui/graphics/point.h"
#include "ccl/gui/graphics/imaging/bitmapfilter.h"
#include "ccl/gui/graphics/imaging/bitmappainter.h"
#include "ccl/gui/graphics/imaging/bitmap.h"
#include "ccl/gui/controls/editbox.h"
#include "ccl/public/gui/framework/iwindow.h"
#include "ccl/platform/cocoa/macutils.h"
#include "ccl/platform/cocoa/gui/nativeview.mac.h"
#include "ccl/platform/cocoa/quartz/nshelper.h"
#include "ccl/platform/cocoa/quartz/quartzbitmap.h"
#include "ccl/app/application.h"
#include "ccl/base/asyncoperation.h"

#include "ccl/public/base/ccldefpush.h"

using namespace CCL;
using namespace VKey;

static bool gPopupActive = false;
static MenuItem* gSelectedItem = nullptr;
static String strApplication = "Application";
extern int modalState;
bool isKeyWindow = true;

//////////////////////////////////////////////////////////////////////////////////////////////////
// Strings
//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_XSTRINGS ("MacMenu")
	XSTRING (Hide, "Hide")
	XSTRING (HideOthers, "Hide Others")
	XSTRING (ShowAll, "Show All")
	XSTRING (Quit, "Quit")
	XSTRING (Services, "Services")
END_XSTRINGS

//************************************************************************************************
// MenuController
//************************************************************************************************

@interface CCL_ISOLATED (MenuController) : NSObject<NSMenuDelegate>
{
	CCL::CocoaPopupMenu* cclMenu;
	void* handlerRef;
}

- (BOOL)validateMenuItem:(NSMenuItem*)nsItem;
- (void)menuNeedsUpdate:(NSMenu *)menu;
- (void)selectItem:(id)sender;
- (void)setCCLMenu:(CCL::CocoaPopupMenu*)cclMenu;
- (CCL::CocoaPopupMenu*)getCCLMenu;
- (BOOL)isSystemEditItem:(NSMenuItem*)item;
- (void)selectSystemEditItem:(NSMenuItem*)item;
@end

//////////////////////////////////////////////////////////////////////////////////////////////////

@implementation CCL_ISOLATED (MenuController)

- (void)menuNeedsUpdate:(NSMenu*)menu
{
	cclMenu->init ();

	for(int i = 0, count = cclMenu->countItems (); i < count; i++)
	{
		MenuItem* item = cclMenu->at (i);
		NSMenuItem* nsItem = [menu itemAtIndex:i];
		if(nsItem)
		{
			[nsItem setTag:i];
			[nsItem setEnabled:(item->isEnabled () && isKeyWindow)];
			[nsItem setTarget:isKeyWindow ? [menu delegate] : nil];
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (BOOL)validateMenuItem:(NSMenuItem*)nsItem
{
	if(nsItem.keyEquivalentModifierMask == 0 && NativeTextControl::isNativeTextControlPresent ())
		return NO;

	bool result = false;
	if(!modalState)
	{
		// CMD-Q = Quit always possible when non-modal
		NSInteger idx = [nsItem tag];
		if(idx >= 1000)
			return YES;

		MenuItem* item = cclMenu->at ((int)idx);
		if(item)
		{
			CommandMsgEx msg;
			if(item->makeCommand (msg))
			{
				msg.flags |= CommandMsg::kCheckOnly;

				ICommandHandler* handler = item->getCommandHandler ();
				if(handler)
					result = handler->interpretCommand (msg) != 0;
				else
					result = CommandTable::instance ().interpretCommand (msg);
			}
		}
	}

	if(!result && [self isSystemEditItem:nsItem])
		return [NSApp validateMenuItem:nsItem];

	return result ? YES : NO;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (void)selectItem:(id)sender
{
	NSMenuItem* item = sender;
	if(!modalState)
	{
		NSInteger idx = [item tag];
		if(MenuItem* menuItem = cclMenu->at ((int)idx))
		{
			if(gPopupActive)
			{
				gSelectedItem = menuItem;
				return;
			}
			else if(menuItem->select ())
				return;
		}
	}

	[self selectSystemEditItem:item];
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (void)selectQuitItem:(id)sender
{
	if(modalState)
		return;

	IApplication* application = GUI.getApplication ();
	ASSERT (application != nullptr)
	if(application)
		application->requestQuit ();
	else
		[NSApp terminate:sender];
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (CCL::CocoaPopupMenu*)getCCLMenu
{
	return self->cclMenu;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (void)setCCLMenu:(CCL::CocoaPopupMenu*)_cclMenu
{
	self->cclMenu = _cclMenu;
	self->handlerRef = nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (BOOL)worksWhenModal
{
	return YES;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (BOOL)isSystemEditItem:(NSMenuItem*)item
{
	// CMD-A, CMD-C, CMD-V, CMD-X, CMD-Y, CMD-Z
	if([item.keyEquivalent length] == 0)
		return NO;

	if(item.keyEquivalentModifierMask == NSEventModifierFlagCommand)
		switch([item.keyEquivalent characterAtIndex:0])
		{
		case 0x61 :
		case 0x63 :
		case 0x76 :
		case 0x78 :
		case 0x79 :
		case 0x7A :
			return YES;
		}

	return NO;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (void)selectSystemEditItem:(NSMenuItem*)item
{
	if([item.keyEquivalent length] == 0)
		return;

	if(item.keyEquivalentModifierMask == NSEventModifierFlagCommand)
		switch([item.keyEquivalent characterAtIndex:0])
		{
		case 0x61 :
			[NSApp sendAction:@selector(selectAll:) to:nil from:self];
			break;
		case 0x63 :
			[NSApp sendAction:@selector(copy:) to:nil from:self];
			break;
		case 0x76 :
			[NSApp sendAction:@selector(paste:) to:nil from:self];
			break;
		case 0x78 :
			[NSApp sendAction:@selector(cut:) to:nil from:self];
			break;
		case 0x79 :
			[NSApp sendAction:@selector(redo:) to:nil from:self];
			break;
		case 0x7A :
			[NSApp sendAction:@selector(undo:) to:nil from:self];
			break;
		}
}

@end

//************************************************************************************************
// CocoaPopupMenu
//************************************************************************************************

DEFINE_CLASS_PERSISTENT (CocoaPopupMenu, PopupMenu, "Menu")
DEFINE_CLASS_UID (CocoaPopupMenu, 0x1c1ff2c7, 0xeabe, 0x4b0c, 0xab, 0x94, 0xc2, 0x72, 0x8b, 0xfb, 0xc8, 0x12) // ClassID::Menu

//////////////////////////////////////////////////////////////////////////////////////////////////

CocoaPopupMenu* CocoaPopupMenu::fromSystemMenu (NSMenu* nsMenu)
{
	if(CCL_ISOLATED (MenuController)* delegate = [nsMenu delegate])
		return [delegate getCCLMenu];
	else
		return nil;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CocoaPopupMenu::CocoaPopupMenu ()
: menu (nil),
  delegate (nil),
  isAppMenu (false)
{
	delegate = [[CCL_ISOLATED (MenuController) alloc] init];
	[delegate setCCLMenu:this];
	menu = [[NSMenu alloc] init];
	[menu setDelegate:delegate];
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CocoaPopupMenu::~CocoaPopupMenu ()
{
	if(menu)
		[menu release];
	if(delegate)
		[delegate release];
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IMenu* CCL_API CocoaPopupMenu::createMenu () const
{
	return NEW CocoaPopupMenu;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CocoaPopupMenu::configureAppMenu ()
{
	if(menu)
		[menu release];

	menu = getAppMenu ();
	[menu retain];
	if([menu delegate] == nil)
	{
		[menu setDelegate:delegate];

		NSMenuItem* item = [menu itemWithTitle:@"Quit"];
		if(item) [item setTitle:NSObj<NSString> (String (XSTR(Quit)).createNativeString<NSString*> ())];

		item = [menu itemWithTitle:@"Hide"];
		if(item) [item setTitle:NSObj<NSString> (String (XSTR(Hide)).createNativeString<NSString*> ())];

		item = [menu itemWithTitle:@"Hide Others"];
		if(item) [item setTitle:NSObj<NSString> (String (XSTR(HideOthers)).createNativeString<NSString*> ())];

		item = [menu itemWithTitle:@"Show All"];
		if(item) [item setTitle:NSObj<NSString> (String (XSTR(ShowAll)).createNativeString<NSString*> ())];

		item = [menu itemWithTitle:@"Services"];
		if(item) [item setTitle:NSObj<NSString> (String (XSTR(Services)).createNativeString<NSString*> ())];
	}
	isAppMenu = true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CocoaPopupMenu::realizeItem (MenuItem* item)
{
	int idx = getItemIndex (item);
	NSMenuItem* nsItem = nil;

	if(name == strApplication)
	{
		if(!isAppMenu)
			configureAppMenu ();

		if(item->getName () == "Quit")
		{
			nsItem = [menu itemWithTag:1002];
			if(nsItem != nil)
			{
				NSString* nsTitle = item->getTitle ().createNativeString<NSString*> ();
				[nsItem setTitle:nsTitle];
				[nsItem setTarget:[menu delegate]];
				[nsItem setAction:@selector(selectQuitItem:)];
				[nsTitle release];
				return;
			}
		}
	}

	if(item->isSeparator ())
	{
		[menu insertItem:[NSMenuItem separatorItem] atIndex:idx];
	}
	else
	{
		if(item->isSubMenu ())
		{
			CocoaPopupMenu* subMenu = ccl_cast<CocoaPopupMenu> (item->getSubMenu ());
			if(subMenu)
			{
				NSString* nsTitle = subMenu->getTitle ().createNativeString<NSString*> ();
				NSMenuItem* nsItem = [menu insertItemWithTitle:nsTitle action:nil keyEquivalent:@"" atIndex:idx];
				[nsItem setSubmenu:subMenu->getNSMenu ()];
				[[nsItem submenu] setTitle:nsTitle];
				[nsTitle release];
			}
		}
		else
		{
			NSString* nsTitle = item->getTitle ().createNativeString<NSString*> ();
			if(nsItem == nil)
			{
				nsItem = [menu insertItemWithTitle:nsTitle action:@selector(selectItem:) keyEquivalent:@"" atIndex:idx];
				[nsItem setTag:idx];
			}
			[nsItem setTarget:[menu delegate]];
			[nsItem setEnabled:item->isEnabled ()];
			[nsItem setState:item->isChecked () ? NSControlStateValueOn : NSControlStateValueOff];
			[nsTitle release];
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CocoaPopupMenu::unrealizeItem (MenuItem* item)
{
	int idx = getItemIndex (item);
	[menu removeItemAtIndex:idx];
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CocoaPopupMenu::updateItem (MenuItem* item)
{
	int idx = getItemIndex (item);
	ASSERT (idx >= 0)

	NSMenuItem* nsItem = nil;

	if(name == strApplication)
	{
		if(item->getName () == "Quit")
			nsItem = [menu itemWithTag:1002];

		if(nsItem != nil)
		{
			NSString* nsTitle = item->getTitle ().createNativeString<NSString*> ();
			[nsItem setTitle:nsTitle];
			[nsItem setTarget:[menu delegate]];
			[nsItem setAction:@selector(selectQuitItem:)];
			[nsTitle release];
			return;
		}
	}

	if(nsItem == nil)
	{
		@try
		{
			nsItem = [menu itemAtIndex:idx];
			[nsItem setTag:idx];
		}
		@catch(NSException *e) { return; }
	}

	unsigned int modifier = 0;
	unichar character = 0;
	unichar virtualKey = 0;

	CCL_PRINT (title)
	CCL_PRINT (" ")
	CCL_PRINT (item->getTitle())
	CCL_PRINT (" -> ")

	if(const KeyEvent* key = item->getAssignedKey ())
	{
		#if DEBUG_LOG
		String keyString;
		key->toString (keyString);
		CCL_PRINT (keyString)
		#endif

		if(key->state & KeyState::kCommand)
			modifier |= NSEventModifierFlagCommand;
		if(key->state & KeyState::kOption)
			modifier |= NSEventModifierFlagOption;
		if(key->state & KeyState::kShift)
			modifier |= NSEventModifierFlagShift;
		if(key->state & KeyState::kControl)
			modifier |= NSEventModifierFlagControl;

		switch (key->vKey)
		{
			case kSpace:	 character = ' '; break;
			case kNumPad0:	 character = '0'; modifier |= NSEventModifierFlagNumericPad; break;
			case kNumPad1:   character = '1'; modifier |= NSEventModifierFlagNumericPad; break;
			case kNumPad2:   character = '2'; modifier |= NSEventModifierFlagNumericPad; break;
			case kNumPad3:   character = '3'; modifier |= NSEventModifierFlagNumericPad; break;
			case kNumPad4:   character = '4'; modifier |= NSEventModifierFlagNumericPad; break;
			case kNumPad5:   character = '5'; modifier |= NSEventModifierFlagNumericPad; break;
			case kNumPad6:   character = '6'; modifier |= NSEventModifierFlagNumericPad; break;
			case kNumPad7:   character = '7'; modifier |= NSEventModifierFlagNumericPad; break;
			case kNumPad8:   character = '8'; modifier |= NSEventModifierFlagNumericPad; break;
			case kNumPad9:   character = '9'; modifier |= NSEventModifierFlagNumericPad; break;
			case kMultiply:  character = '*'; break;
			case kAdd:       character = '+'; break;
			case kSubtract:  character = '-'; break;
			case kDecimal:   character = ','; break;
			case kDivide:    character = '/'; break;

			default:
				virtualKey = (unichar)VKey::toSystemKey (key->vKey);
				character = Unicode::toLowercase (key->character);
		}
	}

	[nsItem setEnabled:(item->isEnabled () && isKeyWindow)];
	[nsItem setTarget:isKeyWindow ? [menu delegate] : nil];

	if(item->getIcon ())
	{
		const int kMenuIconSize = 20;

		// convert image to bitmap with unified menu icon size
		SharedPtr<Bitmap> bitmap = ccl_cast<Bitmap> (item->getNativeIcon ());
		if(bitmap == nil)
		{
			item->getIcon ()->setCurrentFrame (0); // always draw first frame to get a consistent result

			BitmapProcessor processor;
			const Point menuIconSize (kMenuIconSize, kMenuIconSize);
			processor.setup (item->getIcon (), Colors::kWhite, 0, &menuIconSize);
			BitmapFilterList copyFilter; // no filtering, just copy
			processor.process (static_cast<IBitmapFilterList&> (copyFilter));
			bitmap = unknown_cast<Bitmap> (processor.getOutput ());
			bitmap->setIsTemplate (item->getIcon ()->getIsTemplate ());
			item->keepNativeIcon (bitmap);
		}

		ASSERT (bitmap != nil)
		if(bitmap)
		{
			NSImage* nsImage = MacOS::nsImageFromBitmap (*bitmap);
			if(bitmap->getIsTemplate ())
				[nsImage setTemplate:YES];
			[nsItem setImage:nsImage];
			[nsImage release];
		}
	}

	if(item->isSubMenu ())
	{
		PopupMenu* subMenu = ccl_cast<PopupMenu> (item->getSubMenu ());
		if(subMenu)
		{
			NSString* nsTitle = subMenu->getTitle ().createNativeString<NSString*> ();
			[[nsItem submenu] setTitle:nsTitle];
			[nsItem setTitle:nsTitle];
			[nsTitle release];
		}
	}
	else
	{
		if(isKeyWindow && (character || virtualKey))
		{
			if(character)
				[nsItem setKeyEquivalent:[NSString stringWithCharacters:&character length:1]];
			else if(virtualKey)
				[nsItem setKeyEquivalent: [NSString stringWithCharacters:&virtualKey length:1]];
			[nsItem setKeyEquivalentModifierMask:modifier];
		}
		else
		{
			[nsItem setKeyEquivalent:@""];
			[nsItem setKeyEquivalentModifierMask:0];
		}

		[nsItem setState:item->isChecked () ? NSControlStateValueOn : NSControlStateValueOff];

		NSString* nsTitle = item->getTitle ().createNativeString<NSString*> ();
		[nsItem setTitle:nsTitle];
		[nsTitle release];
	}

	CCL_PRINT ("\n");
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IAsyncOperation* CocoaPopupMenu::popupPlatformMenu (const CCL::Point& where, IWindow* window)
{
	if(!menu)
		return nullptr;

	ScopedVar<bool> var (gPopupActive, true);

	NSWindow* nsWindow = toNSWindow (window);

	NSEvent* event = [NSApp currentEvent];
	if(event.type == NSEventTypeRightMouseDown)
		[NSMenu popUpContextMenu:menu withEvent:event forView:[nsWindow contentView]];

	MenuItemID result = -1;
	if(gSelectedItem)
		result = gSelectedItem->getItemID ();
	gSelectedItem = nullptr;

	// return an AsyncOperation (already completed, since we ran modally)
	return AsyncOperation::createCompleted (result);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

NSMenu* CocoaPopupMenu::getAppMenu ()
{
	NSMenu* menuBar = [NSApp mainMenu];
	NSMenuItem* appMenuItem = [menuBar itemAtIndex:0];
	NSMenu* appMenu = [appMenuItem submenu];

	return appMenu;
}

//************************************************************************************************
// CocoaMenuBar
//************************************************************************************************

DEFINE_CLASS (CocoaMenuBar, MenuBar)
DEFINE_CLASS_UID (CocoaMenuBar, 0x32ac7729, 0x5ee3, 0x4273, 0xaf, 0x9d, 0xaf, 0x50, 0x1e, 0x7c, 0xe5, 0xb0) // ClassID::MenuBar

DEFINE_CLASS (CocoaVariantMenuBar, CocoaMenuBar)
DEFINE_CLASS_UID (CocoaVariantMenuBar, 0xd0d769c9, 0xe469, 0x445a, 0xb1, 0x9, 0x66, 0x7f, 0x55, 0xe1, 0xa0, 0xf5) // ClassID::VariantMenuBar

//////////////////////////////////////////////////////////////////////////////////////////////////

CocoaMenuBar::CocoaMenuBar ()
{
	menu = [NSApp mainMenu];
	[NSMenuItem setUsesUserKeyEquivalents:YES];
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CocoaMenuBar::~CocoaMenuBar ()
{
	[[NSNotificationCenter defaultCenter] removeObserver:[NSApp delegate]];
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CocoaMenuBar::activatePlatformMenu ()
{
	NSNotificationCenter* nc = [NSNotificationCenter defaultCenter];
	[nc addObserver:[NSApp delegate] selector:@selector(menuTrackingStarted:) name:NSMenuDidBeginTrackingNotification object:[NSApp mainMenu]];
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CocoaMenuBar::insertPlatformMenu (PopupMenu* menu)
{
	if(menu->getName () == strApplication)
		return;

	int index = menus.index (menu);
	ASSERT (index >= 0)

	CocoaPopupMenu* cocoaMenu = ccl_cast<CocoaPopupMenu> (menu);

	NSMenuItem* item = [[NSMenuItem alloc] init];

	NSString* nsTitle = menu->getTitle ().createNativeString<NSString*> ();
	[cocoaMenu->getNSMenu () setTitle:nsTitle];
	[nsTitle release];
	[item setSubmenu:cocoaMenu->getNSMenu ()];
	[item setTag:index];
	[[NSApp mainMenu] insertItem:item atIndex:index];
	[item release];
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CocoaMenuBar::removePlatformMenu (PopupMenu* menu)
{
	if(menu->getName () == strApplication)
		return;

	int index = menus.index (menu);
	ASSERT (index >= 0)

	@try
	{
		NSMenu* nsMenu = [NSApp mainMenu];
		[nsMenu removeItem:[nsMenu itemAtIndex:index]];
	}
	@catch(NSException *e)
	{
		return;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CocoaMenuBar::updateMenu (Menu* _menu)
{
	CocoaPopupMenu* menu = ccl_cast<CocoaPopupMenu> (_menu);
	if(menu == nil)
		return;

	NSString* nsTitle = menu->getTitle ().createNativeString<NSString*> ();
	[menu->getNSMenu () setTitle:nsTitle];
	[nsTitle release];
}
