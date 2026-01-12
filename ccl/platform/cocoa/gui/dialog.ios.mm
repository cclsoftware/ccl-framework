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
// Filename    : ccl/platform/cocoa/gui/dialog.ios.mm
// Description : platform-specific dialog implementation
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "ccl/gui/windows/dialog.h"
#include "ccl/gui/windows/desktop.h"
#include "ccl/gui/touch/touchinput.h"
#include "ccl/gui/controls/popupbox.h"
#include "ccl/gui/graphics/nativegraphics.h"
#include "ccl/gui/views/imageview.h"

#include "ccl/base/asyncoperation.h"
#include "ccl/base/signalsource.h"

#include "ccl/platform/cocoa/iosapp/contentview.h"
#include "ccl/platform/cocoa/iosapp/appdelegate.h"
#include "ccl/platform/cocoa/quartz/cghelper.h"
#include "ccl/platform/cocoa/gui/popoverbackgroundview.ios.h"
#include "ccl/platform/cocoa/gui/nativeview.ios.h"

#include "ccl/public/gui/framework/controlsignals.h"

#include "ccl/public/base/ccldefpush.h"

using namespace CCL;

#define VIEWCONTROLLER ((UIViewController*)handle)

//************************************************************************************************
// DialogViewController
//************************************************************************************************

@interface CCL_ISOLATED (DialogViewController) : UIViewController<UIPopoverPresentationControllerDelegate>
{
	CCL::SharedPtr<CCL::Dialog> dialog;
	CCL::SharedPtr<CCL::AsyncOperation> dialogOperation;
	bool viewAppeared;
}

- (void)closeDialog;
- (BOOL)isClosing;

@end

//************************************************************************************************
// DialogViewController
//************************************************************************************************

@implementation CCL_ISOLATED (DialogViewController)

//////////////////////////////////////////////////////////////////////////////////////////////////

