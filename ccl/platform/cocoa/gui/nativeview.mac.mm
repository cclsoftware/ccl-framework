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
// Filename    : ccl/platform/cocoa/gui/nativeview.mac.mm
// Description : Customized NSView, wrapped
//
//************************************************************************************************

#define DEBUG_LOG 0
#define DEBUG_UPDATES 0

#define USE_OFFSCREEN_TARGET 0

#include "ccl/platform/cocoa/gui/nativeview.mac.h"
#include "ccl/platform/cocoa/quartz/device.h"
#include "ccl/platform/cocoa/gui/dragndrop.cocoa.h"
#include "ccl/gui/windows/window.h"
#include "ccl/gui/windows/windowmanager.h"
#include "ccl/gui/windows/childwindow.h"
#include "ccl/gui/windows/desktop.h"
#include "ccl/gui/graphics/nativegraphics.h"
#include "ccl/gui/controls/pluginview.h"
#include "ccl/gui/controls/editbox.h"
#include "ccl/gui/views/mousehandler.h"
#include "ccl/gui/system/dragndrop.h"
#include "ccl/gui/system/systemtimer.h"
#include "ccl/gui/keyevent.h"
#include "ccl/gui/gui.h"
#include "ccl/platform/cocoa/quartz/nshelper.h"
#include "ccl/public/systemservices.h"
#include "ccl/public/gui/framework/idrawable.h"
#include "ccl/gui/graphics/imaging/image.h"
#include "ccl/gui/views/sprite.h"

#include "ccl/platform/cocoa/cclcocoa.h"
#include "ccl/platform/cocoa/gui/window.mac.h"
#include "ccl/platform/cocoa/gui/platformwindow.mac.h"

#include "ccl/public/base/ccldefpush.h"

#include <IOKit/IOKitLib.h>
#include <IOKit/hidsystem/IOHIDLib.h>
#include <IOKit/hidsystem/IOHIDParameter.h>

using namespace CCL;
using namespace MacOS;

extern DragSession* gInsideDrag;
extern NSArray* getDragTypes ();
extern int modalState;
extern Sprite* gDragSprite;

//************************************************************************************************
// NSWindow helper
//************************************************************************************************

NSWindow* CCL::MacOS::toNSWindow (const IWindow* window)
{
	return ((NSView*)window->getSystemWindow ()).window;
}

//************************************************************************************************
// CustomNSView
//************************************************************************************************

@interface CCL_ISOLATED (CustomNSView): NSView
{
	OSXWindow* window;
	DragSession* currentDragSession;
	bool flagsChangedFlag;
	CCL::Point lastMousePosition;
}

- (id)initWithFrame:(NSRect)frameRect frameworkWindow:(Window*)window;
- (void)onMouseDown:(NSEvent*)nsEvent;
- (void)onMouseUp:(NSEvent*)nsEvent;
- (void)onMouseMoved:(NSEvent*)nsEvent;
- (void)setFrameworkWindow:(Window*)window;
- (BOOL)acceptsFirstMouse:(NSEvent*)nsEvent;
- (void)mouseDown:(NSEvent*)nsEvent;
- (void)mouseDragged:(NSEvent*)nsEvent;
- (void)mouseUp:(NSEvent*)nsEvent;
- (void)mouseMoved:(NSEvent*)nsEvent;
- (void)mouseEntered:(NSEvent*)nsEvent;
- (void)mouseExited: (NSEvent*)nsEvent;
- (void)rightMouseDown:(NSEvent*)nsEvent;
- (void)rightMouseUp:(NSEvent*)nsEvent;
- (void)cursorUpdate:(NSEvent*)nsEvent;
- (void)drawRect:(NSRect)dirtyRect;
- (BOOL)isFlipped;
- (BOOL)keyEvent:(NSEvent*)nsEvent;
- (DragEvent)makeDragEvent:(id <NSDraggingInfo>)sender type:(int)type;
- (void)magnifyWithEvent:(NSEvent*)event;
- (void)gestureEvent:(NSEvent*)event withType:(int)type;
- (void)viewDidMoveToWindow;
@end

