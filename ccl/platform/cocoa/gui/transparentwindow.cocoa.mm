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
// Filename    : ccl/platform/cocoa/gui/transparentwindow.cocoa.mm
// Description : Transparent Window for Cocoa Windows
//
//************************************************************************************************

/*
 http://developer.apple.com/samplecode/RoundTransparentWindow/listing6.html
*/

#define DEBUG_LOG 0

#include "ccl/platform/cocoa/gui/transparentwindow.cocoa.h"
#include "ccl/platform/cocoa/gui/platformwindow.mac.h"
#include "ccl/platform/cocoa/gui/nativeview.mac.h"
#include "ccl/platform/cocoa/quartz/nshelper.h"

#include "ccl/gui/windows/window.h"
#include "ccl/gui/windows/nativewindow.h"
#include "ccl/gui/graphics/imaging/offscreen.h"

#include "ccl/platform/cocoa/cclcocoa.h"

#include "ccl/public/base/ccldefpush.h"

using namespace CCL;
using namespace MacOS;

//************************************************************************************************
// PlatformTransparentWindow
//************************************************************************************************

@interface CCL_ISOLATED (PlatformTransparentWindow) : NSWindow
{
}
- (id)initWithContentRect:(NSRect)contentRect styleMask:(NSWindowStyleMask)windowMask;
- (BOOL)canBecomeKeyWindow;
- (BOOL)canBecomeMainWindow;
@end

//************************************************************************************************
// TransparentWindowView
//************************************************************************************************

@interface CCL_ISOLATED (TransparentWindowView): NSView
{
	CCL::Window* window;
}

- (id)initWithFrame:(NSRect)frameRect window:(CCL::Window*)window;
- (void)drawRect:(NSRect)dirtyRect;
- (BOOL)isOpaque;
- (BOOL)isFlipped;
@end

//************************************************************************************************
// OSXTransparentWindow
//************************************************************************************************

class OSXTransparentWindow : public OSXWindow
{
public:
	OSXTransparentWindow (TransparentWindow* window, float contentScaleFactor);

	float CCL_API getContentScaleFactor () const { return contentScaleFactor; }
	void draw (const UpdateRgn& updateRgn);

protected:
	TransparentWindow* transparentWindow;
	float contentScaleFactor;
};

//////////////////////////////////////////////////////////////////////////////////////////////////

OSXTransparentWindow::OSXTransparentWindow (TransparentWindow* window, float _contentScaleFactor)
: transparentWindow (window),
  contentScaleFactor (_contentScaleFactor)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void OSXTransparentWindow::draw (const UpdateRgn& updateRgn)
{
	if(transparentWindow)
		if(Bitmap* bitmap = transparentWindow->getSavedBitmap ())
		{
			GraphicsPort port (this);
			port.addClip (updateRgn.bounds);
			port.clearRect (updateRgn.bounds);
			bitmap->draw (port, CCL::Point ());
		}
}

//************************************************************************************************
// PlatformTransparentWindow
//************************************************************************************************

@implementation CCL_ISOLATED (PlatformTransparentWindow)

//////////////////////////////////////////////////////////////////////////////////////////////////

