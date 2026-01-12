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
// Filename    : ccl/platform/cocoa/gui/gui.cocoa.mm
// Description : platform-specific GUI implementation
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "ccl/base/storage/url.h"
#include "ccl/gui/gui.h"
#include "ccl/gui/commands.h"
#include "ccl/gui/popup/menu.h"
#include "ccl/gui/views/view.h"
#include "ccl/gui/keyevent.h"
#include "ccl/gui/system/systemtimer.h"
#include "ccl/gui/system/mousecursor.h"
#include "ccl/gui/system/systemevent.h"
#include "ccl/gui/windows/desktop.h"
#include "ccl/gui/windows/window.h"
#include "ccl/gui/graphics/imaging/bitmap.h"
#include "ccl/public/gui/iapplication.h"
#include "ccl/public/gui/framework/guievent.h"
#include "ccl/public/systemservices.h"
#include "ccl/public/system/ilocalemanager.h"
#include "ccl/public/system/isignalhandler.h"
#include "ccl/public/system/isysteminfo.h"
#include "ccl/gui/popup/popupselector.h"
#include "ccl/gui/controls/editbox.h"
#include "ccl/main/cclargs.h"

#include "ccl/platform/cocoa/cclcocoa.h"
#include "ccl/platform/cocoa/cclcarbon.h"
#include "ccl/platform/cocoa/interfaces/icocoalocalemanager.h"
#include "ccl/platform/cocoa/gui/exceptionhandler.h"
#include "ccl/platform/cocoa/gui/nativeview.mac.h"

#include <CoreGraphics/CoreGraphics.h>

#include <IOKit/IOKitLib.h>
#include <IOKit/hidsystem/IOHIDLib.h>
#include <IOKit/hidsystem/IOHIDParameter.h>

#include <crt_externs.h>

#include "ccl/public/base/ccldefpush.h"

using namespace CCL;

int modalState = 0;
void modalStateEnter () { modalState++; }
void modalStateLeave () { modalState--; }

bool inQuitEvent = false;

//************************************************************************************************
// CustomApplication
//************************************************************************************************

@interface CCL_ISOLATED (CustomApplication): NSApplication
{
	id<NSObject> _activityToken;
}

@property (nonatomic, strong) id<NSObject>activityToken;

@end

//************************************************************************************************
// MainController
//************************************************************************************************

@interface CCL_ISOLATED (MainController): NSObject<NSApplicationDelegate>
{
	Url fileToOpen;
	bool openFileOnLaunch;
	bool uiInitialized;
}

- (void)applicationDidFinishLaunching:(NSNotification*)aNotification;
- (void)applicationDidBecomeActive:(NSNotification*)aNotification;
- (void)applicationDidResignActive:(NSNotification*)aNotification;
- (NSApplicationTerminateReply)applicationShouldTerminate:(NSApplication*)sender;
- (void)keyDown:(NSEvent*)nsEvent;
- (void)menuTrackingStarted:(NSNotification*)aNotification;
- (BOOL)applicationShouldHandleReopen:(NSApplication*)theApplication hasVisibleWindows:(BOOL)flag;
- (void)handleGetURLEvent:(NSAppleEventDescriptor*)event withReplyEvent:(NSAppleEventDescriptor*)replyEvent;
- (void)applicationWillSuspend:(NSNotification*)aNotification;
- (void)applicationDidResume:(NSNotification*)aNotification;

@end

//************************************************************************************************
// TimerCallback
//************************************************************************************************

@interface CCL_ISOLATED (TimerCallback): NSObject
{
}
- (void)timerCallback:(NSObject*)info;
- (void)service;
@end

//************************************************************************************************
// Helper functions
//************************************************************************************************

struct EventTypeHelper
{
	static bool isMouseEvent (NSEvent* event);
	static bool isKeyEvent (NSEvent* event);
	static NSEventModifierFlags modifierFlagsFromEventFlags (CGEventFlags cgFlags);
};

//////////////////////////////////////////////////////////////////////////////////////////////////

