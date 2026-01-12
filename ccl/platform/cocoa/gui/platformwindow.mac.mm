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
// Filename    : platformwindow.mac.mm
// Description : Customized Cocoa window classes
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "platformwindow.mac.h"

#include "ccl/gui/windows/window.h"
#include "ccl/gui/windows/desktop.h"
#include "ccl/gui/gui.h"
#include "ccl/gui/graphics/nativegraphics.h"

#include "ccl/base/message.h"

#include "ccl/platform/cocoa/quartz/nshelper.h"

#include "window.mac.h"
#include "transparentwindow.cocoa.h"

using namespace CCL;

//************************************************************************************************
// FlippedView
//************************************************************************************************

@implementation CCL_ISOLATED (FlippedView)

- (id)initWithFrame:(NSRect)frameRect
{
	self = [super initWithFrame:frameRect];
	if(self)
		self->childWindow = NO;

	return self;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (id)initWithFrame:(NSRect)frameRect childWindow:(BOOL)isChildWindow
{
	self = [super initWithFrame:frameRect]; 
	if(self)
		self->childWindow = isChildWindow;

	return self;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (BOOL)isFlipped
{
    return YES;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (BOOL)acceptsFirstMouse:(NSEvent*)nsEvent
{
	return YES;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (BOOL)acceptsFirstResponder
{
	// When we are in a ChildWindow we must stay away from being firstResponder
	if(childWindow)
		return NO;

	// We do not want first responder state,
	// but returning YES here enables [PlatformWindow makeFirstResponder] being called
	return YES;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (BOOL)becomeFirstResponder
{
	return NO;
}

@end

//************************************************************************************************
// WindowController
//************************************************************************************************

@implementation CCL_ISOLATED (WindowController)

//////////////////////////////////////////////////////////////////////////////////////////////////

- (id)initWithFrameworkWindow:(OSXWindow*)window
{
	if(self = [super init])
		frameworkWindow = window;

	return self;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (BOOL)windowShouldClose:(id)window
{
	if(frameworkWindow->onClose ())
	{
		//keep this object alive in case it gets released by the frameworkWindow
		[[self retain] autorelease];
 
		frameworkWindow->setInCloseEvent (true);
		frameworkWindow->setInDestroyEvent (true);
 
		frameworkWindow->onDestroy ();

		frameworkWindow->release ();
		frameworkWindow = nullptr;

		if(!GUI.isQuitting () && Desktop.getActiveWindow () == nullptr)
		{
			// activate another window
			Window* w = unknown_cast<Window> (Desktop.getApplicationWindow ());
			if(!w)
				w = Desktop.getLastWindow (); // topmost
 			OSXWindow* osxWindow = OSXWindow::cast (w);
			if(osxWindow)
				osxWindow->forceActivate (0);
		}
		return YES;
	}
	return NO;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (void)windowDidResize:(NSNotification*)notification
{
	if([self suppressResize:notification])
		return;
 
	if(frameworkWindow)
		frameworkWindow->updateSize ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (NSSize)windowWillResize:(NSWindow*)window toSize:(NSSize)toSize
{
	if(frameworkWindow && window)
	{
		// translate to client rect
		NSRect frame (NSMakeRect (0, 0, toSize.width, toSize.height));
		NSRect content = [window contentRectForFrameRect:frame];
		content.origin.y = MacOS::flipCoord (content.origin.y) - (Coord)content.size.height;
		CCL::Rect clientRect;
		MacOS::fromNSRect (clientRect, content);

		CCL::Rect constrained (clientRect);
		frameworkWindow->constrainSize (constrained);
		
		if(constrained != clientRect)
		{
			CCL::Point diff (constrained.getSize () - clientRect.getSize ());
			toSize.width += diff.x;
			toSize.height += diff.y;
		}
	}
	return toSize;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (void)windowWillStartLiveResize:(NSNotification*)notification
{
	if(frameworkWindow)
		frameworkWindow->onResizing (true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (void)windowDidEndLiveResize:(NSNotification*)notification
{
	if(frameworkWindow)
		frameworkWindow->onResizing (false);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (NSRect)windowWillUseStandardFrame:(NSWindow*)window defaultFrame:(NSRect)newFrame
{
    return newFrame; // allow the window to potentially fill the whole screen
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (void)windowDidMove:(NSNotification*)notification
{
	if([self suppressResize:notification])
		return;
 
	if(frameworkWindow)
		frameworkWindow->updateSize ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (void)windowDidBecomeKey:(NSNotification*)notification
{
	if(frameworkWindow)
		frameworkWindow->onActivate (true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (void)windowDidResignKey:(NSNotification*)notification
{
	if(frameworkWindow)
		frameworkWindow->onActivate (false);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (void)windowWillMiniaturize:(NSNotification*)notification
{
    AutoPtr<Iterator> iter = frameworkWindow->getTransparentWindows ();
    while(!iter->done ())
    {
        CocoaTransparentWindow* w = (CocoaTransparentWindow*)iter->next ();
        w->suspend (true);
    }
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (void)windowDidDeminiaturize:(NSNotification*)notification
{
    AutoPtr<Iterator> iter = frameworkWindow->getTransparentWindows ();
    while(!iter->done ())
    {
        CocoaTransparentWindow* w = (CocoaTransparentWindow*)iter->next ();
        w->suspend (false);
    }
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (BOOL)windowShouldZoom:(NSWindow*)window toFrame:(NSRect)proposedFrame
{
	if(frameworkWindow == nullptr)
		return NO;

	return YES;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (void)windowDidEnterFullScreen:(NSNotification*)notification
{
	if(frameworkWindow)
	{
		frameworkWindow->updateSize ();
		frameworkWindow->setFullscreenState (true);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (void)windowDidExitFullScreen:(NSNotification*)notification
{
	if(frameworkWindow)
	{
		CCL_PRINTF ("\nwindowDidExitFullScreen %d x %d\n", frameworkWindow->getSize ().getWidth (), frameworkWindow->getSize ().getHeight ())
		frameworkWindow->updateSize ();
 
		// window has been resized to the old size before fullscreen, but the OS doesn't seem take the new limits into account (min/maxSize ignored, windowWillResize is not called)
		CCL::Rect size (frameworkWindow->getSize ());
		frameworkWindow->getSizeLimits ().makeValid (size);
		frameworkWindow->setSize (size);
		frameworkWindow->setFullscreenState (false);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (void)windowDidChangeBackingProperties:(NSNotification*)notification
{
 
    NSWindow* theWindow = (NSWindow*)[notification object];
 
    CGFloat newBackingScaleFactor = [theWindow backingScaleFactor];
    CGFloat oldBackingScaleFactor = [[[notification userInfo] objectForKey:@"NSBackingPropertyOldScaleFactorKey"] floatValue];

    if((newBackingScaleFactor != oldBackingScaleFactor) && frameworkWindow)
	{
		DisplayChangedEvent event ((float)newBackingScaleFactor, CCL::DisplayChangedEvent::kResolutionChanged);
		frameworkWindow->onDisplayPropertiesChanged (event);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (BOOL)suppressResize:(NSNotification*) notification
{
	if([[notification object] isKindOfClass:[CCL_ISOLATED (PlatformPanel) class]])
	{
		CCL_ISOLATED (PlatformPanel)* panel = (CCL_ISOLATED (PlatformPanel)*)[notification object];
		if([panel isInResize])
			return YES;
	}
 
	return NO;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (void) onFirstResponderChanged:(NSResponder*)responder
{
	if(frameworkWindow)
		frameworkWindow->signal (Message (IWindow::kFirstResponderChanged, IntPtr (responder)));
}

@end

//************************************************************************************************
// PlatformWindow
//************************************************************************************************

@implementation CCL_ISOLATED (PlatformWindow)

//////////////////////////////////////////////////////////////////////////////////////////////////

- (id)initWithContentRect:(NSRect)contentRect styleMask:(NSWindowStyleMask)windowMask
{
	self = [super initWithContentRect:contentRect styleMask:windowMask backing:NSBackingStoreBuffered defer:NO];
	return self;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (BOOL)canBecomeKeyWindow
{
	return YES;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (BOOL)makeFirstResponder:(NSResponder*)aResponder
{
	#if DEBUG_LOG
	NSString* n1 = NSStringFromClass ([aResponder class]);
	NSString* n2 = NSStringFromClass ([[self firstResponder] class]);
	String n1s; n1s.appendNativeString (n1);
	String n2s; n2s.appendNativeString (n2);
	CCL_PRINTF ("makeFirstResponder: %s (was %s)\n", MutableCString (n1s).str (), MutableCString (n2s).str ())
	#endif

	BOOL result = NO;
	if([aResponder isKindOfClass:[CCL_ISOLATED (FlippedView) class]] || [aResponder isKindOfClass:[CCL_ISOLATED (PlatformWindow) class]])
	{
		NSView* view = [self contentView];
		while(view && [view acceptsFirstResponder] == NO)
			view = [[view subviews] objectAtIndex:0];
		if(view)
			result = [super makeFirstResponder:view];
	}
	else
		result = [super makeFirstResponder:aResponder];
	
	if(result)
		if([[self delegate] isKindOfClass:[CCL_ISOLATED (WindowController) class]])
		{
			auto controller = (CCL_ISOLATED (WindowController)*)[self delegate];
			[controller onFirstResponderChanged:aResponder];
		}
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (id)accessibilityHitTest:(NSPoint)point
{
	NSArray* children = [self accessibilityChildren];

	if(!children)
		return [super accessibilityHitTest:point];

	for(id element in children)
	{
		if(CGRectContainsPoint ([element accessibilityFrame], point))
		{
			id hit = [element accessibilityHitTest:point];
			if(hit)
				return hit;
		}
	}

	return [super accessibilityHitTest:point];
}
@end

//************************************************************************************************
// PlatformPanel
//************************************************************************************************

@implementation CCL_ISOLATED (PlatformPanel)

@synthesize floatingLevel = _floatingLevel;
@synthesize inResize = _inResize;

//////////////////////////////////////////////////////////////////////////////////////////////////

- (id)initWithContentRect:(NSRect)contentRect styleMask:(NSWindowStyleMask)windowMask
{
	self = [super initWithContentRect:contentRect styleMask:windowMask backing:NSBackingStoreBuffered defer:NO];
	_inResize = NO;
	if([[NSApplication sharedApplication] isActive])
		_floatingLevel = NSFloatingWindowLevel;
	else
		_floatingLevel = NSNormalWindowLevel;
	[self setLevel: _floatingLevel];
 
	[[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(makeFloat) name:NSApplicationDidBecomeActiveNotification object:nil];
	[[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(makeUnfloat) name:NSApplicationWillResignActiveNotification object:nil];

	return self;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (void)makeFloat
{
	[self setLevel: [self floatingLevel]];
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (void)makeUnfloat
{
	[self setLevel:NSNormalWindowLevel];
	[self orderFront:self];
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (BOOL)canBecomeKeyWindow
{
	return YES;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (BOOL)makeFirstResponder:(NSResponder*)aResponder
{
	#if DEBUG_LOG
	NSString* n1 = NSStringFromClass ([aResponder class]);
	NSString* n2 = NSStringFromClass ([[self firstResponder] class]);
	String n1s; n1s.appendNativeString (n1);
	String n2s; n2s.appendNativeString (n2);
	CCL_PRINTLN ("makeFirstResponder")
	CCL_PRINTLN (n1s)
	CCL_PRINTLN (n2s)
	#endif

	BOOL result = NO;
	if([aResponder isKindOfClass:[CCL_ISOLATED (FlippedView) class]] || [aResponder isKindOfClass:[CCL_ISOLATED (PlatformWindow) class]])
	{
		NSView* view = [self contentView];
		while(view && [view acceptsFirstResponder] == NO)
			view = [[view subviews] objectAtIndex:0];
		if(view)
			result = [super makeFirstResponder:view];
	}
	else
		result = [super makeFirstResponder:aResponder];

	if(result)
		if([[self delegate] isKindOfClass:[CCL_ISOLATED (WindowController) class]])
		{
			auto controller = (CCL_ISOLATED (WindowController)*)[self delegate];
			[controller onFirstResponderChanged:aResponder];
		}
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (void)expandIfNeeded:(NSRect)newContentRect
{
	NSSize size = [[self contentView] frame].size;
	NSSize newSize = size;
	CGFloat maxX = CGRectGetMaxX (NSRectToCGRect (newContentRect));
	CGFloat maxY = CGRectGetMaxY (NSRectToCGRect (newContentRect));
	if(size.width < maxX)
		newSize.width = maxX;
 
	if(size.height < maxY)
		newSize.height = maxY;
 
	if(!NSEqualSizes (size, newSize))
	{
		#if DEBUG_LOG
		NSLog (@"Expand from %f,%f to %f,%f\n", size.width, size.height, newSize.width, newSize.height);
		#endif
		NSPoint origin = [self frame].origin;
		origin.y -= newSize.height - size.height;
		[self setInResize:YES];
		[self setFrameOrigin:origin];
		[self setContentSize:newSize];
		[self setInResize:NO];
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (id)accessibilityHitTest:(NSPoint)point
{
	NSArray* children = [self accessibilityChildren];

	if(!children)
		return [super accessibilityHitTest:point];

	for(id element in children)
	{
		if(CGRectContainsPoint ([element accessibilityFrame], point))
		{
			id hit = [element accessibilityHitTest:point];
			if(hit)
				return hit;
		}
	}

	return [super accessibilityHitTest:point];
}
@end

//************************************************************************************************
// PlatformTooltip
//************************************************************************************************

@implementation CCL_ISOLATED (PlatformTooltip)

//////////////////////////////////////////////////////////////////////////////////////////////////

- (id)initWithContentRect:(NSRect)contentRect styleMask:(NSWindowStyleMask)windowMask
{
	self = [super initWithContentRect:contentRect styleMask:windowMask backing:NSBackingStoreBuffered defer:NO];
	return self;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (BOOL)canBecomeKeyWindow
{
	return NO;
}

@end
