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
// Filename    : ccl/platform/cocoa/gui/window.ios.mm
// Description : platform-specific window implementation
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "ccl/platform/cocoa/gui/window.ios.h"
#include "ccl/platform/cocoa/gui/nativeview.ios.h"
#include "ccl/gui/gui.h"
#include "ccl/gui/windows/desktop.h"
#include "ccl/gui/graphics/nativegraphics.h"
#include "ccl/platform/cocoa/quartz/cghelper.h"

#include "ccl/platform/cocoa/cclcocoa.h"

#include "ccl/public/base/ccldefpush.h"

using namespace CCL;

//************************************************************************************************
// WindowViewController
//************************************************************************************************

@interface CCL_ISOLATED (WindowViewController) : UIViewController
{
	IOSWindow* window;
	UIViewController* parentController;
}

- (id)initWithWindow:(IOSWindow*)window parent:(UIViewController*)parent;
- (void)viewWillLayoutSubviews;
- (void)viewSafeAreaInsetsDidChange;
- (void)show;
- (void)hide;
- (void)close;

@end

#define VIEWCONTROLLER ((CCL_ISOLATED (WindowViewController)*)handle)

//************************************************************************************************
// WindowViewController
//************************************************************************************************

@implementation CCL_ISOLATED (WindowViewController)

