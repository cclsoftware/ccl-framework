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
// Filename    : ccl/platform/cocoa/gui/window.mac.mm
// Description : platform-specific window implementation
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "ccl/gui/gui.h"
#include "ccl/gui/windows/popupwindow.h"
#include "ccl/gui/windows/desktop.h"
#include "ccl/gui/system/systemtimer.h"

#include "ccl/base/storage/configuration.h"
#include "ccl/base/message.h"

#include "ccl/platform/cocoa/gui/menu.cocoa.h"
#include "ccl/platform/cocoa/quartz/device.h"
#include "ccl/platform/cocoa/quartz/nshelper.h"
#include "ccl/platform/cocoa/gui/nativeview.mac.h"
#include "ccl/platform/cocoa/macutils.h"
#include "ccl/platform/cocoa/quartz/quartzbitmap.h"

#include "platformwindow.mac.h"

using namespace CCL;

static const Configuration::BoolValue transparentTitlebarConfiguration ("CCL.OSX", "TransparentTitlebar", false);
static const Configuration::BoolValue coloredTitlebarConfiguration ("CCL.OSX", "ColoredTitlebar", false);

#define VIEW (nativeView ? nativeView->getView () : nil)
#define WINDOW (MacOS::toNSWindow (this))

@implementation CCL_ISOLATED (ContentView)