using namespace CCL;

//************************************************************************************************
// NativeView
//************************************************************************************************

NativeView::NativeView (NSView* _view, OSXWindow* _window)
: view (_view),
  window (_window)

{
	[view retain];
	view.clipsToBounds = YES;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

NativeView::~NativeView ()
{
	[view release];
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void NativeView::setLayer (CALayer* layer)
{
    view.layer = layer;
    view.wantsLayer = YES;
    view.layerContentsPlacement = NSViewLayerContentsPlacementTopLeft;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CALayer* NativeView::getLayer () const
{
    return view.layer;
}

//************************************************************************************************
// CustomView
//************************************************************************************************

CustomView::CustomView (OSXWindow* _window, const Rect& size)
: NativeView ([[CCL_ISOLATED (CustomNSView) alloc] initWithFrame:NSMakeRect (size.left, size.top, size.getWidth (), size.getHeight ()) frameworkWindow:_window], _window)
{
	[view release];
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CustomView::~CustomView ()
{
	[(CCL_ISOLATED (CustomNSView)*)view removeFromSuperview];
	[(CCL_ISOLATED (CustomNSView)*)view setFrameworkWindow:nullptr];
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool CustomView::embedInto (NSWindow* parent)
{
	CCL_PRINTF ("CustomView: Embedding %d into %d\n", view, parent);
	[parent setContentView:view];
	
	NSTrackingAreaOptions options = NSTrackingMouseEnteredAndExited |
	NSTrackingMouseMoved |
	NSTrackingCursorUpdate |
	NSTrackingInVisibleRect |
	NSTrackingActiveInActiveApp;
	NSTrackingArea* trackingArea = [[NSTrackingArea alloc] initWithRect:[view frame] options:options owner:view userInfo:nil];
	ASSERT ([[view trackingAreas] count] == 0)
	[view addTrackingArea:trackingArea];
	[trackingArea release];
	
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool CustomView::embedInto (NSView* parent)
{
	[parent addSubview:view];
	[view setAutoresizingMask:NSViewWidthSizable | NSViewHeightSizable];
	return true;
}

//************************************************************************************************
// CustomNSView implementation
//************************************************************************************************

@implementation CCL_ISOLATED (CustomNSView)

//////////////////////////////////////////////////////////////////////////////////////////////////

- (id)initWithFrame:(NSRect)frameRect frameworkWindow:(Window*)cclWindow
{
	self = [super initWithFrame:frameRect];
	if(self)
	{
		self->window = OSXWindow::cast (cclWindow);
		[self registerForDraggedTypes:getDragTypes ()];
		self->lastMousePosition = CCL::Point (0,0);
	}
	return self;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (void)dealloc
{
	[self unregisterDraggedTypes];
	[super dealloc];
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (void)setFrameworkWindow:(Window*) cclWindow
{
	self->window = OSXWindow::cast (cclWindow);
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//	Overrides
//////////////////////////////////////////////////////////////////////////////////////////////////

- (BOOL)isFlipped
{
    return YES;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (BOOL)acceptsFirstResponder
{
    return YES;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (BOOL)resignFirstResponder
{
	if(!WindowManager::instance ().shouldActivateWindows ())
	{
		// unless new windows should be activated, we deny giving up first responder status while a PlugInView is being attached
		// to the same window (in case a plug-in tries to become first responder)
		PlugInView* plugView = PlugInView::getAttachingView ();
		if(plugView && plugView->getWindow() == window)
			return NO;
	}
	return YES;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (BOOL)acceptsFirstMouse:(NSEvent*)nsEvent
{
    return YES;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (void)viewDidMoveToWindow
{
	if(NSWindow* nativeWindow = [self window])
	{
		[nativeWindow setAcceptsMouseMovedEvents:YES];
		[nativeWindow makeFirstResponder:self];
		if(window)
			window->setNativeWindow (nativeWindow);
	}
	
	if(window && [self window] == nil)
		window->onNativeViewRemoved ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//	Mouse events
//////////////////////////////////////////////////////////////////////////////////////////////////

- (void)mouseDown:(NSEvent*)nsEvent
{
	[self onMouseDown: nsEvent];
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (void)mouseDragged:(NSEvent*)nsEvent
{
	if(window == nullptr)
		return;
		
	int modifiers = (int)[nsEvent modifierFlags];
	NSPoint mouseLoc = [self convertPoint:[nsEvent locationInWindow] fromView:nil];
	
	CCL::Point p ((int)mouseLoc.x, (int)mouseLoc.y);
	MouseEvent event (MouseEvent::kMouseMove, p);
	event.eventTime = [nsEvent timestamp];
	VKey::fromSystemModifiers (event.keys, modifiers);
	
	//CCL_PRINTF ("Mouse mv %4d %4d\n", p.x, p.y)
	window->onMouseMove (event);
	GUI.processMouseMove (false);
		
	#if DEBUG_LOG
	static NSDate* time = nil;
	if(time)
	{
		CCL_PRINTF ("mouseDragged after %f ms\n" ,-[time  timeIntervalSinceNow]*1000.)
		[time release];
	}
	time = [[NSDate alloc] init];
	#endif
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (void)mouseUp:(NSEvent*)nsEvent
{
	[self onMouseUp: nsEvent];
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (void)mouseMoved:(NSEvent*)nsEvent
{
	if(![[self window] isKindOfClass:[CCL_ISOLATED (PlatformTooltip) class]])
		[self onMouseMoved: nsEvent];
	
	[self.nextResponder mouseMoved:nsEvent];
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (void)mouseEntered:(NSEvent*)nsEvent
{
	[self onMouseMoved:nsEvent];
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (void)mouseExited: (NSEvent*)nsEvent
{
	[self onMouseMoved:nsEvent];
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (void)rightMouseDown:(NSEvent*)nsEvent
{
	[self onMouseDown:nsEvent];
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (void)rightMouseUp:(NSEvent*)nsEvent
{
	[self onMouseUp:nsEvent];
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (void)otherMouseDown:(NSEvent*)nsEvent
{
	[self onMouseDown:nsEvent];
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (void)otherMouseUp:(NSEvent*)nsEvent
{
	[self onMouseUp:nsEvent];
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (void)otherMouseDragged:(NSEvent*)nsEvent
{
	[self onMouseMoved:nsEvent];
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (void)onMouseDown:(NSEvent*)nsEvent
{
	if(window == nullptr)
		return;
	
	int mouseButton = (int)[nsEvent buttonNumber];
	int modifiers = (int)[nsEvent modifierFlags];
	NSPoint mouseLoc = [self convertPoint:[nsEvent locationInWindow] fromView:nil];
		
	if(modifiers & NSEventModifierFlagControl)
	{
		mouseButton = 1;
		modifiers &= ~NSEventModifierFlagControl;
	}
	
	CCL::Point where ((int)mouseLoc.x, (int)mouseLoc.y);
	MouseEvent event (MouseEvent::kMouseDown, where);
	event.eventTime = [nsEvent timestamp];
	VKey::fromSystemModifiers (event.keys, modifiers);
	
	int myButton = 0;
	if(mouseButton == 0)
		myButton = KeyState::kLButton;
	else if(mouseButton == 1)
		myButton = KeyState::kRButton;
	else if(mouseButton == 2)
		myButton = KeyState::kMButton;
	event.keys.keys |= myButton;

	View* activeView = ccl_cast<View> (Desktop.getActiveWindow ());
	Window* activeWindow = activeView ? activeView->getWindow () : nullptr;
	if(activeWindow != window)
	{
		if(activeWindow)
			activeWindow->onActivate (false);

		if(window == nullptr)   // This can happen in onActivate
			return;
		window->onActivate (true);
	}

	CCL_PRINTF ("Mouse dn %4d %4d %d %x\n", (int)mouseLoc.x, (int)mouseLoc.y, mouseButton, [nsEvent window])
	if(window)
	{
		[self retain];
		window->setSuppressContextMenu (false);
		window->onMouseDown (event); // this can close the window (release "self" and set "window" to nullptr)
		if(window)
		{
			if((mouseButton == 1 || event.keys.isSet (KeyState::kControl)) && !window->isSuppressContextMenu ())
				window->popupContextMenu (where, false);
			window->setSuppressContextMenu (false);
		}
		[self release];
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (void)onMouseUp:(NSEvent*)nsEvent
{
	if(window == nullptr)
		return;
#if DEBUG_LOG
	int mouseButton = (int)[nsEvent buttonNumber];
#endif
	int modifiers = (int)[nsEvent modifierFlags];
	NSPoint mouseLoc = [self convertPoint:[nsEvent locationInWindow] fromView:nil];

	CCL::Point p ((int)mouseLoc.x, (int)mouseLoc.y);
	MouseEvent event (MouseEvent::kMouseUp, p);
	event.eventTime = [nsEvent timestamp];
	VKey::fromSystemModifiers (event.keys, modifiers);
	
	CCL_PRINTF ("Mouse up %4d %4d %d %x\n", p.x, p.y, mouseButton, [nsEvent window])
	window->onMouseUp (event);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (void)onMouseMoved:(NSEvent*)nsEvent
{
	if(window == nullptr)
		return;
		
	CCL::Point mousePosition;
	GUI.getMousePosition (mousePosition);
	window->screenToClient (mousePosition);
	
	bool contained = CGRectContainsPoint([self frame], CGPoint {static_cast<CGFloat>(mousePosition.x), static_cast<CGFloat>(mousePosition.y)});
	
	if(lastMousePosition == mousePosition || !contained || modalState)
	{
		// Skip mouse moves if a) location did not change (optimization), b) the cursor is outside of our view frame or
		// c) a modal dialog is open and our UI should not receive mouse overs meanwhile
		if(lastMousePosition != mousePosition)
		{
			CCL_PRINTF ("NSView: Mouse move skipped %4d %4d, contained = %s, modal = %d, view = %d\n", mousePosition.x, mousePosition.y, contained ? "yes" : "no", modalState, self)
		}
		return;
	}
	lastMousePosition = mousePosition;
	
	SharedPtr<Object> protector (window); // This avoids windows unintentionally deleting themselves during mouse events
	// CCL_PRINTF ("NSView: Mouse moved %4d %4d\n", mousePosition.x, mousePosition.y)
	GUI.onMouseMove (window, MouseEvent (MouseEvent::kMouseMove, mousePosition, GUI.getLastKeyState ()));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (void)cursorUpdate:(NSEvent*)nsEvent
{
	// This is called when the OS wants to change the cursor. We handle this event in our engine.
	GUI.updateCursor ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (void)scrollWheel:(NSEvent*)nsEvent
{
	static float latchAmountX = 0.f;
	static float latchAmountY = 0.f;
	bool rollOutPhase = false;
	
	if(window == nullptr)
		return;
	
	switch([nsEvent phase])
	{
		case NSEventPhaseBegan :
		{
			flagsChangedFlag = false;
			latchAmountX = 0.f;
			latchAmountY = 0.f;
			break;
		}
		case NSEventPhaseNone :
		{
			if([nsEvent momentumPhase] != NSEventPhaseNone) // rollout of Trackpad/Magic Mouse
			{
				rollOutPhase = true;
				if(flagsChangedFlag)
					return;
			}
			break;
		}
		case NSEventPhaseEnded:
		case NSEventPhaseChanged:
		case NSEventPhaseStationary:
		case NSEventPhaseMayBegin:
		{
			break;
		}
		case NSEventPhaseCancelled:
		{
			return;
		}
	}
			
	CCL::PointF scrollDelta ((float)[nsEvent scrollingDeltaX], (float)[nsEvent scrollingDeltaY]);
	
	latchAmountX = (latchAmountX * 0.9f) + (ccl_abs (scrollDelta.x) * 0.1f);
	latchAmountY = (latchAmountY * 0.9f) + (ccl_abs (scrollDelta.y) * 0.1f);
	
	// previous axis is preferred
	bool isHorizontal = ((latchAmountX + ccl_abs (scrollDelta.x)) >= (latchAmountY + ccl_abs (scrollDelta.y)));
	
	// create MouseWheelEvent
	int eventType;
	if(isHorizontal)
		eventType = scrollDelta.x < 0 ? MouseWheelEvent::kWheelRight : MouseWheelEvent::kWheelLeft;
	else
		eventType = scrollDelta.y > 0 ? MouseWheelEvent::kWheelUp : MouseWheelEvent::kWheelDown;

	NSPoint mouseLoc = [self convertPoint:[nsEvent locationInWindow] fromView:nil];
	
	int modifiers = (int)[nsEvent modifierFlags];
	KeyState keystate;
	VKey::fromSystemModifiers (keystate, modifiers);
	
	MouseWheelEvent event (eventType, CCL::Point ((int)mouseLoc.x, (int)mouseLoc.y), keystate);
	
	event.deltaX = scrollDelta.x;
	event.deltaY = scrollDelta.y;
	event.delta = isHorizontal ? event.deltaX : event.deltaY;

	event.eventTime = [nsEvent timestamp];
	
	if([nsEvent isDirectionInvertedFromDevice])
		event.wheelFlags |= MouseWheelEvent::kAxisInverted;
	if([nsEvent hasPreciseScrollingDeltas])
		event.wheelFlags |= MouseWheelEvent::kContinuous;
	if(rollOutPhase)
		event.wheelFlags |= MouseWheelEvent::kRollOutPhase;
	
	if(event.keys.isSet (KeyState::kShift))
	{
		if(event.isContinuous ())
			event.eventType = (event.eventType + 2) % 4;
		event.keys.keys &= ~KeyState::kShift;
		event.wheelFlags |= MouseWheelEvent::kAxisToggled;
	}
	
	if(event.delta != 0)
		window->onMouseWheel (event);
	
	SystemTimer::serviceTimers ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//	Gesture events
//////////////////////////////////////////////////////////////////////////////////////////////////

- (void)magnifyWithEvent:(NSEvent*)nsEvent
{	
	[self gestureEvent: nsEvent withType: GestureEvent::kZoom];
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (void)gestureEvent:(NSEvent*)nsEvent withType:(int)type
{
	if(window == nullptr)
		return;
	
	int modifiers = (int)[nsEvent modifierFlags];	
	NSPoint mouseLoc = [self convertPoint:[nsEvent locationInWindow] fromView:nil];
	CGFloat amount = [nsEvent magnification];
	KeyState keystate;
	VKey::fromSystemModifiers (keystate, modifiers);
	
	CCL::Point p ((int)mouseLoc.x, (int)mouseLoc.y);
	
	GestureEvent event (type, p, keystate);
	event.eventTime = [nsEvent timestamp];
	
	event.amountX = (float)amount;
	event.amountY = event.amountX;
	
	//	CCL_PRINTF ("gesture amount %f\n", event.amount)
	
	window->onGesture (event);
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//	Key events
//////////////////////////////////////////////////////////////////////////////////////////////////

- (BOOL)keyEvent:(NSEvent*)nsEvent
{
	if(window == nullptr)
		return NO;

	KeyEvent key;
	VKey::fromSystemEvent (key, SystemEvent (nsEvent));
	
	// send all key presses but the function keys to the active text edit
	if(NativeTextControl::isNativeTextControlPresent () && !(key.vKey >= VKey::kF1 && key.vKey <= VKey::kF24))
		return NO;

	bool handled = false;
	switch(key.eventType)
	{
		case KeyEvent::kKeyDown:
			handled = window->onKeyDown (key);
			break;
		case KeyEvent::kKeyUp:
			handled = window->onKeyUp (key);
			break;
	}
	
	if(handled && key.vKey == VKey::kCapsLock && key.eventType == KeyEvent::kKeyDown)
		setCapsLock (false);
	
	return handled;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (void)keyDown:(NSEvent*)nsEvent
{
	CCL_PRINTF ("keyDown in window %x\n", [NSApp keyWindow])

	if([self keyEvent:nsEvent] == NO)
		[super keyDown:nsEvent];
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (void)keyUp:(NSEvent*)nsEvent
{
	CCL_PRINTF ("keyUp in window %x\n", [NSApp keyWindow])

	if([self keyEvent:nsEvent] == NO)
		[super keyUp:nsEvent];
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (void)flagsChanged:(NSEvent*)nsEvent
{
	flagsChangedFlag = true;
	
	if([self keyEvent:nsEvent] == NO)
		[super flagsChanged:nsEvent];
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (BOOL)performKeyEquivalent:(NSEvent*)event
{
	CCL_PRINTF ("=======> performKeyEquivalent \n", 0)
		
	// Don't let an inactive ChildWindow receive key events (CCLContentView performKeyEquivalent just forwards to all child NSViews!)
	// Note: PlugInView::onActivate () passes the activation to its ChildWindow, but the implementation of OSXWindow::isActive does not reflect this
	if(ccl_cast<ChildWindow> (window) && !window->WindowBase::isActive ())
	{
		CCL_PRINTF ("==========> ignore inactive ChildWindow\n", 0)
		return NO;
	}
	
	if([super performKeyEquivalent:event] == NO)
	{
		// workaround: otherwise some key combinations with CTRL (e.g. CTRL-Tab) never reach our keyDown
		if([event type] == NSEventTypeKeyDown && !modalState)
			if([self keyEvent:event] == YES)
			{
				CCL_PRINTF ("==========> handled by us\n", 0)
				return YES;
			}
			else
			{
				// filter CMD-Q, do not allow for uncontrolled application quit
				NSString* key = [event charactersIgnoringModifiers];
				NSEventModifierFlags modifiers = [event modifierFlags] & NSEventModifierFlagDeviceIndependentFlagsMask;
				if([key isEqualToString:@"q"] && modifiers == NSEventModifierFlagCommand)
					return YES;
			}

		CCL_PRINTF ("==========> not handled\n", 0)
		return NO;
	}
	CCL_PRINTF ("==========> handled by super\n", 0)
	return YES;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void setCapsLock (bool state)
{
	CFMutableDictionaryRef hidService = IOServiceMatching (kIOHIDSystemClass);
	if(io_connect_t ioMasterPort = IOServiceGetMatchingService (kIOMasterPortDefault, (CFDictionaryRef)hidService))
	{
		io_connect_t hidParam = 0;
		kern_return_t result = IOServiceOpen (ioMasterPort, mach_task_self (), kIOHIDParamConnectType, &hidParam);
		IOObjectRelease (ioMasterPort);
		if(result == KERN_SUCCESS)
			IOHIDSetModifierLockState (hidParam, kIOHIDCapsLockState, state);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//	Drag and drop
//////////////////////////////////////////////////////////////////////////////////////////////////

- (DragEvent)makeDragEvent:(id<NSDraggingInfo>)sender type:(int)type
{		
	ASSERT (currentDragSession)

	NSPoint loc = [self convertPoint:[sender draggingLocation] fromView:nil];
	CCL::Point where ((Coord)loc.x, (Coord)loc.y);
	
	if(window)
		window->windowToClient (where);
	
	KeyState keystate;
	GUI.getKeyState (keystate);

	return DragEvent (*currentDragSession, type, where, keystate);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (NSDragOperation)draggingEntered:(id<NSDraggingInfo>)sender
{
	if(window == nullptr || modalState)
		return NSDragOperationNone;


	if(gInsideDrag)
	{
		ASSERT (currentDragSession == nullptr || currentDragSession == gInsideDrag)
		if(currentDragSession == nullptr)
		{
			currentDragSession = gInsideDrag;
			currentDragSession->retain ();
		}
	}
	else
	{
		ASSERT (currentDragSession == nullptr)
		currentDragSession = NEW CocoaDragSession (sender);
	}

	DragEvent dragEvent ([self makeDragEvent:sender type:DragEvent::kDragEnter]);
	bool accepted = window->onDragEnter (dragEvent);

	Image* dragImage = unknown_cast<Image> (currentDragSession->getDragImage ());
	if(dragImage)
	{
		AutoPtr<IDrawable> drawable (NEW ImageDrawable (dragImage, 0.7f));
		if(drawable)
		{
			CCL::Rect size;
			dragImage->getSize (size);
			if(gDragSprite)
			{
				gDragSprite->hide ();
				gDragSprite->release ();
			}
			gDragSprite = NEW FloatingSprite (window, drawable, size, ISprite::kKeepOnTop);
		}
	}
	
	currentDragSession->setDragImagePosition (dragEvent.where);
	currentDragSession->showNativeDragImage (!currentDragSession->hasVisualFeedback ());

	if(accepted)
		return [self draggingUpdated:sender];	

	return NSDragOperationNone;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (NSDragOperation)draggingUpdated:(id<NSDraggingInfo>)sender
{
	if(window == nullptr || modalState || currentDragSession == nullptr)
		return NSDragOperationNone;
	
	DragEvent dragEvent ([self makeDragEvent:sender type:DragEvent::kDragOver]);
	bool accepted = window->onDragOver (dragEvent);

	currentDragSession->setDragImagePosition (dragEvent.where);
	currentDragSession->showNativeDragImage (!currentDragSession->hasVisualFeedback ());

	if(accepted)
	{		
		if(currentDragSession->isDropCopyReal () || currentDragSession->isDropCopyShared ())
			return NSDragOperationCopy;
		else if(currentDragSession->isDropMove ())
			return NSDragOperationMove;
		else
			return NSDragOperationGeneric;
	}
	return NSDragOperationNone;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (void)draggingExited:(id<NSDraggingInfo>)sender
{
	if(window == nullptr || currentDragSession == nullptr)
		return;

	currentDragSession->showNativeDragImage (false);
	DragEvent dragEvent ([self makeDragEvent:sender type:DragEvent::kDragLeave]);
	window->onDragLeave (dragEvent); // this leads to DragSession::onDragFinished, which does clean up after a drop or if the session was cancelled
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (void)concludeDragOperation:(id<NSDraggingInfo>)sender
{
	if(window == nullptr || currentDragSession == nullptr)
		return;

    if(currentDragSession->getResult() != IDragSession::kDropNone || currentDragSession->getSourceResult () != IDragSession::kDropNone)
    {
        DragEvent dragEvent ([self makeDragEvent:sender type:DragEvent::kDrop]);
        window->onDrop (dragEvent);
    }
	else
		currentDragSession->setCanceled ();

	// cleanup after drop!
	[self draggingExited:sender];
	safe_release (currentDragSession);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (void)draggingEnded:(id<NSDraggingInfo>)sender
{
	// if the drag operation ends somewhere else, this method is called instead of concludeDragOperation
	if(currentDragSession == nullptr)
		return;

	currentDragSession->setCanceled ();

	// clean up after cancel
	[self draggingExited:sender];
	safe_release (currentDragSession);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (BOOL)prepareForDragOperation:(id <NSDraggingInfo>)sender
{
    return YES;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (BOOL)performDragOperation:(id <NSDraggingInfo>)sender 
{
    return YES;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (BOOL)wantsPeriodicDraggingUpdates
{
    return NO;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//	Drawing
//////////////////////////////////////////////////////////////////////////////////////////////////

- (void)drawRect:(NSRect)dirtyRect
{
	//CCL_PRINTF ("drawRect  %4d %4d %4d %4d\n", (int)dirtyRect.origin.x, (int)dirtyRect.origin.y, (int)dirtyRect.size.width, (int)dirtyRect.size.height)
	if(window == nullptr)
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