- (id)initWithWindow:(IOSWindow*)_window parent:(UIViewController*)parent
{
	if(self = [super init])
	{
		window = _window;
		parentController = parent;
	}
	return self;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (void)viewWillLayoutSubviews
{
	if(!window)
		return;

	window->updateSize ();
}

////////////////////////////////////////////////////////////////////////////////////////////////////

- (void)viewSafeAreaInsetsDidChange
{
	[super viewSafeAreaInsetsDidChange];

	if(window)
		window->updateSize ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (void)show
{
	if(!window)
		return;
		
	if(window->getStyle ().isCustomStyle (Styles::kWindowBehaviorSheetStyle))
		[parentController showViewController:self sender:nil];
	else
		[self.view setHidden:NO];
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (void)hide
{
	if(!window)
		return;
		
	if(window->getStyle ().isCustomStyle (Styles::kWindowBehaviorSheetStyle))
		[parentController dismissViewControllerAnimated:YES completion:nil];
	else
		[self.view setHidden:YES];
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (void)close
{
	window = nil;
}

@end

//************************************************************************************************
// IOSWindow
//************************************************************************************************

IOSWindow::IOSWindow (const Rect& size, StyleRef style, StringRef title)
: Window (size, style, title),
  nativeView (nil)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

IOSWindow::~IOSWindow ()
{
	destruct ();
	
	if(nativeView)
		nativeView->release ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API IOSWindow::getProperty (Variant& var, MemberID propertyId) const
{
	if(propertyId == IWindow::kStatusBarStyle)
	{
		var = statusBarStyle;
		return true;
	}
	return Window::getProperty (var, propertyId);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API IOSWindow::setProperty (MemberID propertyId, const Variant& var)
{
	if(propertyId == IWindow::kStatusBarStyle)
	{
		statusBarStyle = var;
		UIWindow* window = (UIWindow*)[[[UIApplication sharedApplication] windows] objectAtIndex:0];
		[window.rootViewController setNeedsStatusBarAppearanceUpdate];
		return true;
	}
	return Window::setProperty (propertyId, var);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void* IOSWindow::getTopViewController () const
{
	UIViewController* viewController = VIEWCONTROLLER;
    
    while([viewController presentedViewController])
        viewController = [viewController presentedViewController];

    return (void*)viewController;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void IOSWindow::makeNativePopupWindow (IWindow* parent)
{
	UIViewController* parentViewController = nil;
	
	if(!parent)
		parent = Desktop.getDialogParentWindow ();
	
	if(parent)
		parentViewController = (UIViewController*)parent->getSystemWindow ();
	
	CCL_ISOLATED (ContentView)* content = [[[CCL_ISOLATED (ContentView) alloc] init] autorelease];
	[content setWindow:this];
	
	UIViewController* controller = [[CCL_ISOLATED (WindowViewController) alloc] initWithWindow:this parent:parentViewController];
	[controller setView:content];

	if(getStyle ().isCustomStyle (Styles::kWindowBehaviorSheetStyle))
	{
		controller.modalPresentationStyle = UIModalPresentationFormSheet;
		controller.modalInPopover = YES;
	}
	else
		[parentViewController.view addSubview:content];
		
	initSize ();
	
    handle = controller; // needed in setWindowSize
	nativeView = NEW NativeView (content);
	
	applySafeAreaInsetsToChild (getSize ()); // respect safe area insets
	
	// init render target
	renderTarget = Window::getRenderTarget ();

    Rect contentSize = getSize ();
	setWindowSize (contentSize);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void IOSWindow::setWindowSize (Rect& newSize)
{
	if(!handle || !nativeView)
		return;
	
	if(isResizing ())
		return;

	if(isInDestroyEvent ())
		return;

	if([VIEWCONTROLLER isBeingPresented])
	{
		UIView* view = nativeView->getView ();
		CGSize wantedSize = CGSizeMake (newSize.getWidth (), newSize.getHeight ());
		CGSize actualSize = view.bounds.size;
		if(!CGSizeEqualToSize (wantedSize, view.bounds.size))
		{
			CCL_PRINTF ("contentView request new frame : left=%d top=%d width=%d height=%d\n", newSize.left, newSize.top, newSize.getWidth (), newSize.getHeight ())
			VIEWCONTROLLER.preferredContentSize = wantedSize;
		}
	}
	else
	{
		CGRect cgSize;
		MacOS::toCGRect (cgSize, newSize);
		[nativeView->getView () setFrame:cgSize];
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void IOSWindow::invalidate (RectRef rect)
{
	if(!nativeView || isInDestroyEvent ())
		return;
	
	if(NativeWindowRenderTarget* target = getRenderTarget ())
		if(IMutableRegion* region = target->getInvalidateRegion ())
			region->addRect (rect);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void IOSWindow::showWindow (bool state)
{
	if(!handle)
		return;
	
	if(getStyle ().isCustomStyle (Styles::kWindowBehaviorSheetStyle))
	{
		if(state)
			[VIEWCONTROLLER show];
		else
			[VIEWCONTROLLER hide];
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API IOSWindow::close ()
{
	if(!handle || !nativeView || isInCloseEvent () || isInDestroyEvent ())
		return false;
	
	if(onClose ())
	{
		cancelDragSession ();

		setInCloseEvent (true);
		setInDestroyEvent (true);
		showWindow (false);
		
		onDestroy ();
		AutoPtr<IOSWindow> releaser = this;
		
		CCL_ISOLATED (ContentView)* view = nativeView->getView ();
		[view setWindow:nil];
		[view removeFromSuperview];
			
		[VIEWCONTROLLER close];
		[VIEWCONTROLLER release];
	}
	
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void IOSWindow::getFrameSize (Rect& size) const
{
	if(!handle || !nativeView)
		return;
	
	CGRect frame = [nativeView->getView () frame];
	MacOS::fromCGRect (size, frame);	
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL::Point& CCL_API IOSWindow::clientToScreen (CCL::Point& pos) const
{
	Point origin;
	screenToClient (origin); // result is negative
	pos.x -= origin.x;
	pos.y -= origin.y;
	return pos;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL::Point& CCL_API IOSWindow::screenToClient (CCL::Point& pos) const
{
	pos -= size.getLeftTop ();
	return pos;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

float IOSWindow::getContentScaleFactor () const
{
	return static_cast<float>([[UIScreen mainScreen] scale]);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API IOSWindow::scrollClient (RectRef rect, PointRef delta)
{
	if(!nativeView || inDestroyEvent)
		return;

	if(collectUpdates)
	{
		// don't scroll, just invalidate
		Rect r (rect);
		r.offset (delta);
		r.join (rect);
		invalidate (r);
		return;
	}
	
	// inform render target
	if(NativeWindowRenderTarget* target = getRenderTarget ())
	{
		target->onScroll (rect, delta);
		finishScroll (rect, delta);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API IOSWindow::redraw ()
{
	if(!nativeView)
		return;
	
	if(NativeWindowRenderTarget* target = getRenderTarget ())
		target->onRender ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API IOSWindow::center ()
{
	Rect screen;
	MacOS::fromCGRect (screen, [[UIScreen mainScreen] bounds]);
	Rect size (getSize ());
	size.center (screen);
	setSize (size);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void IOSWindow::updateBackgroundColor ()
{
	if(!handle || !nativeView)
		return;
	
	if(UIView* view = nativeView->getView ())
	{
		bool isTranslucent = shouldBeTranslucent ();
		Color c = getVisualStyle ().getBackColor ();
		if(isTranslucent)
			c.setAlphaF (0);
		UIColor* uiColor = [UIColor colorWithRed:c.getRedF() green:c.getGreenF () blue:c.getBlueF () alpha:c.getAlphaF ()];
		[view setBackgroundColor:uiColor];
		[view setOpaque: isTranslucent ? NO : YES];
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void IOSWindow::updateSize ()
{
	if(!handle || !nativeView)
		return;

	UIView* view = nativeView->getView ();
    CGRect frame = view.frame;
	CGRect bounds = view.bounds;
	Rect rect ((Coord)frame.origin.x, (Coord)frame.origin.y, Point ((Coord)bounds.size.width, (Coord)bounds.size.height));
	CCL_PRINTF ("IOSWindow::updateSize: %d, %d, %d x %d\n", rect.left, rect. top, rect.getWidth (), rect.getHeight ())
	if(size.isEmpty ())
        return;

    ScopedFlag<kAttachDisabled> scope (sizeMode); // don't size child(s) automatically, this is done explicitely below
	View::setSize (rect);
    
    applySafeAreaInsetsToChild (rect);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void IOSWindow::applySafeAreaInsetsToChild (CCL::RectRef windowSize)
{
	if(!handle || !nativeView)
		return;

	UIView* view = nativeView->getView ();
	
    UIEdgeInsets safeAreaInsets = view.safeAreaInsets;
    CCL_PRINTF ("safeAreaInsets: %f, %f, %f, %f\n", safeAreaInsets.left, safeAreaInsets.top, safeAreaInsets.right, safeAreaInsets.bottom)

    // size child view (expecting only one) according to safe area insets (e.g. notch on iPhone); window displays background beyond
    ASSERT (views.count () <= 1)

    CCL::Rect size;
    size.left = (Coord)safeAreaInsets.left;
    size.top = (Coord)safeAreaInsets.top;
    size.setWidth ((Coord)(windowSize.getWidth () - safeAreaInsets.left - safeAreaInsets.right));
    size.setHeight ((Coord)(windowSize.getHeight () - safeAreaInsets.top - safeAreaInsets.bottom));

    ScopedFlag<kResizing> flagSetter (privateFlags);
    if(View* child = getFirst ())
        child->setSize (size);
}

//************************************************************************************************
// IOSDialog
//************************************************************************************************

DEFINE_CLASS (IOSDialog, IOSWindow)