bool EventTypeHelper::isMouseEvent (NSEvent* event)
	{
	switch([event type])
	{
	case NSEventTypeLeftMouseDown:
	case NSEventTypeLeftMouseUp:
	case NSEventTypeRightMouseDown:
	case NSEventTypeRightMouseUp:
	case NSEventTypeMouseMoved:
	case NSEventTypeLeftMouseDragged:
	case NSEventTypeRightMouseDragged:
	case NSEventTypeMouseEntered:
	case NSEventTypeMouseExited:
	case NSEventTypeOtherMouseDown:
	case NSEventTypeOtherMouseUp:
	case NSEventTypeOtherMouseDragged:
	case NSEventTypeCursorUpdate:
	case NSEventTypeScrollWheel:
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool EventTypeHelper::isKeyEvent (NSEvent* event)
{
	switch([event type])
	{
	case NSEventTypeKeyDown:
	case NSEventTypeKeyUp:
	case NSEventTypeFlagsChanged:
		return true;
	}	
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

NSEventModifierFlags EventTypeHelper::modifierFlagsFromEventFlags (CGEventFlags cgFlags)
{
	NSEventModifierFlags flags = 0;

	if(cgFlags & kCGEventFlagMaskAlphaShift)
		flags |= NSEventModifierFlagCapsLock;

	if(cgFlags & kCGEventFlagMaskShift)
		flags |= NSEventModifierFlagShift;
	
	if(cgFlags & kCGEventFlagMaskControl)
		flags |= NSEventModifierFlagControl;
	
	if(cgFlags & kCGEventFlagMaskAlternate)
		flags |= NSEventModifierFlagOption;
	
	if(cgFlags & kCGEventFlagMaskCommand)
		flags |= NSEventModifierFlagCommand;

	if(cgFlags & kCGEventFlagMaskNumericPad)
		flags |= NSEventModifierFlagNumericPad;

	if(cgFlags & kCGEventFlagMaskHelp)
		flags |= NSEventModifierFlagHelp;
	
	if(cgFlags & kCGEventFlagMaskSecondaryFn)
		flags |= NSEventModifierFlagFunction;

	return flags;
}
	
namespace CCL {

//************************************************************************************************
// CocoaSystemTimer
//************************************************************************************************

class CocoaSystemTimer: public SystemTimer
{
public:
	CocoaSystemTimer (unsigned int period);
};

//************************************************************************************************
// CocoaUserInterface
//************************************************************************************************

class CocoaUserInterface: public UserInterface
{
public:
	CocoaUserInterface ();
	~CocoaUserInterface ();

	void onActivate ();
	void onDeactivate ();
	void setInSystemStartup (bool newValue) { inSystemStartup = newValue; }
	void setInSystemShutdown (bool newValue) { inSystemShutdown = newValue;	}

	CLASS_INTERFACES (UserInterface)

protected:
	NSTimer* systemTimer;
	CCL_ISOLATED (TimerCallback)* callBack;
	NSArray* nibObjects;
	CCL_ISOLATED (MainController)* mainController;
	bool inSystemStartup;
	bool inSystemShutdown;
	bool inFlushWindowEvents;

	// UserInterface
	bool startupPlatform (ModuleRef module) override;
	void shutdownPlatform () override;
	void quitPlatform () override;
	tbool CCL_API activateApplication (tbool startupMode, ArgsRef args) override;

	int CCL_API runEventLoop () override;
	void runModalLoop (IWindow* window, tbool& loopTerminated) override;

	tbool CCL_API flushUpdates (tbool wait = true) override;
	tbool CCL_API flushWindowEvents (IWindow* window) override;

	void CCL_API getKeyState (KeyState& keys) const override;
	tresult CCL_API detectKeyPressed (VirtualKey vkey, uchar character) const override;

	Point& CCL_API getMousePosition (Point& pos) const override;
	void CCL_API setMousePosition (const Point& pos) override;

	double CCL_API getDoubleClickDelay () const override;

	bool detectDrag (View* view, const Point& where) override;	
	bool detectDoubleClick (View* view, const Point& where) override;
	void tryDoubleClick () override;
	void resetCursor () override;

	void updateNativeUserActivity () override;
	void realizeActivityMode (ActivityMode mode) override;

	tresult CCL_API simulateEvent (const GUIEvent& event) override;
	ITimer* CCL_API createTimer (unsigned int period) const override;
};

//////////////////////////////////////////////////////////////////////////////////////////////////

CocoaUserInterface cocoaGUI;
UserInterface& GUI = cocoaGUI;

} // namespace CCL

//************************************************************************************************
// CocoaUserInterface
//************************************************************************************************

CocoaUserInterface::CocoaUserInterface ()
: systemTimer (nil),
  callBack (nil),
  nibObjects (nil),
  mainController (nil),
  inSystemStartup (true),
  inSystemShutdown (false),
  inFlushWindowEvents (false)
{
	buttonOrder = Styles::kAffirmativeButtonRight;
	roundedWindowCornersSupported = true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CocoaUserInterface::~CocoaUserInterface ()
{
#if EXCEPTION_HANDLER_ENABLED
	CocoaExceptionHandler::instance ().cleanupInstance ();
#endif
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API CocoaUserInterface::queryInterface (UIDRef iid, void** ptr)
{
#if EXCEPTION_HANDLER_ENABLED
	if(iid == ccl_iid<IDiagnosticDataProvider> ())
		return CocoaExceptionHandler::instance ().queryInterface (iid, ptr);
#endif
	return UserInterface::queryInterface (iid, ptr);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CocoaUserInterface::onActivate ()
{
	tooltipSuspended = false;
	GUI.onAppStateChanged (IApplication::kAppActivated);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CocoaUserInterface::onDeactivate ()
{
	tooltipSuspended = true;
	GUI.onAppStateChanged (IApplication::kAppDeactivated);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool CocoaUserInterface::startupPlatform (ModuleRef module)
{
#if EXCEPTION_HANDLER_ENABLED
	// install exception handler for main application
	if(module)
		CocoaExceptionHandler::instance ().install ();
#endif
	
	Bitmap::setResolutionNamingMode (Bitmap::kMultiResolution);
	
	ASSERT (callBack == 0)
	callBack = [[CCL_ISOLATED (TimerCallback) alloc] init];
	ASSERT (systemTimer == 0)
	systemTimer = [NSTimer scheduledTimerWithTimeInterval:0.010 target:callBack selector:@selector(timerCallback:) userInfo:nil repeats:true];
	[systemTimer setTolerance:0.010];
	[[NSRunLoop currentRunLoop] addTimer:systemTimer forMode:NSDefaultRunLoopMode];
	[[NSRunLoop currentRunLoop] addTimer:systemTimer forMode:NSEventTrackingRunLoopMode];
	[[NSRunLoop currentRunLoop] addTimer:systemTimer forMode:NSModalPanelRunLoopMode];

	bool ownProcess = module == System::GetMainModuleRef ();
	if(!ownProcess)
		return true;

	[CCL_ISOLATED (CustomApplication) sharedApplication];
	ASSERT (nibObjects == 0)
	[[NSBundle mainBundle] loadNibNamed:@"MainMenu" owner:NSApp topLevelObjects:&nibObjects];
	[nibObjects retain];

	ASSERT (mainController == 0)
	mainController = [[CCL_ISOLATED (MainController) alloc] init];
	[(NSApplication*)NSApp setDelegate: mainController];
	[[NSWorkspace sharedWorkspace].notificationCenter addObserver:mainController selector:@selector(applicationWillSuspend:) name:NSWorkspaceWillSleepNotification object:nil];
	[[NSWorkspace sharedWorkspace].notificationCenter addObserver:mainController selector:@selector(applicationDidResume:) name:NSWorkspaceDidWakeNotification object:nil];
	
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CocoaUserInterface::shutdownPlatform ()
{
	[[NSWorkspace sharedWorkspace].notificationCenter removeObserver:mainController];
	
	if(systemTimer)
		[systemTimer invalidate];
	systemTimer = nil;
	
	if(callBack)
		[callBack release];
	callBack = nil;
	
	if(nibObjects)
		[nibObjects release];
	nibObjects = nil;
	
	if(mainController)
		[mainController release];
	mainController = nil;
	
#if EXCEPTION_HANDLER_ENABLED
	CocoaExceptionHandler::instance ().uninstall ();
#endif
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CocoaUserInterface::runEventLoop ()
{
	if(!finishStartup ())
		return kExitError;

	ScopedVar<bool> scope (eventLoopRunning, true);
	
	if(![[NSApplication sharedApplication] mainMenu])
	{
		// command line tool without GUI
		[[NSApp delegate] applicationDidFinishLaunching:[NSNotification notificationWithName:NSApplicationDidFinishLaunchingNotification object:nil]];
		while(!inQuitEvent)
			[[NSRunLoop currentRunLoop] runUntilDate:[NSDate dateWithTimeIntervalSinceNow:0.001]];
	}
	else
		[NSApp performSelectorOnMainThread:@selector(run) withObject:nil waitUntilDone:YES];

	GUI.onAppStateChanged (IApplication::kAppTerminates);

	return exitCode;
}
	
//////////////////////////////////////////////////////////////////////////////////////////////////

void CocoaUserInterface::runModalLoop (IWindow* _window, tbool& loopTerminated)
{
	OSXWindow* window = OSXWindow::cast (unknown_cast<Window> (_window));
	NSWindow* nsWindow = toNSWindow (window);
	NSModalSession session = 0;
	
	if(window->getStyle ().isCustomStyle (Styles::kWindowBehaviorPopupSelector))
		window->onActivate (true);
	else if([NSApp isRunning])
		session = [NSApp beginModalSessionForWindow:nsWindow];
	
	while(!loopTerminated)
	{
		if(session)
			[NSApp runModalSession:session];
		NSEvent* nsEvent = [NSApp nextEventMatchingMask:NSEventMaskAny untilDate:[NSDate dateWithTimeIntervalSinceNow:0.025] inMode:NSDefaultRunLoopMode dequeue:YES];
		if(nsEvent == nil)
			SystemTimer::serviceTimers ();
		else
		{
			if(EventTypeHelper::isMouseEvent (nsEvent))
			{
				NSWindow* clickedWindow = [nsEvent window];
				NSPoint loc = [nsEvent locationInWindow];
				loc = [clickedWindow convertRectToScreen:NSMakeRect (loc.x, loc.y, 0, 0)].origin;
				loc = [nsWindow convertRectFromScreen:NSMakeRect (loc.x, loc.y, 0, 0)].origin;
				Point where ((Coord)loc.x, window->getSize ().getHeight () - (Coord)loc.y);
				
				switch([nsEvent type])
				{
					case NSEventTypeLeftMouseDown:
					case NSEventTypeRightMouseDown:
					case NSEventTypeOtherMouseDown:
					{
						//CCL_PRINT ("Mouse Down Event received\n")
						if(clickedWindow != nsWindow)
						{
							// Give popupselector a chance to disappear
							window->onActivate (false);
							nsEvent = nil;
						}
					}	break;
						
					case NSEventTypeLeftMouseUp:
					case NSEventTypeRightMouseUp:
					case NSEventTypeOtherMouseUp:
					{
						//CCL_PRINT ("Mouse Up Event received\n")
						if(clickedWindow != nsWindow)
						{
							window->onMouseUp (MouseEvent (MouseEvent::kMouseUp, where, KeyState (), nsEvent.timestamp));
							nsEvent = nil;
						}
					}	break;
						
					case NSEventTypeMouseMoved:
					case NSEventTypeMouseEntered:
					case NSEventTypeMouseExited:
					case NSEventTypeRightMouseDragged:
					case NSEventTypeLeftMouseDragged:
					case NSEventTypeOtherMouseDragged:
					{
						if(clickedWindow != nsWindow)
						{
							//CCL_PRINT ("Mouse Move  Event received\n")
							KeyState keys;
							getKeyState (keys);
							window->onMouseMove (MouseEvent (MouseEvent::kMouseMove, where, keys, nsEvent.timestamp));
							nsEvent = nil;
						}
					}
				}
			}
			else if(EventTypeHelper::isKeyEvent (nsEvent))
			{
				// When the popupselector is active, the window underneath remains the key window
				if(!NativeTextControl::isNativeTextControlPresent ())
				{
					if(window->getStyle ().isCustomStyle (Styles::kWindowBehaviorPopupSelector))
					{
						KeyEvent key;
						VKey::fromSystemEvent (key, SystemEvent (nsEvent));
						switch (key.eventType)
						{
							case KeyEvent::kKeyDown:
								window->onKeyDown (key);
							case KeyEvent::kKeyUp:
								window->onKeyUp (key);
						}
						nsEvent = nil;
					}
					else if([nsEvent window] != nsWindow)
						nsEvent = nil;
				}
			}
			else if([nsEvent type] == NSEventTypeAppKitDefined && [nsEvent subtype] == NSEventSubtypeApplicationDeactivated)
			{
				window->onActivate (false);
			}
			
			[NSApp sendEvent:nsEvent];
		}
	}
	window->onActivate (false);
	
	CCL_PRINT ("Modal loop terminated\n")
	if(session != 0)
		[NSApp endModalSession:session];
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CocoaUserInterface::flushUpdates (tbool wait)
{
	CCL_PROFILE_START (flushUpdates)

	NSTimeInterval delay = 0.001;
	if(wait)
	{
		int64 diff = System::GetSystemTicks () - lastUpdateTime;
		if(diff < kUpdateDelay)
			delay = (kUpdateDelay - diff) / 1000.;
	}

	if(inSystemStartup)
		SystemTimer::serviceTimers ();
	[[NSRunLoop currentRunLoop] runUntilDate:[NSDate dateWithTimeIntervalSinceNow:delay]];
	lastUpdateTime = (unsigned int) System::GetSystemTicks ();

	if([NSGraphicsContext currentContextDrawingToScreen] == YES)
		for(NSWindow* window in [NSApp windows])
			[window displayIfNeeded];

	CCL_PROFILE_STOP (flushUpdates)
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API CocoaUserInterface::flushWindowEvents (IWindow* _window)
{
	if(inFlushWindowEvents)
		return false;
	
	static int64 lastFlush = 0;
	int64 now = System::GetSystemTicks ();
	if(now - lastFlush < 25)
		return false;

	lastFlush = now;
	
	ScopedVar<bool> guard (inFlushWindowEvents, true);
	CCL_PROFILE_START (flushWindowEvents)
	Window* window = unknown_cast<Window> (_window);
	NSWindow* nsWindow = window ? toNSWindow (window) : nil;
	
	int eventMask = NSEventMaskLeftMouseDown | NSEventMaskLeftMouseUp | NSEventMaskKeyDown | NSEventMaskKeyUp;
	NSEvent* nsEvent = [NSApp nextEventMatchingMask:eventMask untilDate:[NSDate dateWithTimeIntervalSinceNow:0.001] inMode:NSDefaultRunLoopMode dequeue:YES];
	if(nsEvent == nil)
	{
		SystemTimer::serviceTimers ();
	}
	else
	{
		if(EventTypeHelper::isMouseEvent (nsEvent))
		{
			NSWindow* clickedWindow = [nsEvent window];
			if(clickedWindow == nsWindow)
				[NSApp sendEvent:nsEvent];
		}
		else if(EventTypeHelper::isKeyEvent (nsEvent))
		{
			if([nsEvent window] != nsWindow)
				return true;
			
			KeyEvent key;
			VKey::fromSystemEvent (key, SystemEvent (nsEvent));
			if(key.vKey != VKey::kEscape)
				return true;
	
			[NSApp sendEvent:nsEvent];
		}
	}
	CCL_PROFILE_STOP (flushWindowEvents)
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CocoaUserInterface::quitPlatform ()
{
	inQuitEvent = true;
	if(!inSystemShutdown)
	{
		// need a Cocoa event for NSApp stop to work, see Apple docu for NSApplication
		[NSApp postEvent:[NSEvent otherEventWithType:NSEventTypeApplicationDefined location:NSMakePoint (0, 0) modifierFlags:0 timestamp:0.0 windowNumber:0 context:nil subtype:0 data1:0 data2:0] atStart:NO];
		[NSApp stop:nil];
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API CocoaUserInterface::activateApplication (tbool startupMode, ArgsRef args)
{
	if(startupMode)
		return false;
	
	if(![[NSRunningApplication currentApplication] isActive])
		return [[NSRunningApplication currentApplication] activateWithOptions:NSApplicationActivateAllWindows | NSApplicationActivateIgnoringOtherApps];
	else
		return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API CocoaUserInterface::detectKeyPressed (VirtualKey virtualKey, uchar character) const
{
	CGKeyCode vkey = 0;
	if(virtualKey != VKey::kUnknown)
	{
		switch(virtualKey)
		{
		case VKey::kTab:		vkey = kVK_Tab; break;
		case VKey::kReturn:		vkey = kVK_Return; break;

		case VKey::kShift:		vkey = kVK_Shift; break;
		case VKey::kCommand:	vkey = kVK_Command; break;
		case VKey::kOption:		vkey = kVK_Option; break;
		case VKey::kControl:	vkey = kVK_Control; break;

		case VKey::kEscape:		vkey = kVK_Escape; break;
		case VKey::kSpace:		vkey = kVK_Space; break;
		case VKey::kHome:		vkey = kVK_Home; break;
		case VKey::kEnd:		vkey = kVK_End; break;

		case VKey::kLeft:		vkey = kVK_LeftArrow; break;
		case VKey::kUp:			vkey = kVK_UpArrow; break;
		case VKey::kRight:		vkey = kVK_RightArrow; break;
		case VKey::kDown:		vkey = kVK_DownArrow; break;
		case VKey::kPageUp:		vkey = kVK_PageUp; break;
		case VKey::kPageDown:	vkey = kVK_PageDown; break;

		case VKey::kDelete:		vkey = kVK_Delete; break;
	
		case VKey::kAdd:		vkey = kVK_ANSI_KeypadPlus; break;
		case VKey::kSubtract:	vkey = kVK_ANSI_KeypadMinus; break;

		default:
			return kResultNotImplemented;
		}
	}
	else
	{
		UnknownPtr<ICocoaLocaleManager> cocoaLocaleManager (&System::GetLocaleManager ());
		if(!cocoaLocaleManager || !cocoaLocaleManager->getSystemKeyForCharacter (vkey, character))
			return kResultNotImplemented;
	}

	return CGEventSourceKeyState (kCGEventSourceStateCombinedSessionState, vkey) ? kResultTrue : kResultFalse;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API CocoaUserInterface::getKeyState (KeyState& keys) const
{
	keys = 0;
	if(NSEvent* currentEvent = [NSApp currentEvent])
		VKey::fromSystemModifiers (keys, (unsigned int)[currentEvent modifierFlags]);
	else
		// fall back for program startup time
		VKey::fromSystemModifiers (keys, (unsigned int)EventTypeHelper::modifierFlagsFromEventFlags (CGEventSourceFlagsState (kCGEventSourceStateCombinedSessionState)));
	
	NSUInteger mouseButtons = [NSEvent pressedMouseButtons];
	if(mouseButtons & 1<<0)
		keys.keys |= KeyState::kLButton;
	if(mouseButtons & 1<<2)
		keys.keys |= KeyState::kMButton;
	if(mouseButtons & 1<<1)
		keys.keys |= KeyState::kRButton;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL::Point& CCL_API CocoaUserInterface::getMousePosition (CCL::Point& pos) const
{
	NSPoint mouseLoc = [NSEvent mouseLocation];
	pos ((Coord)mouseLoc.x, (Coord)([[[NSScreen screens] objectAtIndex:0] frame].size.height - mouseLoc.y));
	return pos;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API CocoaUserInterface::setMousePosition (const CCL::Point& pos)
{
	::CGPoint p = { static_cast<CGFloat>(pos.x), static_cast<CGFloat>(pos.y) };
	::CGWarpMouseCursorPosition (p);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

double CCL_API CocoaUserInterface::getDoubleClickDelay () const
{
	return [NSEvent doubleClickInterval];
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool CocoaUserInterface::detectDrag (View* view, const Point& where)
{
	static int kDragRange = 3;
	
	Window* window = view->getWindow ();
	if(!window)
		return false;
	NSWindow* nsWindow = toNSWindow (window);
	if(!nsWindow)
		return false;
	
	Point where2 (where);
	view->clientToWindow (where2);
	Rect dragRect (where2.x - kDragRange, where2.y - kDragRange, where2.x + kDragRange, where2.y + kDragRange);
	
	int mask = NSEventMaskLeftMouseUp | NSEventMaskRightMouseUp | NSEventMaskOtherMouseUp | NSEventMaskLeftMouseDragged | NSEventMaskRightMouseDragged | NSEventMaskOtherMouseDragged | NSEventMaskTabletPoint;
    while(true)
	{
		NSEvent *nextEvent = [NSApp nextEventMatchingMask:mask untilDate:[NSDate dateWithTimeIntervalSinceNow:10] inMode:NSEventTrackingRunLoopMode dequeue:NO];
		switch ([nextEvent type])
		{
		case NSEventTypeLeftMouseDragged:
			{
				NSPoint loc = [nextEvent locationInWindow];
				CGFloat windowHeight = [[nsWindow contentView] frame].size.height;
				loc.y = windowHeight - loc.y;
				if(dragRect.pointInside (Point ((Coord)loc.x, (Coord)loc.y)) == false)
				{
					nextEvent = [NSApp nextEventMatchingMask:mask untilDate:[NSDate dateWithTimeIntervalSinceNow:10] inMode:NSEventTrackingRunLoopMode dequeue:YES];
					return true;
				}
			}
			break;
		case NSEventTypeTabletPoint:
			{
				NSPoint loc = [NSEvent mouseLocation];
				loc = [nsWindow convertRectFromScreen:NSMakeRect (loc.x, loc.y, 0, 0)].origin;
				CGFloat windowHeight = [[nsWindow contentView] frame].size.height;
				loc.y = windowHeight - loc.y;
				if(dragRect.pointInside (Point ((Coord)loc.x, (Coord)loc.y)) == false)
				{
					nextEvent = [NSApp nextEventMatchingMask:mask untilDate:[NSDate dateWithTimeIntervalSinceNow:10] inMode:NSEventTrackingRunLoopMode dequeue:YES];
					return true;
				}
			}
			break;
		case NSEventTypeLeftMouseUp:
		case NSEventTypeRightMouseUp:
		case NSEventTypeOtherMouseUp:
		case NSEventTypeRightMouseDragged:
		case NSEventTypeOtherMouseDragged:
			return false;
		default:
			break;
		}
		nextEvent = [NSApp nextEventMatchingMask:mask untilDate:[NSDate dateWithTimeIntervalSinceNow:10] inMode:NSEventTrackingRunLoopMode dequeue:YES];
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool CocoaUserInterface::detectDoubleClick (View* view, const Point& where)
{
	if(doubleClicked > kDoubleClickReset)
		return doubleClicked == kDoubleClickTrue;
	
	doubleClicked = kDoubleClickFalse;
	
	if(!view || !view->getWindow())
		return false;

	NSWindow* nsWindow = toNSWindow (view->getWindow ());
	Point pos (where);
	if(view)
		view->clientToWindow (pos);
	Rect clickRect (pos.x - 2, pos.y - 2, pos.x + 2, pos.y + 2);

	NSUserDefaults* defaults = [NSUserDefaults standardUserDefaults];
	float doubleClickTime = 0.25;
	id object = [defaults objectForKey: @"com.apple.mouse.doubleClickThreshold"];
	if(object && [object floatValue] < 0.25)
		doubleClickTime = [object floatValue];
	
	NSEventMask eventMask = NSEventMaskLeftMouseDown | NSEventMaskLeftMouseDragged;
	NSDate* timeout = [NSDate dateWithTimeIntervalSinceNow:doubleClickTime];
	while(true)
	{
		NSEvent* nextEvent = [nsWindow nextEventMatchingMask:eventMask untilDate:timeout inMode:NSEventTrackingRunLoopMode dequeue:YES];
		if(!nextEvent)
			break;
		if(!view->getWindow ())
			break; // might have been detached meanwhile
		
		NSPoint loc = [nextEvent locationInWindow];
		bool stillInside = clickRect.pointInside (Point ((Coord)loc.x, (Coord)(view->getWindow ()->getSize ().getHeight () - loc.y)));
		if(!stillInside)
			break;
		else if(nextEvent.type == NSEventTypeLeftMouseDown)
		{
			doubleClicked = kDoubleClickTrue;
			break;
		}
	}
	return doubleClicked == kDoubleClickTrue;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CocoaUserInterface::tryDoubleClick ()
{
	NSEvent* nsEvent = [NSApp currentEvent];
	if(nsEvent == nil)
		return;
	NSEventType type = [nsEvent type];
	if(type != NSEventTypeLeftMouseDown && type != NSEventTypeLeftMouseUp)
		return;
	
	NSUserDefaults* defaults = [NSUserDefaults standardUserDefaults];
	float doubleClickTime = 0.25;
	id object = [defaults objectForKey: @"com.apple.mouse.doubleClickThreshold"];
	if(object && [object floatValue] < 0.25)
		doubleClickTime = [object floatValue];
	
	if([NSApp nextEventMatchingMask:NSEventMaskLeftMouseDown untilDate:[NSDate dateWithTimeIntervalSinceNow:doubleClickTime] inMode:NSEventTrackingRunLoopMode dequeue:NO] != nil)
		doubleClicked = kDoubleClickPending;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CocoaUserInterface::resetCursor ()
{
	[[NSCursor arrowCursor] set];
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CocoaUserInterface::updateNativeUserActivity ()
{
	if(System::GetSystem ().isProcessSandboxed ())
		return;
		
	// reset the user idle timer
	if(io_connect_t service = IOServiceGetMatchingService (kIOMasterPortDefault, IOServiceMatching (kIOHIDSystemClass)))
	{
		io_connect_t client = 0;
		kern_return_t result = IOServiceOpen (service, mach_task_self (), kIOHIDParamConnectType, &client);
		if(result == KERN_SUCCESS && MACH_PORT_VALID (client))
		{
			result = IOHIDSetStateForSelector (client, kIOHIDActivityUserIdle, 0);
			ASSERT (result == KERN_SUCCESS)
			IOServiceClose (client);
		}
		IOObjectRelease (service);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CocoaUserInterface::realizeActivityMode (ActivityMode mode)
{
	CCL_ISOLATED (CustomApplication)* app = (CCL_ISOLATED (CustomApplication)*)NSApp;
	ASSERT (app)
	
	switch(mode)
	{
	case ActivityMode::kNormal:
		if(app.activityToken != 0)
		{
			[[NSProcessInfo processInfo] endActivity:app.activityToken];
			app.activityToken = 0;
		}			
		break;
	case ActivityMode::kBackground:
		if(app.activityToken == 0)
			app.activityToken = [[NSProcessInfo processInfo] beginActivityWithOptions:(NSActivityBackground | NSActivityIdleSystemSleepDisabled) reason:@"CCL activity"];
		break;
	case ActivityMode::kAlwaysOn:
		if(app.activityToken == 0)
			app.activityToken = [[NSProcessInfo processInfo] beginActivityWithOptions:(NSActivityUserInitiatedAllowingIdleSystemSleep & ~NSActivitySuddenTerminationDisabled) reason:@"CCL activity"];
		break;
	default:
			break;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API CocoaUserInterface::simulateEvent (const GUIEvent& event)
{
	if(event.eventClass == GUIEvent::kMouseEvent)
	{
		const MouseEvent& mouseEvent = reinterpret_cast<const MouseEvent&> (event);
		CGPoint point = CGPointMake (mouseEvent.where.x, mouseEvent.where.y);
		bool isLeftClick = mouseEvent.keys.isSet (KeyState::kLButton);
		bool isRightClick = mouseEvent.keys.isSet (KeyState::kRButton);
		CGMouseButton button = kCGMouseButtonLeft;
		if(isRightClick)
			button = kCGMouseButtonRight;	
		CGEventType type = kCGEventNull;
		switch(mouseEvent.eventType)
		{
		case MouseEvent::kMouseDown :
			if(isRightClick)
				type = kCGEventRightMouseDown;
			else
				type = kCGEventLeftMouseDown;
			break;
		case MouseEvent::kMouseMove :
			if(mouseEvent.dragged == 1)
			{
				if(isRightClick)
					type = kCGEventRightMouseDragged;
				else
					type = kCGEventLeftMouseDragged;						
			}
			else
				type = kCGEventMouseMoved;
			break;
		case MouseEvent::kMouseUp :
			if(isRightClick)
				type = kCGEventRightMouseUp;
			else
				type = kCGEventLeftMouseUp;				
			break;								
		}
		CGEventRef event = CGEventCreateMouseEvent (NULL, type, point, button);
		CGEventSetType (event, type);
		CGEventPost (kCGHIDEventTap, event);
		CFRelease (event);
		return kResultOk;
	}
	else
		return kResultNotImplemented;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ITimer* CCL_API CocoaUserInterface::createTimer (unsigned int period) const
{
	return NEW CocoaSystemTimer (period);
}

//************************************************************************************************
// CocoaSystemTimer
//************************************************************************************************

CocoaSystemTimer::CocoaSystemTimer (unsigned int period)
: SystemTimer (period)
{
	systemTimer = this;
}

//************************************************************************************************
// CCLCustomApplication
//************************************************************************************************

@implementation CCL_ISOLATED (CustomApplication)

@synthesize activityToken = _activityToken;

//////////////////////////////////////////////////////////////////////////////////////////////////

- (id)init
{
    if(self = [super init])
		_activityToken = 0;
	
	return self;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (void)dealloc
{
	[_activityToken release];
	[super dealloc];
}

@end

//************************************************************************************************
// CCLTimerCallback
//************************************************************************************************

@implementation CCL_ISOLATED (TimerCallback)

- (void)timerCallback:(NSObject*)info
{
	if(!GUI.isTimerBlocked ())
		[self performSelectorOnMainThread:@selector(service) withObject:nil waitUntilDone:NO];
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (void)service
{
	if(!GUI.isTimerBlocked ())
		SystemTimer::serviceTimers ();
}

@end

//************************************************************************************************
// CCLMainController
//************************************************************************************************

@implementation CCL_ISOLATED (MainController)

- (id)init
{
	if(self = [super init])
	{
		openFileOnLaunch = false;
		uiInitialized = false;
	}
	return self;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (void)applicationDidFinishLaunching:(NSNotification*)aNotification
{
	CCL_PRINTLN ("App did finish launching")
	NSRunLoop* theRL = [NSRunLoop currentRunLoop];
	[theRL runMode:NSDefaultRunLoopMode beforeDate:[NSDate distantFuture]];

	NSAppleEventManager* appleEventManager = [NSAppleEventManager sharedAppleEventManager];
	[appleEventManager setEventHandler:self andSelector:@selector(handleGetURLEvent:withReplyEvent:) forEventClass:kInternetEventClass andEventID:kAEGetURL];

	if(IApplicationProvider* appProvider = GUI.getApplicationProvider ())
		if(!appProvider->onInit ())
		{
			GUI.blockTimer (true);
			[NSApp stop:self];
			return;
		}
	
	if(IApplication* application = GUI.getApplication ())
	{
		if(openFileOnLaunch)
			application->openFile (fileToOpen);

		GUI.onAppStateChanged (IApplication::kUIInitialized);
		uiInitialized = true;
	}
	
	cocoaGUI.setInSystemStartup (false);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (void)applicationDidBecomeActive:(NSNotification*)aNotification
{
	cocoaGUI.onActivate ();
	
	// reset key modifier state
	NSEvent* modifierEvent = [NSEvent keyEventWithType:NSEventTypeFlagsChanged location:NSMakePoint (0, 0) modifierFlags:[[NSApp currentEvent] modifierFlags] timestamp:0 windowNumber:[[NSApp mainWindow] windowNumber] context:nil characters:@"" charactersIgnoringModifiers:@"" isARepeat:NO keyCode:0];
	[NSApp postEvent:modifierEvent atStart:NO];
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (void)applicationDidResignActive:(NSNotification*)aNotification
{
	cocoaGUI.onDeactivate ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (void)menuTrackingStarted:(NSNotification*)aNotification
{
	if(IWindow* popup = Desktop.getTopWindow (kPopupLayer))
		if(unknown_cast<PopupSelectorWindow> (popup))
			popup->close ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (NSApplicationTerminateReply)applicationShouldTerminate:(NSApplication*)sender
{
	// this gets called when macOS triggers the app terminate (e.g. system shutdown or via dock context menu)

	// keep ourself alive during this call
	[[self retain] autorelease];

	IApplication* application = GUI.getApplication ();
	if(application)
	{
		cocoaGUI.setInSystemShutdown (true);
		if(application->requestQuit ())
		{
			GUI.onAppStateChanged (IApplication::kAppTerminates);
			GUI.onExit ();
			return NSTerminateNow;
		}
		else
		{
			cocoaGUI.setInSystemShutdown (false);
			return NSTerminateCancel;
		}
	}
	return NSTerminateNow;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (BOOL)application:(NSApplication*)sender openFile:(NSString*)_fileName
{
	CCL_PRINTLN ("App openFile")
	String pathString;
	pathString.appendNativeString (_fileName);
	fileToOpen.fromDisplayString (pathString);
	IApplication* application = GUI.getApplication ();
	if(application && uiInitialized)
		application->openFile (fileToOpen);
	else
	{
		// if there are command line arguments, the file is opened via IApplication::processCommandLine
		int* argc = _NSGetArgc ();
		if(argc && *argc > 1)
			return YES;
			
		openFileOnLaunch = true;
	}

	return YES;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (void)keyDown:(NSEvent*)nsEvent
{
	CCL_PRINTF ("Global Key %x\n", [NSApp keyWindow])
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (BOOL)applicationShouldHandleReopen:(NSApplication*)theApplication hasVisibleWindows:(BOOL)flag
{
	if(flag)
		return YES;
	if([NSApp mainWindow])
		return YES;
	
	// no windows anywhere: open main window like on application start up
	if(IApplication* application = GUI.getApplication ())
	{
		application->processCommandLine (ArgumentList ());
		return NO;
	}
	
	return YES;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (void)handleGetURLEvent:(NSAppleEventDescriptor*)event withReplyEvent:(NSAppleEventDescriptor*)replyEvent
{
	if(IApplication* application = GUI.getApplication ())
	{
		NSAppleEventDescriptor* directObjectDescriptor = [event paramDescriptorForKeyword:keyDirectObject];
		NSString* nsString = [directObjectDescriptor stringValue];
		String urlString;
		urlString.appendNativeString (nsString);
		fileToOpen.setUrl (urlString);
		application->openFile (fileToOpen);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (void)applicationWillSuspend:(NSNotification*)aNotification
{
	GUI.onAppStateChanged (IApplication::kAppSuspended);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (void)applicationDidResume:(NSNotification*)aNotification
{
	GUI.onAppStateChanged (IApplication::kAppResumed);
}

@end