- (id)initWithDialog:(CCL::Dialog*)_dialog operation:(CCL::AsyncOperation*)_dialogOperation
{
	if(self = [super init])
	{
		// share the Dialog to keep it it alive (non-modal)
		dialog = _dialog;
		dialogOperation = _dialogOperation;
		viewAppeared = false;
	}
	return self;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (void)dismissViewControllerAnimated:(BOOL)flag completion:(void (^)(void))completion
{
	if(viewAppeared)
	{
		[super dismissViewControllerAnimated:flag completion:completion];
		return;
	}

	if(dialogOperation && dialog)
		dialogOperation->setResult (dialog->getDialogResult ());

	void (^completionInternal)(void) = ^{
		if(dialogOperation)
			dialogOperation->setState (AsyncOperation::kCompleted);

		[self closeDialog];

		if(completion)
			completion ();
	};

	[super dismissViewControllerAnimated:flag completion:completionInternal];
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (void)closeDialog
{
	if(dialog && !dialog->isInCloseEvent ())
	{
		dialog->setInDestroyEvent (true);
		dialog->onActivate (false);
		if(!dialog->isInCloseEvent ())
		{
			dialog->close ();
			dialog->removed (0);
			dialog->onDestroy ();
			dialog.release ();
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (BOOL)isClosing
{
	if(dialog && (dialog->isInCloseEvent () || dialog->isInDestroyEvent ()))
		return YES;
	else
		return NO;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (void)viewDidAppear:(BOOL)animated
{
	viewAppeared = true;
	[super viewDidAppear:animated];
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (void)viewDidDisappear:(BOOL)animated
{
	if(dialogOperation && dialog)
	{
		dialogOperation->setResult (dialog->getDialogResult ());
		dialogOperation->setState (AsyncOperation::kCompleted);
	}

	[self closeDialog];
	[super viewDidDisappear:animated];
}

////////////////////////////////////////////////////////////////////////////////////////////////////

- (void)popoverPresentationController:(UIPopoverPresentationController*)popoverPresentationController willRepositionPopoverToRect:(inout CGRect*)rect inView:(inout UIView**)view
{
	if(dialog)
	{
		CCL::Rect size = dialog->getSize ();
		if(dialog->getStyle ().isCustomStyle (Styles::kWindowBehaviorCenter))
		{
			CGRect screenSize = [[UIScreen mainScreen] bounds];
			rect->origin.x = screenSize.size.width / 2;
			rect->origin.y = (screenSize.size.height - size.getHeight ()) / 2;
		}
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////

- (UIModalPresentationStyle)adaptivePresentationStyleForPresentationController:(UIPresentationController*)controller traitCollection:(UITraitCollection*)traitCollection
{
	return UIModalPresentationNone;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

- (void)viewWillLayoutSubviews
{
	if(dialog)
	{
		dialog->updateSize ();

		// get rid of rounded corners by disabling the mask in superlayers
		CALayer* layer = self.view.layer;
		while(layer)
		{
			[layer setMasksToBounds:NO];
			layer = layer.superlayer;
		}
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////

- (void)viewSafeAreaInsetsDidChange
{
	[super viewSafeAreaInsetsDidChange];
	
	if(dialog)
		dialog->updateSize ();
}

@end

//************************************************************************************************
// Dialog
//************************************************************************************************

IAsyncOperation* Dialog::showPlatformDialog (IWindow* parent)
{
	collectUpdates = true;
		
	bool isTranslucent = shouldBeTranslucent () || getStyle ().isCustomStyle (Styles::kWindowAppearanceCustomFrame);

	Window* parentWindow = nil;
	UIViewController* parentViewController = nil;

	if(parent)
	{
		parentViewController = (UIViewController*)parent->getSystemWindow ();
		parentWindow = unknown_cast<Window> (parent);
	}
	if(!parentViewController)
		if(parentWindow = unknown_cast<Window> (Desktop.getDialogParentWindow ()))
			parentViewController = (UIViewController*)(parentWindow->getSystemWindow ());
	if(!parentViewController)
		parentViewController = [[[[UIApplication sharedApplication] windows] objectAtIndex:0] rootViewController];
	
	while([parentViewController presentedViewController] != nil)
		parentViewController = [parentViewController presentedViewController];
	
	if([parentViewController isKindOfClass:[CCL_ISOLATED (DialogViewController) class]])
		if([(CCL_ISOLATED (DialogViewController)*)parentViewController isClosing])
			return nullptr;

	initSize ();
	Rect contentSize = getSize ();
	bool isSheetStyle = getStyle ().isCustomStyle (Styles::kWindowBehaviorSheetStyle);
	if(isSheetStyle && popupSizeInfo.parent)
		contentSize.setSize (Rect (popupSizeInfo.sizeLimits).getRightBottom ());

	CCL_PRINTF ("contentView request frame : left=%d top=%d width=%d height=%d\n", contentSize.left, contentSize.top, contentSize.getWidth (), contentSize.getHeight ())

	// Store the initial position to be restored after the controls are added
	CCL::Point initialPos = contentSize.getLeftTop ();
	contentSize.moveTo (CCL::Point ()); // origin is used below in sourceRect

	bool isPopupStyle = style.isCustomStyle (Styles::kWindowBehaviorPopupSelector);
	if(!isPopupStyle)
		setSize (contentSize);
	CGRect frame;
	MacOS::toCGRect (frame, contentSize);
	CCL_ISOLATED (ContentView)* contentView = [[[CCL_ISOLATED (ContentView) alloc] initWithFrame:frame] autorelease];
	if(isTranslucent)
		[contentView setOpaque:NO];
	[contentView setWindow:this];
	nativeView = NEW NativeView (contentView);
	
	renderTarget = Window::getRenderTarget ();
	
	AsyncOperation* operation = NEW AsyncOperation;
	operation->setState (AsyncOperation::kStarted);
	
	CCL_ISOLATED (DialogViewController)* dialogController = [[CCL_ISOLATED (DialogViewController) alloc] initWithDialog:this operation:operation];
	dialogController.view = contentView;
	handle = dialogController;

	attached (0);

	if(getStyle ().isCustomStyle (Styles::kWindowBehaviorSheetStyle))
		dialogController.modalPresentationStyle = UIModalPresentationFormSheet;
	else
		dialogController.modalPresentationStyle = UIModalPresentationPopover;

	dialogController.preferredContentSize = frame.size;
	UIPopoverPresentationController* presentationController = [dialogController popoverPresentationController];
	presentationController.delegate = dialogController;

	CGRect anchorRect;
	auto popup = ccl_cast<PopupSelectorWindow> (this);
	// suppress close by tapping outside dialog
	if(popup)
		dialogController.modalInPopover = NO;
	else
		dialogController.modalInPopover = YES;

	if(popup && popup->getAnchorRect () != Rect ().setReallyEmpty () && !isTranslucent)
	{
		presentationController.permittedArrowDirections = UIPopoverArrowDirectionAny;
		// use anchor Rect for positioning, display arrow (aka "nose") pointing to anchor
		Rect anchor;
		anchor = popup->getAnchorRect ();
		Point p;
		anchor.offset (popup->clientToWindow (p));
		CCL_PRINTF ("anchorRect : left=%d top=%d width=%d height=%d\n", anchor.left, anchor.top, anchor.getWidth (), anchor.getHeight ())
		MacOS::toCGRect (anchorRect, anchor);
		
		// the anchor rect plus dialog must have enough space in the window, else the dialog will be shown invisibly
		CGRect fullArea = parentViewController.view.bounds;
		anchorRect = CGRectIntersection (anchorRect, fullArea);
		ccl_upper_limit (anchorRect.size.width, fullArea.size.width - frame.size.width);
		ccl_upper_limit (anchorRect.size.height, fullArea.size.height - frame.size.height);
	}
	else
	{
		// use size for positioning, do not display an arrow (aka "nose")
		Rect currentSize (getSize ());

		// Restore the initial position if this is not a popup
		if(!isPopupStyle)
			currentSize.offset (initialPos);

		if(parentWindow)
		{
			// our window position is in screen coords, but sourceRect is relative to parentViewController's view
			Point pos;
			pos -= parentWindow->getSize ().getLeftTop ();
			currentSize.offset (pos);
		}
		CGRect sourceRect;
		MacOS::toCGRect (sourceRect, currentSize);
		CCL_PRINTF ("no anchorRect, size : left=%d top=%d width=%d height=%d\n", size.left, size.top, size.getWidth (), size.getHeight ())
		anchorRect = CGRectMake (CGRectGetMidX (sourceRect), CGRectGetMinY (sourceRect), 1.0, 1.0);
		presentationController.permittedArrowDirections = UIPopoverArrowDirectionUp;
		if(isTranslucent)
			presentationController.popoverBackgroundViewClass = [CCL_ISOLATED (TransparentPopoverBackgroundView) class];
		else
			presentationController.popoverBackgroundViewClass = [CCL_ISOLATED (OpaquePopoverBackgroundView) class];
	}

	presentationController.sourceView = parentViewController.view;
	presentationController.sourceRect = anchorRect;

	initFocusView ();
	
	[parentViewController presentViewController:dialogController animated:YES completion:nil];

	return operation;
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
		
		if(Window* parentWindow = unknown_cast<Window> (getParentWindow ()))
			parentWindow->getTouchInputState ().discardTouches (true);
	}
	SuperClass::onActivate (state);
}

//************************************************************************************************
// IOSDialog
//************************************************************************************************

IOSDialog::IOSDialog (const Rect& size, StyleRef style, StringRef title)
: IOSWindow (size, style, title),
  loopTerminated (false),
  repostMouseDown (false),
  updateSizeCalled (false),
  popupSizeInfo (Point ())
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API IOSDialog::close ()
{
	if(!handle || !nativeView)
		return true;

	if(onClose ())
	{
		cancelDragSession ();

		setInCloseEvent (true);
		setInDestroyEvent (true);

		[VIEWCONTROLLER dismissViewControllerAnimated:YES completion:nil];

		removed (0);
		onDestroy ();

		Desktop.removeWindow (this);
		
		setInCloseEvent (false);

		auto contentView = nativeView->getView ();
		if(contentView)
			[contentView setWindow:nil];

		SharedPtr<IUnknown> holder (this->asUnknown ()); // dialog might be released and destroyed by the next line of code
		[VIEWCONTROLLER release];
		handle = nullptr;
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void IOSDialog::updateSize ()
{
	if(!handle || !nativeView)
		return;
		
	updateSizeCalled = true;

	UIView* view = nativeView->getView ();
	CGRect bounds = view.bounds;
	CCL_PRINTF ("IOSDialog::updateSize: %f x %f\n", bounds.size.width, bounds.size.height)
	if(size.isEmpty ())
		return;

	Rect rect = getSize ();
	rect.setWidth (bounds.size.width);
	rect.setHeight (bounds.size.height);
	rect.moveTo (Point (bounds.origin.x, bounds.origin.y));
	ScopedFlag<kAttachDisabled> scope (sizeMode); // don't size child(s) automatically, this is done explicitely below
	View::setSize (rect);
	
	applySafeAreaInsetsToChild (rect);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void IOSDialog::setWindowSize (Rect& newSize)
{
	if(!handle || !nativeView)
		return;

	UIPopoverPresentationController* presentationController = [VIEWCONTROLLER popoverPresentationController];
	if(!presentationController)
		return IOSWindow::setWindowSize (newSize);

	if(isResizing ())
		return;

	ASSERT (presentationController.permittedArrowDirections == UIPopoverArrowDirectionUp) // setWindowSize does not work with an anchorRect, see Dialog::showPlatformDialog

	UIView* view = nativeView->getView ();
	CGSize wantedSize = CGSizeMake (newSize.getWidth (), newSize.getHeight ());
	CGSize actualSize = view.bounds.size;
	if(!CGSizeEqualToSize (wantedSize, view.bounds.size))
	{
		CCL_PRINTF ("contentView request new size : width=%d height=%d\n", newSize.getWidth (), newSize.getHeight ())
		VIEWCONTROLLER.preferredContentSize = wantedSize;
	}

	Rect relativeSize (newSize);

	if(auto* popupSelector = ccl_cast<PopupSelectorWindow> (this))
	{
		if(Window* parent = unknown_cast<Window> (popupSelector->getParentWindow ()))
		{
			Point pos;
			pos -= parent->getSize ().getLeftTop ();
			relativeSize.offset (pos);
		}
	}

	CGRect sourceRect;
	MacOS::toCGRect (sourceRect, relativeSize);
	CGRect anchorRect = CGRectMake (CGRectGetMidX (sourceRect), CGRectGetMinY (sourceRect), 1.0, 1.0);

	if(!CGRectEqualToRect (anchorRect, presentationController.sourceRect))
	{
		CCL_PRINTF ("contentView request new position : left=%d top=%d\n", newSize.left, newSize.top)
		presentationController.sourceRect = anchorRect;
	}
}
