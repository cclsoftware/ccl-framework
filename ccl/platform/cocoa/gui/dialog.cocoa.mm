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
// Filename    : ccl/platform/cocoa/gui/dialog.cocoa.mm
// Description : platform-specific dialog implementation
//
//************************************************************************************************

#include "ccl/gui/windows/dialog.h"
#include "ccl/gui/dialogs/progressdialog.h"
#include "ccl/gui/popup/popupselector.h"
#include "ccl/gui/windows/desktop.h"
#include "ccl/gui/controls/editbox.h"
#include "ccl/gui/gui.h"
#include "ccl/gui/keyevent.h"
#include "ccl/gui/system/dragndrop.h"

#include "ccl/base/asyncoperation.h"

#include "ccl/platform/cocoa/macutils.h"
#include "ccl/platform/cocoa/gui/nativeview.mac.h"
#include "ccl/platform/cocoa/quartz/nshelper.h"

#include "ccl/base/storage/configuration.h"

#include "platformwindow.mac.h"

#include "ccl/public/base/ccldefpush.h"

using namespace CCL;
using namespace MacOS;

extern bool inQuitEvent;

//************************************************************************************************
// DialogController
//************************************************************************************************

@interface CCL_ISOLATED (DialogController): NSObject<NSWindowDelegate>
{
	CCL::Dialog* frameworkDialog;
}

- (CCL_ISOLATED (DialogController)*)initWithFrameworkDialog:(CCL::Dialog*)dialog;
- (BOOL)windowShouldClose:(id)window;
- (void)windowDidResize:(NSNotification*)notification;
- (void)windowDidBecomeKey:(NSNotification*)notification;
- (void)windowDidBecomeMain:(NSNotification*)notification;
- (BOOL)windowShouldZoom:(NSWindow*)window toFrame:(NSRect)proposedFrame;
@end

@implementation CCL_ISOLATED (DialogController)

//////////////////////////////////////////////////////////////////////////////////////////////////