- (BOOL)isFlipped
{
    return YES;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (BOOL)acceptsFirstResponder
{
    return NO;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (BOOL)acceptsFirstMouse:(NSEvent*)nsEvent
{
    return NO;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (void)setFrameSize:(NSSize)newSize;
{
	[super setFrameSize:newSize];
	if([[self subviews] count] > 0)
		[[[self subviews] objectAtIndex:0] setFrame:NSMakeRect (0, 0, [self bounds].size.width, [self bounds].size.height)];
}

	
//////////////////////////////////////////////////////////////////////////////////////////////////

- (BOOL)performKeyEquivalent:(NSEvent*)event
{
	for(NSView* subView in [[[self subviews] reverseObjectEnumerator] allObjects])
		if([subView performKeyEquivalent:event])
			return YES;

	return [super performKeyEquivalent:event];
}

@end

//////////////////////////////////////////////////////////////////////////////////////////////////

namespace CCL {
namespace MacOS {

static void translateWindowStyle (NSWindowStyleMask& windowMask, StyleRef style)
{
	if(!style.isCustomStyle (Styles::kWindowAppearanceCustomFrame))
	{
		if(style.isCustomStyle (Styles::kWindowAppearanceTitleBar))
			windowMask |= NSWindowStyleMaskTitled;
	
		windowMask |= NSWindowStyleMaskClosable | NSWindowStyleMaskMiniaturizable;
		
		if(style.isCustomStyle (Styles::kWindowBehaviorFloating) || style.isCustomStyle (Styles::kWindowBehaviorIntermediate))
			windowMask |= NSWindowStyleMaskUtilityWindow | NSWindowStyleMaskHUDWindow;
	}
	else if(style.isCustomStyle (Styles::kWindowAppearanceRoundedCorners))
		windowMask |= NSWindowStyleMaskFullSizeContentView | NSWindowStyleMaskTitled;

	if(style.isCustomStyle (Styles::kWindowBehaviorSizable) || style.isCustomStyle (Styles::kWindowBehaviorMaximizable))
		windowMask |= NSWindowStyleMaskResizable;
}

} // namespace MacOS
} // namespace CCL

//************************************************************************************************
// OSXWindow
//************************************************************************************************

OSXWindow::OSXWindow (const Rect& size, StyleRef style, StringRef title)
: Window (size, style, title),
  delegate (nil),
  nativeView (nil),
  suppressContextMenu (false),
  fullscreen (false),
  activating (false),
  savedOpacity (1)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

OSXWindow::~OSXWindow ()
{
	destruct ();
	if(nativeView)
		nativeView->release ();

	if(delegate)
	{
	    [[NSNotificationCenter defaultCenter] removeObserver:(id)delegate];
		[(id)delegate release];
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void OSXWindow::makeNativePopupWindow (IWindow* parent)
{
	NSWindowStyleMask windowMask = NSWindowStyleMaskBorderless;
	MacOS::translateWindowStyle (windowMask, style);

	Rect size (getSize ());
	moveWindowRectInsideScreen (size);

	NSRect bounds = NSMakeRect (0, 0, size.getWidth (), size.getHeight ());

	NSWindow* window = nil;
	if(style.isCustomStyle (Styles::kWindowBehaviorTooltip))
		window = [[CCL_ISOLATED (PlatformTooltip) alloc] initWithContentRect:bounds styleMask:windowMask];
	else if(style.isCustomStyle (Styles::kWindowBehaviorFloating) || style.isCustomStyle (Styles::kWindowBehaviorIntermediate))
	{
		window = [[CCL_ISOLATED (PlatformPanel) alloc] initWithContentRect:bounds styleMask:windowMask];
		
		int floatingLevel = NSFloatingWindowLevel;
		if(style.isCustomStyle (Styles::kWindowBehaviorIntermediate))
			floatingLevel -= 1;
			
		[(CCL_ISOLATED (PlatformPanel)*)window setFloatingLevel:floatingLevel];
		[(CCL_ISOLATED (PlatformPanel)*)window makeFloat];
	}
	else
		window = [[CCL_ISOLATED (PlatformWindow) alloc] initWithContentRect:bounds styleMask:windowMask];
		
	[window setHidesOnDeactivate:NO];	
	[window setHasShadow:shouldBeTranslucent () ? NO : YES];
	[window setAcceptsMouseMovedEvents:YES];
	delegate = [[CCL_ISOLATED (WindowController) alloc] initWithFrameworkWindow:this];
	[window setDelegate:(CCL_ISOLATED (WindowController)*) delegate];
	[window setReleasedWhenClosed:YES];
    [window setMovableByWindowBackground:NO];
	
	NSWindowCollectionBehavior windowBehavior = 0;
    if(style.isCustomStyle (Styles::kWindowBehaviorProgressDialog) || style.isCustomStyle (Styles::kWindowBehaviorTooltip) || style.isCustomStyle (Styles::kWindowBehaviorPopupSelector))
		windowBehavior |= NSWindowCollectionBehaviorTransient;
	else if(style.isCustomStyle (Styles::kWindowBehaviorFloating) || style.isCustomStyle (Styles::kWindowBehaviorIntermediate))
	{
		windowBehavior |= NSWindowCollectionBehaviorFullScreenAuxiliary;
		windowBehavior |= NSWindowCollectionBehaviorIgnoresCycle;
		windowBehavior |= NSWindowCollectionBehaviorManaged;
	}
	else
	{
		windowBehavior |= NSWindowCollectionBehaviorFullScreenPrimary;
		windowBehavior |= NSWindowCollectionBehaviorManaged;
	}
	[window setCollectionBehavior:windowBehavior];

	if(style.isCustomStyle (Styles::kWindowBehaviorProgressDialog) || style.isCustomStyle (Styles::kWindowBehaviorTooltip))
		[window setLevel:NSPopUpMenuWindowLevel];
	
	if(style.isCustomStyle (Styles::kWindowBehaviorTopMost))
		[(CCL_ISOLATED (PlatformWindow)*)window setLevel:NSFloatingWindowLevel];
	
	handle = window;
	
	updateBackgroundColor ();
	
	Rect clientSize;
	getClientRect (clientSize);
	ASSERT(nativeView == nil)
	CustomView* customView = NEW CustomView (this, clientSize);

	customView->embedInto (window);
	nativeView = customView;
	
	// init render target
	renderTarget = Window::getRenderTarget ();

	setWindowSize (size);
	setWindowTitle (title);

	if(style.isCustomStyle (Styles::kWindowAppearanceCustomFrame) && style.isCustomStyle (Styles::kWindowAppearanceRoundedCorners))
		suppressTitleBar ();

	// use an intermediate content view to allow subviews for 3rd party plug-ins to be without layer
	NSView* contentView = [[CCL_ISOLATED (ContentView) alloc] init];
	[contentView setAutoresizesSubviews:NO];
	[window setContentView:contentView];
	[contentView release];
	customView->embedInto (contentView);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void OSXWindow::makeNativeChildWindow (void* nativeParent)
{
	ASSERT(nativeView == nil)
	CustomView* contentView = NEW CustomView (this, getSize ());
	nativeView = contentView;
	
	if(nativeParent)
	{
		ASSERT([(id)nativeParent isKindOfClass:[NSView class]])
		NSView* parentView = (NSView*)nativeParent;
		parentView.clipsToBounds = YES;
		handle = (void*)[parentView window];
		contentView->embedInto (parentView);
		
		delegate = [[CCL_ISOLATED (WindowController) alloc] initWithFrameworkWindow:this];
		[[NSNotificationCenter defaultCenter] addObserver:(id)delegate selector:@selector(windowDidChangeBackingProperties:) name:NSWindowDidChangeBackingPropertiesNotification object:[parentView window]];
		[[NSNotificationCenter defaultCenter] addObserver:(id)delegate selector:@selector(windowDidBecomeKey:) name:NSWindowDidBecomeKeyNotification object:[parentView window]];
		[[NSNotificationCenter defaultCenter] addObserver:(id)delegate selector:@selector(windowDidResignKey:) name:NSWindowDidResignKeyNotification object:[parentView window]];
		[[NSNotificationCenter defaultCenter] addObserver:(id)delegate selector:@selector(windowDidResize:) name:NSWindowDidResizeNotification object:[parentView window]];
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API OSXWindow::getProperty (Variant& var, MemberID propertyId) const
{
	if(propertyId == kSystemView && nativeView)
	{
		var.setIntPointer (reinterpret_cast<UIntPtr> (nativeView->getView ()));
		return true;
	}
	
	return Window::getProperty (var, propertyId);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API OSXWindow::setProperty (MemberID propertyId, const Variant& var)
{
	if(propertyId == kRepresentedFile)
	{
		if(!isChildWindow ())
		{
			UnknownPtr<IUrl> path (var.asUnknown ());
			NSURL* nsFileUrl = nil;
			if(path)
			{
				nsFileUrl = [NSURL alloc];
				MacUtils::urlToNSUrl (*path, nsFileUrl);
			}
			[WINDOW setRepresentedURL:nsFileUrl];
			return true;
		}
	}
	else if(propertyId == kDocumentDirty)
	{
		if(!isChildWindow ())
		{
			[WINDOW setDocumentEdited:var.asBool ()];
			return true;
		}
	}
	return Window::setProperty (propertyId, var);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void OSXWindow::invalidate (RectRef rect)
{
	if(!nativeView || inDestroyEvent)
		return;
	
	if(NativeWindowRenderTarget* target = getRenderTarget ())
		if(IMutableRegion* region = target->getInvalidateRegion ())
			region->addRect (rect);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void OSXWindow::updateMenuBar ()
{
	if(isChildWindow ())
		return;
	
	if(menuBar)
		CocoaMenuBar::cast (menuBar)->activatePlatformMenu ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void OSXWindow::setWindowSize (Rect& newSize)
{
	if(newSize.getWidth () == 0)
		newSize.setWidth (1);
		
	if(newSize.getHeight () == 0)
		newSize.setHeight (1);
	
	View::setSize (newSize);

	if(!handle || (privateFlags & kInUpdateSize) || isInDestroyEvent () != 0)
		return;
	
	ScopedFlag<kInUpdateSize> resizingScope (privateFlags);
	
	if(isChildWindow ())
	{
		CCL_PRINTF ("ChildWindow::setWindowSize %d %d %d %d (%d x %d)\n", newSize.left, newSize.top, newSize.right, newSize.bottom, newSize.getWidth (), newSize.getHeight ())
		NSRect newFrame (NSZeroRect);
		newFrame.size = [VIEW bounds].size;
		newFrame.origin.x = newSize.left;
		newFrame.origin.y = newSize.top;
		newFrame.size.width = newSize.getWidth ();
		newFrame.size.height = newSize.getHeight ();
		[VIEW setFrame:newFrame];
		return;
	}
	
	CCL::Rect size (newSize);
	moveWindowRectInsideScreen (size);
	
	CCL_PRINTF ("%s::setWindowSize %d %d %d %d (%d x %d)\n", myClass ().getPersistentName (), size.left, size.top, size.right, size.bottom, size.getWidth (), size.getHeight ())

	NSRect nsRect;
	MacOS::toNSRect (nsRect, size);
	nsRect.origin.y = MacOS::flipCoord (nsRect.origin.y) - size.getHeight ();
	
	NSRect frame = [WINDOW frameRectForContentRect:nsRect];

	if([WINDOW styleMask] & NSWindowStyleMaskFullScreen)
	{
		// fullscreen: center on monitor, or move to left/top if window doesn't fit (ignore position of newSize) 
		int monitor = Desktop.findNearestMonitor (size.getCenter ());
		Rect monitorSize;
		Desktop.getMonitorSize (monitorSize, monitor, true);
		
		Coord freeY = monitorSize.getHeight () - Coord (frame.size.height);
		Coord freeX = monitorSize.getWidth () - Coord (frame.size.width);
		frame.origin.x = freeX >= 0 ? (freeX) / 2 : freeX;
		frame.origin.y = freeY >= 0 ? (freeY) / 2 : freeY;
	}

	// apply current sizeLimits to platform window before sizing it
	getSizeLimits ();
	NSSize minSize = {static_cast<CGFloat>(sizeLimits.minWidth), static_cast<CGFloat>(sizeLimits.minHeight)};
	NSSize maxSize = {static_cast<CGFloat>(sizeLimits.maxWidth), static_cast<CGFloat>(sizeLimits.maxHeight)};
	[WINDOW setContentMinSize:minSize];
	[WINDOW setContentMaxSize:maxSize];
	CCL_PRINTF ("  -> minW %d, maxW %d\n", (int)[WINDOW contentMinSize].width, (int)[WINDOW contentMaxSize].width)
	CCL_PRINTF ("  -> minH %d, maxH %d\n", (int)[WINDOW contentMinSize].height, (int)[WINDOW contentMaxSize].height)

	[WINDOW setFrame:frame display:YES];
	GraphicsPort port (this); // creates a rendertarget
	
	Rect r;
	getClientRect (r);
	invalidate (r);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API OSXWindow::moveWindow (PointRef pos)
{
	Rect size (getSize ());
	size.moveTo (pos);
	setSize (size);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void OSXWindow::setWindowTitle (StringRef title)
{
	if(!handle || isChildWindow ())
		return;
	
	NSString* nsString = title.createNativeString<NSString*> ();
	if(nsString)
	{
		[WINDOW setTitle:nsString];
		[nsString release];
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void OSXWindow::showWindow (bool state)
{
	if(!handle || isChildWindow ())
		return;

	NSWindow* nsWindow = WINDOW;

	if(state)
	{
		if(style.isCustomStyle (Styles::kWindowBehaviorTooltip))
			[nsWindow setAlphaValue:savedOpacity];
		[nsWindow orderFront:nil];
	}
	else
	{
		// Frequent orderFront/orderOut calls on a window, which can happen with tooltips, sometimes causes the main thread to block -> make invisible instead
		if(style.isCustomStyle (Styles::kWindowBehaviorTooltip))
			[nsWindow setAlphaValue:0.0];
		else
			[nsWindow orderOut:nil];
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API OSXWindow::maximize (tbool state)
{
	if(!handle || isChildWindow ())
		return;

	if(!state && isMinimized ())
		[WINDOW deminiaturize:nil];
	if(!state && isMaximized ())
		[WINDOW zoom:nil];
	if(state && !isMaximized ())
		[WINDOW zoom:nil];
		
	WindowEvent maximizeEvent (*this, state ? WindowEvent::kMaximize : WindowEvent::kUnmaximize);
	signalWindowEvent (maximizeEvent);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API OSXWindow::isMaximized () const
{
	if(!handle)
		return false;

	if(fullscreen)
		return false;

	if(style.isCustomStyle (Styles::kWindowBehaviorMaximizable))
		return [WINDOW isZoomed];

	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API OSXWindow::isMinimized () const
{
	if(!handle)
		return false;
		
	return [WINDOW isMiniaturized];
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API OSXWindow::setUserSize (RectRef userSize)
{
	// userSize not needed on macOS, [WINDOW zoom:nil] toggle takes care of this internally
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API OSXWindow::getUserSize (Rect& userSize) const
{
	userSize = size;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API OSXWindow::isVisible () const
{	
	if(!handle)
		return true;
	
	if(isMinimized ()) 
		return false;
		
	return [WINDOW isVisible];
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API OSXWindow::center ()
{
	if(!handle || isChildWindow ())
		return;
	
	[WINDOW center];
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API OSXWindow::redraw ()
{
	if(!nativeView)
		return;
	
	if(isChildWindow ())
		[VIEW displayIfNeeded];
	else if(NativeWindowRenderTarget* target = getRenderTarget ())
		target->onRender ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API OSXWindow::activate ()
{
	if(!handle || isChildWindow ())
		return;
	
	if(activating)
		return;
	
	ScopedVar<bool> activatingGuard (activating, true);
		
	if(!isMinimized () && !isActive ())
		[WINDOW makeKeyAndOrderFront:nil];
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API OSXWindow::isActive () const
{
	return [WINDOW isKeyWindow] == YES;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API OSXWindow::close ()
{
	if(isInCloseEvent () || isInDestroyEvent ())
		return false;

	if(onClose ())
	{
		BOOL wasActive = [WINDOW isKeyWindow];
		setInCloseEvent (true);
		setInDestroyEvent (true);
		
		OSXWindow::onDestroy ();

		if(nativeView)
		{
			nativeView->release ();
			nativeView = nil;
		}

		AutoPtr<OSXWindow> releaser = this;

		if(!isChildWindow ())
		{
			CCL_PRINTF ("Closing nsWindow\n", 0)
			[WINDOW setDelegate:nil];
			[WINDOW close];
			
			if(wasActive && !GUI.isQuitting () && Desktop.getActiveWindow () == nil)
			{
				// activate another window
				Window* w = unknown_cast<Window> (Desktop.getApplicationWindow ());
				if(!w)
					w = Desktop.getLastWindow (); // topmost
				OSXWindow* osxWindow = OSXWindow::cast (w);
				if(osxWindow)
					osxWindow->forceActivate (nil);
			}
		}
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void OSXWindow::updateSize ()
{
	if(!handle || (privateFlags & kInUpdateSize) != 0)
		return;

	ScopedFlag<kInUpdateSize> resizingScope (privateFlags);

	if(isChildWindow ())
		return;
	
	NSRect content = [WINDOW contentRectForFrameRect:[WINDOW frame]];
	content.origin.y = MacOS::flipCoord (content.origin.y) - (Coord)content.size.height;
	Rect size;
	MacOS::fromNSRect (size, content);

	CCL_PRINTF ("%s::updateSize %d %d %d %d (%d x %d)\n", myClass ().getPersistentName (), size.left, size.top, size.right, size.bottom, size.getWidth (), size.getHeight ())

	View::setSize (size);
	
	NSSize minSize = {static_cast<CGFloat>(sizeLimits.minWidth), static_cast<CGFloat>(sizeLimits.minHeight)};
	NSSize maxSize = {static_cast<CGFloat>(sizeLimits.maxWidth), static_cast<CGFloat>(sizeLimits.maxHeight)};
	[WINDOW setContentMinSize:minSize];
	[WINDOW setContentMaxSize:maxSize];
	CCL_PRINTF ("  -> minW %d, maxW %d\n", (int)[WINDOW contentMinSize].width, (int)[WINDOW contentMaxSize].width)
	CCL_PRINTF ("  -> minH %d, maxH %d\n", (int)[WINDOW contentMinSize].height, (int)[WINDOW contentMaxSize].height)
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API OSXWindow::getFrameSize (Rect& size) const
{
	if(!handle)
		return;

	NSRect frame;
	if(isChildWindow ())
		frame = [VIEW frame];
	else
		frame = [WINDOW frame];

	frame.origin.y = MacOS::flipCoord (frame.origin.y) - (Coord)frame.size.height;
	MacOS::fromNSRect (size, frame);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void OSXWindow::moveWindow ()
{
	if(!handle || isChildWindow ())
		return;
	
	Rect desktopWorkRect;
	Desktop.getVirtualScreenSize (desktopWorkRect, true);
	CGFloat frameH = [WINDOW frame].size.height;
	CGFloat maxY = MacOS::flipCoord (desktopWorkRect.top) - frameH;
	CGFloat minY = MacOS::flipCoord (desktopWorkRect.bottom) - frameH;

	NSEvent* nsEvent = [NSApp currentEvent];
	if(nsEvent == nil)
		return;
	
	NSPoint start = [WINDOW convertRectToScreen:NSMakeRect ([nsEvent locationInWindow].x, [nsEvent locationInWindow].y, 0, 0)].origin;	
	NSPoint windowLoc = [WINDOW frame].origin;
	NSPoint offset = {start.x - windowLoc.x, start.y - windowLoc.y};
	
	ScopedVar<bool> scope (inMoveLoop, true);
	bool loopTerminated = false;
	@autoreleasepool
	{
		while(!loopTerminated)
		{
			NSEvent* nsEvent = [NSApp nextEventMatchingMask:NSEventMaskAny untilDate:[NSDate dateWithTimeIntervalSinceNow:0.025] inMode:NSEventTrackingRunLoopMode dequeue:YES];
			if(nsEvent == nil)
			{
				SystemTimer::serviceTimers ();
				continue;
			}
			else
			{
				NSWindow* clickedWindow = [nsEvent window];
				switch([nsEvent type])
				{
					case NSEventTypeLeftMouseUp:
					case NSEventTypeRightMouseUp:
					case NSEventTypeOtherMouseUp:
						if(clickedWindow == WINDOW)
							loopTerminated = true;
						break;
						
					case NSEventTypeLeftMouseDragged:
					{
						NSPoint loc = [WINDOW convertRectToScreen:NSMakeRect([nsEvent locationInWindow].x, [nsEvent locationInWindow].y, 0, 0)].origin;
						loc.x -= offset.x;
						loc.y -= offset.y;
						loc.y = ccl_bound (loc.y, minY, maxY);
						[WINDOW setFrameOrigin:loc];
						continue;
					}
				}
			}
			[NSApp sendEvent:nsEvent];
		}
	}

    // check if top bar of window is covered by menu bar
    Point topCenter1 = (getSize ().getLeftTop () + getSize ().getRightTop ()) * .5;
    Point topCenter2 = topCenter1 + Point (0, 16);
    int monitor = Desktop.findMonitor (topCenter2, true);
    if(monitor == 0) // screen 0 is the one with the menu bar
    {
        Rect monitorRect, monitorWorkRect;
        Desktop.getMonitorSize (monitorRect, monitor, false);
        Desktop.getMonitorSize (monitorWorkRect, monitor, true);
        
        if((monitorRect.pointInside (topCenter1) && !monitorWorkRect.pointInside (topCenter1)) ||
           (monitorRect.pointInside (topCenter2) && !monitorWorkRect.pointInside (topCenter2)))
        {
            Rect size (getSize ());
            size.offset (0, monitorWorkRect.top - size.top);
            setSize (size);
		}
    }
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void OSXWindow::resizeWindow (int edge)
{
	if(!handle || isChildWindow ())
		return;

	NSEvent* nsEvent = [NSApp currentEvent];
	if(nsEvent == nil)
		return;
	
	NSPoint start = [WINDOW convertRectToScreen:NSMakeRect ([nsEvent locationInWindow].x, [nsEvent locationInWindow].y, 0, 0)].origin;
	
	NSRect frame = [WINDOW frame];
	NSRect newFrame = frame;

	bool loopTerminated = false;
	while(!loopTerminated)
	@autoreleasepool
	{
		NSEvent* nsEvent = [NSApp nextEventMatchingMask:NSEventMaskAny untilDate:[NSDate dateWithTimeIntervalSinceNow:0.025] inMode:NSDefaultRunLoopMode dequeue:YES];
		if(nsEvent == nil)
		{
			SystemTimer::serviceTimers ();
			continue;
		}
		else
		{
			NSWindow* clickedWindow = [nsEvent window];
			switch([nsEvent type])
			{
				case NSEventTypeLeftMouseUp:
				case NSEventTypeRightMouseUp:
				case NSEventTypeOtherMouseUp:
					if(clickedWindow == WINDOW)
						loopTerminated = true;
					break;
					
				case NSEventTypeLeftMouseDragged:
					NSPoint loc = [WINDOW convertRectToScreen:NSMakeRect ([nsEvent locationInWindow].x, [nsEvent locationInWindow].y, 0, 0)].origin;
					
					if(edge == kEdgeRight || edge == kEdgeBottomRight)
					{
					   newFrame.size.width  = ccl_bound<CGFloat> (frame.size.width + (loc.x - start.x), sizeLimits.minWidth, sizeLimits.maxWidth);
						newFrame.origin.y    = frame.origin.y + (frame.size.height - newFrame.size.height);
					}
					if(edge == kEdgeBottom || edge == kEdgeBottomRight)
					{
					   newFrame.size.height = ccl_bound<CGFloat> (frame.size.height - (loc.y - start.y), sizeLimits.minHeight, sizeLimits.maxHeight);
						newFrame.origin.y    = frame.origin.y + (frame.size.height - newFrame.size.height);						
					}
					
					if(edge == kEdgeLeft)
					{
						newFrame.size.width = ccl_bound<CGFloat> (frame.size.width - (loc.x - start.x), sizeLimits.minWidth, sizeLimits.maxWidth);
						newFrame.origin.x = frame.origin.x + (frame.size.width - newFrame.size.width);
					}
					if(edge == kEdgeTop)
						newFrame.size.height = ccl_bound<CGFloat> (frame.size.height - (start.y - loc.y), sizeLimits.minHeight, sizeLimits.maxHeight);
					
					// let view tree constrain this size (assuming no window frame, as in the code above)
					Rect client (0, 0, (Coord)newFrame.size.width, (Coord)newFrame.size.height);
					constrainSize (client);
					newFrame.size.width = client.getWidth ();
					newFrame.size.height = client.getHeight ();

					[WINDOW setFrame:newFrame display:YES];
					continue;
			}
		}
		[NSApp sendEvent:nsEvent];		
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void OSXWindow::setStyle (StyleRef style)
{
	StyleFlags oldStyle (this->style);
	if(style != oldStyle)
	{
		SuperClass::setStyle (style);
		
		NSWindowStyleMask windowMask = NSWindowStyleMaskBorderless;
		MacOS::translateWindowStyle (windowMask, style);
		[WINDOW setStyleMask:windowMask];

		if(style.isCustomStyle (Styles::kWindowAppearanceCustomFrame) && style.isCustomStyle (Styles::kWindowAppearanceRoundedCorners))
			suppressTitleBar ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL::Point& CCL_API OSXWindow::clientToScreen (CCL::Point& pos) const
{
	Point origin;
	screenToClient (origin); // result is negative
	pos.x -= origin.x;
	pos.y -= origin.y;
	return pos;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL::Point& CCL_API OSXWindow::screenToClient (CCL::Point& pos) const
{
	if(!VIEW)
		return pos;

	NSWindow* window = [VIEW window];
	if(!window)
		return pos;

	NSPoint screen = {static_cast<CGFloat>(pos.x), static_cast<CGFloat>(MacOS::flipCoord (pos.y))};
	NSPoint windowFrame = [window convertRectFromScreen:NSMakeRect (screen.x, screen.y, 0, 0)].origin;
	
	NSPoint windowContent = [window contentRectForFrameRect:NSMakeRect (windowFrame.x, windowFrame.y, 0, 0)].origin;
	windowContent.y = [window contentRectForFrameRect:[window frame]].size.height - windowContent.y;
	
	NSPoint offset = [VIEW convertPoint:NSMakePoint (0, 0) toView:nil];
	offset.y = [[window contentView] frame].size.height - offset.y;

	pos.x = (Coord)(windowContent.x - offset.x);
	pos.y = (Coord)(windowContent.y - offset.y);
	return pos;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool OSXWindow::setOpacity (float opacity)
{
	if(!handle)
		return false;
	
	[WINDOW setAlphaValue:opacity];
	savedOpacity = opacity;
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

float CCL_API OSXWindow::getContentScaleFactor () const
{
	if(!handle)
		return 1.f;
	
	return (float)[WINDOW backingScaleFactor];
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void OSXWindow::beforeMouseDown (const MouseEvent& event)
{
	if(!handle)
		return;
	
	View* activeView = ccl_cast<View> (Desktop.getActiveWindow ());
	Window* activeWindow = activeView ? activeView->getWindow () : nil;
	if(activeWindow != this)
		forceActivate (activeWindow);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API OSXWindow::scrollClient (RectRef rect, PointRef delta)
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

void OSXWindow::updateBackgroundColor ()
{
	if(!handle)
		return;
	
	NSWindow* nsWindow = WINDOW;
	bool windowTitlebar = [nsWindow styleMask] & NSWindowStyleMaskTitled;
	
	Color c = visualStyle ? visualStyle->getBackColor () : (windowTitlebar ? Colors::kWhite : Colors::kTransparentBlack);
	if(shouldBeTranslucent ())
		c.setAlphaF (0);
							
	NSColor* nsColor = [NSColor colorWithDeviceRed:c.getRedF() green:c.getGreenF () blue: c.getBlueF () alpha:c.getAlphaF ()];

	if(windowTitlebar)
	{
		if(transparentTitlebarConfiguration)
		{
			if(c.getIntensity () < 0.5f) // dark color -> set dark appearance -> results in bright text
			{
				[nsWindow setAppearance: [NSAppearance appearanceNamed:NSAppearanceNameDarkAqua]];
				
				[nsWindow setTitlebarAppearsTransparent:YES];
			}
			else // light theme
			{
				[nsWindow setAppearance: [NSAppearance appearanceNamed:NSAppearanceNameAqua]];
				
				// Floating Windows have no header back color in light theme
				// Solution: ignore transparent headers for floating windows
				
				if(([nsWindow styleMask] & NSWindowStyleMaskUtilityWindow) == 0)	// exclude non-floating windows
					[nsWindow setTitlebarAppearsTransparent:YES];
			}
		}
		
		if(coloredTitlebarConfiguration)
			[nsWindow setBackgroundColor:nsColor];
	}
	else
	{
		[nsWindow setBackgroundColor:nsColor];
		if(c.getAlphaF () < 1.f)
			[nsWindow setOpaque:NO];
		
		if(getNativeView () && getNativeView ()->getLayer ())
		{
			getNativeView ()->getLayer ().backgroundColor = [nsColor CGColor];
		}
	}
	
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void OSXWindow::embed (NativeView* subView)
{
	if(!nativeView || !subView)
		return;
		
	[VIEW addSubview:subView->getView ()];
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool OSXWindow::isAttached ()
{
	if(isChildWindow ())
		return true;
	else
		return Window::isAttached ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void OSXWindow::onNativeViewRemoved ()
{
	signal (Message (kOnRemoved));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API OSXWindow::setFullscreen (tbool state)
{
	bool wasFullscreen = fullscreen;

	if(state != wasFullscreen)
	{
		if((style.isCustomStyle (Styles::kWindowBehaviorFloating) || style.isCustomStyle (Styles::kWindowBehaviorIntermediate)) && state)
		{
			NSWindowCollectionBehavior windowBehavior = [WINDOW collectionBehavior];
			windowBehavior |= NSWindowCollectionBehaviorFullScreenPrimary;
			windowBehavior |= NSWindowCollectionBehaviorManaged;
			windowBehavior &= ~NSWindowCollectionBehaviorFullScreenAuxiliary;
			windowBehavior &= ~NSWindowCollectionBehaviorIgnoresCycle;
			[WINDOW setCollectionBehavior:windowBehavior];
		}
		[WINDOW toggleFullScreen:nil];
	}
		
	return wasFullscreen;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API OSXWindow::isFullscreen () const
{
	return fullscreen;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void* CCL_API OSXWindow::getSystemWindow () const
{
	NSWindow* parent = (NSWindow*)handle;
	
	#if DEBUG_LOG
	if(parent && !parent.contentView)
		CCL_PRINTLN ("OSXWindow: WARN: System window had no contentView!");
	#endif
	
	if(isChildWindow () && nativeView)
	{
		return nativeView->getView ();
	}

	return parent ? parent.contentView : nil;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void OSXWindow::setFullscreenState (bool state)
{
	fullscreen = state;

	WindowEvent event (*this, state ? WindowEvent::kFullscreenEnter : WindowEvent::kFullscreenLeave);
	signalWindowEvent (event);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void OSXWindow::setNativeView (NativeView* _nativeView)
{
	ASSERT (nativeView == nil)
	nativeView = _nativeView;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void OSXWindow::forceActivate (Window* oldActiveWindow)
{
	activate ();
	
	// assist in activation:
	if(oldActiveWindow && oldActiveWindow->isActive ())
	{
		CCL_PRINT ("   deactivate (force): ")
		CCL_PRINTLN (oldActiveWindow->getTitle ())
		oldActiveWindow->onActivate (false);
	}
	if(!isActive())
	{
		CCL_PRINT ("   forceActivate: ")
		CCL_PRINTLN (getTitle ())
		onActivate (true);
		if(!isMinimized ())
		{
			NSWindow* systemWindow = WINDOW;
			if(systemWindow)
			{
				if([systemWindow canBecomeMainWindow])
					[systemWindow makeMainWindow];
				if([systemWindow canBecomeKeyWindow])
					[systemWindow makeKeyWindow];
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

QuartzBitmap* OSXWindow::createScreenshotFromWindow () const
{
	CGWindowID windowID = (CGWindowID)[WINDOW windowNumber];
	if(!windowID)
		return nil;

	Point origin;
	origin = clientToScreen (origin);

	CGWindowListOption includes = kCGWindowListOptionIncludingWindow;
	if(NSArray* childs = [WINDOW childWindows])
		if([childs count] > 0)
			includes |= kCGWindowListOptionOnScreenAboveWindow;

	CGImageRef windowImage = CGWindowListCreateImage (CGRectMake (origin.x, origin.y, size.getWidth (), size.getHeight ()), includes, windowID, kCGWindowImageDefault);
	QuartzBitmap* quartzBitmap = NEW QuartzBitmap (windowImage);
	quartzBitmap->setContentScaleFactor (getContentScaleFactor ());
	CGImageRelease (windowImage);
	
	return quartzBitmap;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void OSXWindow::onSize (const Point& delta)
{
	Window::onSize (delta);
	
	if(isChildWindow ())
	{
		// This is only necessary when we are embedded in another application
		if((windowMode == kWindowModeEmbedding) && ([VIEW superview].autoresizesSubviews == NO))
		{
			NSRect newFrame (NSZeroRect);
			newFrame.size = [VIEW bounds].size;
			newFrame.size.width += delta.x;
			newFrame.size.height += delta.y;
			[VIEW setFrame:newFrame];
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool OSXWindow::isChildWindow () const
{
	return windowMode == kWindowModeHosting || windowMode == kWindowModeEmbedding;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void OSXWindow::suppressTitleBar ()
{
	if(handle)
	{
		[WINDOW setTitlebarAppearsTransparent:YES];
		[WINDOW setMovable:NO];
		[WINDOW setTitleVisibility:NSWindowTitleHidden];
		[[WINDOW standardWindowButton:NSWindowCloseButton] setHidden:YES];
		[[WINDOW standardWindowButton:NSWindowMiniaturizeButton] setHidden:YES];
		[[WINDOW standardWindowButton:NSWindowZoomButton] setHidden:YES];
	}
}