- (id)initWithContentRect:(NSRect)contentRect styleMask:(NSWindowStyleMask)windowMask
{
	self = [super initWithContentRect:contentRect styleMask:windowMask backing: NSBackingStoreBuffered defer:NO];
	return self;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (BOOL)canBecomeKeyWindow
{
	return NO;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (BOOL)canBecomeMainWindow
{
	return NO;
}

@end

//************************************************************************************************
// TransparentWindowView implementation
//************************************************************************************************

@implementation CCL_ISOLATED (TransparentWindowView)

//////////////////////////////////////////////////////////////////////////////////////////////////

- (id)initWithFrame:(NSRect)frameRect window:(Window*)window
{
	self = [super initWithFrame:frameRect];
	self->window = window;
	return self;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//	Overides
//////////////////////////////////////////////////////////////////////////////////////////////////

- (BOOL)isFlipped
{
    return YES;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (BOOL)isOpaque
{
	return NO;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (void)drawRect:(NSRect)dirtyRect
{	
	if(!window)
		return;
	
    #if USE_OFFSCREEN_TARGET
	if(NativeWindowRenderTarget* target = window->getRenderTarget ())
	{
		if(IMutableRegion* region = target->getUpdateRegion ())
		{
			CCL::Rect rect;
			MacOS::fromNSRect (rect, dirtyRect);
			region->addRect (rect);
		}
		target->onRender ();
	}
    #else
    
    CCL::Rect size;
    window->getClientRect (size);

    const NSRect* rects = nil;
    NSInteger count = 0;
    [self getRectsBeingDrawn:&rects count:&count];
    
    NSClipRegion clipRegion (dirtyRect, rects, count);
    CCL::Rect updateRect;
    MacOS::fromNSRect (updateRect, dirtyRect);
    
    window->setInDrawEvent (true);
    window->draw (UpdateRgn (updateRect, (count > 1) ? &clipRegion : 0));
    window->setInDrawEvent (false);
    #endif
}
@end

//************************************************************************************************
// TransparentWindow
//************************************************************************************************

TransparentWindow* TransparentWindow::create (Window* parentWindow, int options, StringRef title)
{
	return NEW CocoaTransparentWindow (parentWindow, options, title);
}

//************************************************************************************************
// CocoaTransparentWindow
//************************************************************************************************

CocoaTransparentWindow::CocoaTransparentWindow (Window* parentWindow, int options, StringRef title)
: TransparentWindow (parentWindow, options, title),
  nativeWindow (nil),
  initialized (false),
  visible (true),
  suspended (false)
{
	NSRect bounds = NSMakeRect (0, 0, 1, 1);

	OSXWindow* platformWindow = NEW OSXTransparentWindow (this, parentWindow->getContentScaleFactor ());
	osxWindow = platformWindow;
	CCL_ISOLATED (TransparentWindowView)* contentView = [[CCL_ISOLATED (TransparentWindowView) alloc] initWithFrame:bounds window:osxWindow];
	platformWindow->setNativeView (NEW NativeView (contentView, platformWindow));
	
	NSWindow* window = [[CCL_ISOLATED (PlatformTransparentWindow) alloc] initWithContentRect:bounds styleMask:NSWindowStyleMaskBorderless];
	[window setContentView: contentView];
	[contentView release];
	[window setIgnoresMouseEvents:YES];
	[window setHasShadow: NO];
	[window setAcceptsMouseMovedEvents: NO];
	[window setReleasedWhenClosed:YES];
	[window setOpaque:NO];
	[window setBackgroundColor:[NSColor clearColor]];
	nativeWindow = window;

	// the window will be added as child to the parentWindow in the first update call
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CocoaTransparentWindow::~CocoaTransparentWindow ()
{
	osxWindow->onDestroy ();
	
	CCL_PRINTLN ("~CocoaTransparentWindow")
	if(nativeWindow)
	{
		if(initialized)
			if(NSWindow* parentNSWindow = toNSWindow (parentWindow))
				[parentNSWindow removeChildWindow:nativeWindow];

		[nativeWindow close];
	}
	osxWindow = nil;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CocoaTransparentWindow::show ()
{
    if(suspended)
        return;
    
    if(visible)
        return;
    visible = true;
    
	CCL_PRINTLN ("TransparentWindow::show")
	
    NSWindow* parentNSWindow = toNSWindow (parentWindow);
	if(!initialized)
	{
		// add as child window 
		if(parentNSWindow)
		{
			CCL_PRINTLN ("TransparentWindow::show: addChildWindow")
			[parentNSWindow addChildWindow:nativeWindow ordered:NSWindowAbove];
        }
		initialized = true;
	}
    
    if(parentNSWindow && (options & kKeepOnTop) == 0)
        [nativeWindow orderWindow:NSWindowAbove relativeTo:[parentNSWindow windowNumber]];
    else
        [nativeWindow orderFront:nil]; 
	
	NSRect frame;
	toNSRect (frame, size);
	frame.origin.y = flipCoord (frame.origin.y) - frame.size.height;
	[nativeWindow setFrame:frame display:YES];
}

//////////////////////////////////////////////////////////////////////////////////////////////////
 
void CocoaTransparentWindow::hide ()
{
    if(suspended)
        return;
    
    if(!visible)
        return;
    visible = false;
	
	CCL_PRINTLN ("TransparentWindow::hide")
	
	if(initialized)
	{
		if(NSWindow* parentNSWindow = toNSWindow (parentWindow))
		{
			CCL_PRINTLN ("TransparentWindow::hide: removeChildWindow")
			[parentNSWindow removeChildWindow:nativeWindow];
        }
		initialized = false;
	}
	
	[nativeWindow orderOut:nil]; 
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool CocoaTransparentWindow::isVisible () const
{
	return visible;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CocoaTransparentWindow::suspend (bool state)
{
    if(state)
    {
        hide ();
        suspended = true;
    }
    else
    {
        suspended = false;
        show ();
    }
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CocoaTransparentWindow::update (RectRef _size, Bitmap& bitmap, PointRef offset, float opacity)
{
	size = _size;
	Rect windowSize (0, 0, size.getWidth (), size.getHeight ());
	osxWindow->setSize (windowSize);
	
	// move/size the window
	NSRect frame;
	toNSRect (frame, size);
	frame.origin.y = flipCoord (frame.origin.y) - frame.size.height;
	[nativeWindow setFrame:frame display:NO];
	[nativeWindow setAlphaValue:opacity];
	
	if(NativeWindowRenderTarget* target = osxWindow->getRenderTarget ())
		target->onSize ();
	
    CCL_PRINTF ("TransparentWindow::update: pos (%d, %d) size (%d, %d) offset (%d, %d)\n",
                size.left, size.top, size.getWidth (), size.getHeight (), offset.x, offset.y)

	// copy bitmap into offscreen
	AutoPtr<Offscreen> offscreen = NEW Offscreen (size.getWidth (), size.getHeight (), Offscreen::kRGBAlpha, false, parentWindow);
	{
		Rect src (windowSize);
		src.offset (offset);
		BitmapGraphicsDevice device (offscreen);
		device.drawImage (&bitmap, src, windowSize);
	}
	setSavedBitmap (offscreen);
	
	if(visible && !suspended)
	{
		if(!initialized)
		{
			// add as child window
			if(NSWindow* parentNSWindow = toNSWindow (parentWindow))
			{
				CCL_PRINTLN ("TransparentWindow::update: addChildWindow")
				[parentNSWindow addChildWindow:nativeWindow ordered:NSWindowAbove];
			}
			initialized = true;
		}
		
		osxWindow->invalidate (windowSize);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CocoaTransparentWindow::move (PointRef position)
{
	size.moveTo (position);
	
	if(visible && initialized && !suspended)
    {
        CCL_PRINTF ("TransparentWindow::move: x = %d y = %d\n", position.x, position.y)
	
        NSRect frame = [nativeWindow frame];
        frame.origin.x = position.x;
        frame.origin.y = flipCoord (position.y) - frame.size.height;

        [nativeWindow setFrame:frame display:NO];
    }
}