- (id)initWithFrameworkDialog:(CCL::Dialog*)dialog
{
	if(self = [super init])
		frameworkDialog = dialog;
	return self;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (BOOL)windowShouldClose:(id)window
{
	frameworkDialog->setDialogResult (DialogResult::kCancel);
	frameworkDialog->close ();
	frameworkDialog = nullptr;
	return NO;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (void)windowDidResize:(NSNotification*)notification
{
	if(frameworkDialog)
		frameworkDialog->updateSize ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (void)windowDidMove:(NSNotification*)notification
{
	if(frameworkDialog)
		frameworkDialog->updateSize ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (void)windowDidBecomeKey:(NSNotification*)notification
{
	if(frameworkDialog)
		frameworkDialog->onActivate (true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (void)windowDidResignKey:(NSNotification*)notification
{
	if(frameworkDialog)
		frameworkDialog->onActivate (false);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (void)windowDidBecomeMain:(NSNotification*)notification
{
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (BOOL)windowShouldZoom:(NSWindow*)window toFrame:(NSRect)proposedFrame
{
	return NO;
}
@end

//************************************************************************************************
// Dialog
//************************************************************************************************

IAsyncOperation* Dialog::showPlatformDialog (IWindow* parent)
{
	ObservedPtr<IWindow> parentWindow (parent);
	if(DragSession* dragSession = DragSession::getActiveSession ())
		dragSession->setCanceled (true);

	repostMouseDown = false;

	bool isTranslucent = shouldBeTranslucent ();
	int windowMask = 0;
	
	if(style.isCustomStyle (Styles::kWindowBehaviorPopupSelector) && !style.isCustomStyle (Styles::kWindowAppearanceTitleBar))
		windowMask |= NSWindowStyleMaskBorderless;
	else
	{
		if(!isTranslucent && style.isCustomStyle (Styles::kWindowAppearanceTitleBar))
			windowMask |= NSWindowStyleMaskTitled;

		CCL::ProgressDialog* progress = unknown_cast<CCL::ProgressDialog> (getController ());
		bool canCancel = progress ? progress->isCancelEnabled () : true;

		if(!style.isCustomStyle (Styles::kWindowBehaviorPopupSelector) && canCancel)
			windowMask |= NSWindowStyleMaskClosable;
	}
	
	if(style.isCustomStyle (Styles::kWindowBehaviorSizable) || style.isCustomStyle (Styles::kWindowBehaviorMaximizable))
		windowMask |= NSWindowStyleMaskResizable;
	
	Rect size (getSize ());
	getSizeLimits().makeValid (size);
	moveWindowRectInsideScreen (size);
	
	NSRect bounds = NSMakeRect (0, 0, size.getWidth (), size.getHeight ());
	
	NSWindow* window = [[CCL_ISOLATED (PlatformWindow) alloc] initWithContentRect:bounds styleMask:windowMask];
	[window setHasShadow:YES];
	[window setAcceptsMouseMovedEvents:YES];
	[window setCollectionBehavior:NSWindowCollectionBehaviorFullScreenAuxiliary];
	id delegate = [[CCL_ISOLATED (DialogController) alloc] initWithFrameworkDialog:this];
	[window setDelegate:delegate];
	
	handle = window;
	
	if(style.isCustomStyle (Styles::kWindowBehaviorIntermediate))
		[window setLevel:NSFloatingWindowLevel - 1];
	else if(style.isCustomStyle (Styles::kWindowBehaviorFloating))
		[window setLevel:NSFloatingWindowLevel];
	else if(style.isCustomStyle (Styles::kWindowBehaviorPopupSelector))
	{
		// Popups should show at NSPopUpMenuWindowLevel, 
		// but not when kCGModalPanelWindowLevel is enough, to let them appear
		// on top of their parents, so that potential modal windows 
		// can appear on top of this new popup
		[window setLevel:NSPopUpMenuWindowLevel];
	
		PopupSelectorWindow* popup = ccl_cast<PopupSelectorWindow> (this);
		if(popup && popup->getParentWindow ())
			if(NSWindow* systemWindow = toNSWindow (popup->getParentWindow ()))
				if([systemWindow level] <= kCGModalPanelWindowLevel)
					[window setLevel:kCGModalPanelWindowLevel];
	}
	
	updateBackgroundColor ();
	
	if(!title.isEmpty ())
		setWindowTitle (title);
	
	ASSERT(nativeView == nullptr)
	Rect clientSize;
	getClientRect (clientSize);
	CustomView* contentView = NEW CustomView (this, clientSize);
	contentView->embedInto (window);
	nativeView = contentView;
	setWindowSize (size);
	{
		// init render target
		WindowGraphicsDevice initDevice (*this);
	}
			
	attached (this);

	initSize ();
	initFocusView ();
	show ();
	
	if(parentWindow)
		if(EditBox* editBox = unknown_cast<EditBox> (parentWindow->getFocusIView ()))
		   editBox->killFocus ();
	
	loopTerminated = 0;
	GUI.runModalLoop (this, loopTerminated);

	// inform parent window
	if(parentWindow)
		parentWindow->activate ();
	if(OSXWindow* osxWindow = OSXWindow::cast (unknown_cast<Window> (parentWindow)))
		osxWindow->setSuppressContextMenu (true);

	setFocusView (nullptr);
	[window setDelegate:nullptr];
	[delegate release];
	[window close];
	onDestroy ();
	
	nativeView->release ();
	nativeView = nullptr;
	
	Desktop.removeWindow (this);
	
	PopupSelectorWindow* popup = ccl_cast<PopupSelectorWindow> (this);
	if(popup && repostMouseDown)
	{
		NSEvent* current = [NSApp currentEvent];
		if([current type] == NSEventTypeRightMouseDown)
		{
			NSWindow* systemWindow = toNSWindow (popup->getParentWindow ());
			NSEvent* mouseDownEvent = [NSEvent mouseEventWithType:[current type] location:[systemWindow mouseLocationOutsideOfEventStream] modifierFlags:NSEventModifierFlagControl timestamp:0 windowNumber:[systemWindow windowNumber] context:nil eventNumber:0 clickCount:1 pressure:0];
			[NSApp postEvent:mouseDownEvent atStart:NO];
		}
	}
	
	if(inQuitEvent)  // repost stop event, because it got swallowed in runModal..
		[NSApp stop:nil];
	
	// return an AsyncOperation (already completed, since we ran modally)
	return AsyncOperation::createCompleted (dialogResult);
}

//************************************************************************************************
// PopupSelectorWindow
//************************************************************************************************

void PopupSelectorWindow::onActivate (bool state)
{
	CCL_PRINTF ("PopupSelectorWindow::onActivate %d\n", state)

	if(!state)
	{
		bool swallow = onPopupDeactivated ();
		repostMouseDown = !swallow;
	}
	SuperClass::onActivate (state);
}

//************************************************************************************************
// OSXDialog
//************************************************************************************************

OSXDialog::OSXDialog (const Rect& size, StyleRef style, StringRef title)
: OSXWindow (size, style, title),
  loopTerminated (false),
  repostMouseDown (false)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API OSXDialog::close ()
{
	if(onClose ())
	{
		hide ();
		setInCloseEvent (true);
		setInDestroyEvent (true);
		
		removed (0);
		onDestroy ();
		loopTerminated = true;
		setInCloseEvent (false);
		return true;
	}
	return false;
}
