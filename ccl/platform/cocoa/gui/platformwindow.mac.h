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
// Filename    : platformwindow.mac.h
// Description : Customized Cocoa window classes
//
//************************************************************************************************

#ifndef _ccl_platformwindow_mac_h
#define _ccl_platformwindow_mac_h

#include "ccl/platform/cocoa/cclcocoa.h"

#include "ccl/public/base/ccldefpush.h"

namespace CCL {
	class OSXWindow;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

@interface CCL_ISOLATED (ContentView): NSView
@end

//////////////////////////////////////////////////////////////////////////////////////////////////

@interface CCL_ISOLATED (FlippedView) : NSView
{
	BOOL childWindow;
}
- (BOOL)isFlipped;
@end

//////////////////////////////////////////////////////////////////////////////////////////////////

@interface CCL_ISOLATED (WindowController): NSObject<NSWindowDelegate>
{
	CCL::OSXWindow* frameworkWindow;
}
- (CCL_ISOLATED (WindowController)*) initWithFrameworkWindow:(CCL::OSXWindow*) window;
- (BOOL) windowShouldClose:(id)window;
- (NSSize)windowWillResize:(NSWindow*)window toSize:(NSSize)toSize;
- (void) windowWillStartLiveResize:(NSNotification*)notification;
- (void) windowDidEndLiveResize:(NSNotification*)notification;
- (void) windowDidResize:(NSNotification*)notification;
- (void) windowDidBecomeKey:(NSNotification*)notification;
- (BOOL) windowShouldZoom:(NSWindow*)window toFrame:(NSRect)proposedFrame;
- (BOOL) suppressResize:(NSNotification*)notification;
- (void) onFirstResponderChanged:(NSResponder*)responder;
@end

//////////////////////////////////////////////////////////////////////////////////////////////////

@interface CCL_ISOLATED (PlatformWindow) : NSWindow
{
}
- (id)initWithContentRect:(NSRect)contentRect styleMask:(NSWindowStyleMask)windowMask;
- (id)accessibilityHitTest:(NSPoint)point;
@end

//////////////////////////////////////////////////////////////////////////////////////////////////

@interface CCL_ISOLATED (PlatformPanel) : NSPanel
{
	CGWindowLevel _floatingLevel;
	BOOL _inResize;
	
}
@property (nonatomic, assign) CGWindowLevel floatingLevel;
@property (nonatomic, assign, getter=isInResize) BOOL inResize;
- (id)initWithContentRect:(NSRect)contentRect styleMask:(NSWindowStyleMask)windowMask;
- (void)makeFloat;
- (void)makeUnfloat;
- (void)expandIfNeeded:(NSRect)newContentRect;
- (id)accessibilityHitTest:(NSPoint)point;
@end

//////////////////////////////////////////////////////////////////////////////////////////////////

@interface CCL_ISOLATED (PlatformTooltip) : NSPanel
{
}
- (id)initWithContentRect:(NSRect)contentRect styleMask:(NSWindowStyleMask)windowMask;
@end

//////////////////////////////////////////////////////////////////////////////////////////////////

#include "ccl/public/base/ccldefpop.h"

#endif
