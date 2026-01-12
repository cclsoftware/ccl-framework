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
// Filename    : ccl/platform/cocoa/iosapp/contentview.mm
// Description : iOS View to CCL form bridge
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "ccl/platform/cocoa/iosapp/contentview.h"
#include "ccl/platform/cocoa/iosapp/gesturerecognizer.h"
#include "ccl/platform/cocoa/quartz/cghelper.h"
#include "ccl/platform/cocoa/skia/skiaengine.cocoa.h"
#include "ccl/platform/cocoa/gui/accessibility.ios.h"

#include "ccl/gui/graphics/nativegraphics.h"
#include "ccl/gui/graphics/graphicsdevice.h"
#include "ccl/gui/theme/thememanager.h"
#include "ccl/gui/touch/touchinput.h"
#include "ccl/gui/touch/touchcollection.h"
#include "ccl/gui/system/dragndrop.h"
#include "ccl/gui/system/accessibility.h"
#include "ccl/gui/controls/editbox.h"
#include "ccl/gui/gui.h"
#include "ccl/gui/keyevent.h"

#include "ccl/public/math/mathprimitives.h"
#include "ccl/public/systemservices.h"
#include "ccl/public/gui/framework/controlsignals.h"

#define LOG_DRAW 0 && DEBUG_LOG
#define LOG_GESTURES 0 && DEBUG_LOG

using namespace CCL;

//////////////////////////////////////////////////////////////////////////////////////////////////

#if LOG_GESTURES
MutableCString getRecognizerName (UIGestureRecognizer* recognizer)
{
	NSString* cl = [[recognizer class] description];
	String className;
	className.appendNativeString (cl);
	className.remove (0, 2);
	className.remove (className.index ("GestureRecognizer"), 17);

	if([recognizer isKindOfClass:[UITapGestureRecognizer class]])
		if(((UITapGestureRecognizer*)recognizer).numberOfTapsRequired == 2)
			className.prepend ("Double ");

	return className;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void logGesture (UIGestureRecognizer* gesture, UIView* view, StringID text)
{
    MutableCString state;
    switch([gesture state])
    {
	case UIGestureRecognizerStatePossible:  state = "possible"; break;
	case UIGestureRecognizerStateBegan:     state = "began"; break;
	case UIGestureRecognizerStateChanged:   state = "changed"; break;
	case UIGestureRecognizerStateEnded:     state = "ended"; break;
	case UIGestureRecognizerStateCancelled: state = "cancelled"; break;
	case UIGestureRecognizerStateFailed:    state = "failed"; break;
    }
    
    CGPoint loc = [gesture locationInView:view];
    CCL::Point where ((int)loc.x, (int)loc.y);
    CCL_PRINTF ("%s %s \"%s\" [%s] (%d, %d) %d touches", text.str (), CCL_DEBUG_ID (gesture), getRecognizerName (gesture).str (), state.str (), where.x, where.y, gesture.numberOfTouches)
	
	if([gesture isKindOfClass:[UIPanGestureRecognizer class]])
	{
		CGPoint velocity = [(UIPanGestureRecognizer*)gesture velocityInView:[gesture view]];
		//CCL_PRINTF (" velocity: (%f, %f)",  velocity.x, velocity.y)
	}
	CCL_PRINTLN ("")
}
#define LOG_GESTURE(gesture,text) logGesture (gesture, self, text);
#else
#define LOG_GESTURE(gesture,text)
#endif

//************************************************************************************************
// ContentView
//************************************************************************************************

@implementation CCL_ISOLATED (ContentView)

+ (Class)layerClass
{
	if(CCL::MacOS::MetalGraphicsInfo::instance ().isSkiaEnabled ())
		return [CAMetalLayer class];
	else
		return [CALayer class];
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (id)initWithFrame:(CGRect)rect 
{
	self = [super initWithFrame: rect];
    [self initializeView];    
    return self;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (id)initWithCoder:(NSCoder *)decoder
{
	self = [super initWithCoder: decoder];
    [self initializeView]; 
    return self;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (void)initializeView
{
	self.clearsContextBeforeDrawing = NO;
    self.multipleTouchEnabled = YES;

	recognizerManager = NEW GestureRecognizerManager;
	recognizerManager->init (self);

	UIPencilInteraction* pencilInteraction = [[[UIPencilInteraction alloc] init] autorelease];
	pencilInteraction.delegate = self;
	[self addInteraction:pencilInteraction];
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (void)setWindow:(Window*)_window
{
	window = _window;
	if(AccessibilityManager::isEnabled () == false)
	{
		self.accessibilityTraits |= UIAccessibilityTraitAllowsDirectInteraction;
		self.isAccessibilityElement = true;
	}

	if(window)
		window->getTouchInputState ().setGestureManager (return_shared (recognizerManager));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (void)dealloc
{
	if(recognizerManager)
		recognizerManager->release ();

	[super dealloc];
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (void)drawRect:(CGRect)dirtyRect 
{
	if(!window)
		return;
	
	@try 
	{
        #if LOG_DRAW
		CCL_PRINTF ("draw  %4d %4d %4d %4d\n", (int)dirtyRect.origin.x, (int)dirtyRect.origin.y, (int)dirtyRect.size.width, (int)dirtyRect.size.height)
        #endif
		
		CCL::Rect updateRect;
		MacOS::fromCGRect (updateRect, dirtyRect);
		window->setInDrawEvent (true);
		window->draw (UpdateRgn (updateRect, 0));		
		window->setInDrawEvent (false);
	}
	@catch(NSException* e) 
	{
		CCL_WARN ("NSException in drawRect\n", 0)
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline TouchID makeTouchID (UITouch* touch)
{
	return (TouchID)(void*)touch;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline int64 makeTouchTime (UITouch* touch)
{
	return (int64)([touch timestamp] * 1000);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (CCL::Point)makeTouchLocation:(UITouch*)touch
{
	CGPoint loc = [touch locationInView:self];
	return CCL::Point ((int)loc.x, (int)loc.y);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (CCL::PointF)makeTouchLocationF:(UITouch*)touch
{
	CGPoint loc = [touch locationInView:self];
	return CCL::PointF ((float)loc.x, (float)loc.y);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (CCL::PointerEvent::PenInfo)makePenInfo:(UITouch*)touch
{
	CCL::PointerEvent::PenInfo penInfo;
	CGFloat maxForce = [touch maximumPossibleForce];
	if(maxForce > 0)
		penInfo.pressure = (float)([touch force] / maxForce);
	penInfo.tiltX = (float)Math::radToDegrees ([touch azimuthAngleInView:self]);
	penInfo.tiltY = (float)Math::radToDegrees ([touch altitudeAngle]);
	return penInfo;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (void)processTouches:(NSSet *)touches withEvent:(UIEvent *)event
{
	if(!window)
		return;
	
	NSSet* allTouches = event.allTouches;
	
	TouchCollection touchCollection;
	int eventType = TouchEvent::kMove;
	NSUInteger numTouches = [allTouches count];
	NSUInteger upCount = 0;

	for(UITouch* touch in allTouches)
	{
		int type = 0;

		TouchID id = makeTouchID (touch);
		int64 time = makeTouchTime (touch);
		CCL::Point where ([self makeTouchLocation:touch]);

		switch([touch phase])
		{
		case UITouchPhaseBegan:
			type = TouchEvent::kBegin;
			eventType = TouchEvent::kBegin;
			CCL_PRINTF ("touch began: %s (%d, %d)\n", CCL_DEBUG_ID (touch), where.x, where.y);
			break;
		case UITouchPhaseMoved:
		case UITouchPhaseStationary:
			type = TouchEvent::kMove;
			break;
		case UITouchPhaseEnded:
		case UITouchPhaseCancelled:
			type = TouchEvent::kEnd;
			upCount++;
			CCL_PRINTF ("touch end:   %s (%d, %d)\n", CCL_DEBUG_ID (touch), where.x, where.y);
			break;
		}

		TouchInfo info (type, id, where, time);

        if([touch type] == UITouchTypePencil)
        {
			// handle pen in a separate collection
			TouchCollection penCollection;
			penCollection.add (info);

			TouchEvent touchEvent (penCollection, type, KeyState (), TouchEvent::kPenInput);
			touchEvent.eventTime = System::GetProfileTime ();
			touchEvent.penInfo = [self makePenInfo: touch];
			window->getTouchInputState ().processTouches (touchEvent);
			continue;
        }
		
		touchCollection.add (info);
	}

    if(touchCollection.isEmpty ())
        return;

    if(upCount == numTouches)
		eventType = TouchEvent::kEnd;
	
	TouchEvent touchEvent (touchCollection, eventType, KeyState (), TouchEvent::kTouchInput);
	touchEvent.eventTime = System::GetProfileTime ();
	window->getTouchInputState ().processTouches (touchEvent);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (void)touchesBegan:(NSSet*)touches withEvent:(UIEvent*)event
{
	[self processTouches: touches withEvent: event];
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (void)touchesMoved:(NSSet*)touches withEvent:(UIEvent*)event
{
	[self processTouches: touches withEvent: event];
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (void)touchesEnded:(NSSet*)touches withEvent:(UIEvent*)event
{
	[self processTouches: touches withEvent: event];
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (void)touchesCancelled:(NSSet *)touches withEvent:(UIEvent*)event
{
	[self processTouches: touches withEvent: event];
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//	Press events
//////////////////////////////////////////////////////////////////////////////////////////////////

- (BOOL)processPresses:(NSSet<UIPress*>*)presses withEvent:(UIPressesEvent*)event
{
	if(window == nullptr || NativeTextControl::isNativeTextControlPresent ())
		return NO;

	bool handled = false;
	for(UIPress* press in presses)
	{
		KeyEvent key;
		VKey::fromSystemEvent (key, SystemEvent (press));

		switch(key.eventType)
		{
		case KeyEvent::kKeyDown:
			handled = window->onKeyDown (key);
			break;
		case KeyEvent::kKeyUp:
			handled = window->onKeyUp (key);
			break;
		}
	}

	return handled;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (void)pressesBegan:(NSSet<UIPress*>*)presses withEvent:(UIPressesEvent*)event
{
	if(![self processPresses:presses withEvent:event])
		[super pressesBegan:presses withEvent:event];
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (void)pressesEnded:(NSSet<UIPress*>*)presses withEvent:(UIPressesEvent*)event
{
	if(![self processPresses:presses withEvent:event])
		[super pressesEnded:presses withEvent:event];
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (CCL::GestureEvent)makeGestureEvent:(UIGestureRecognizer*)recognizer type:(int)eventType
{
    static CGPoint lastTranslate;   // the last value
    static CGPoint prevTranslate;   // the value before that one
    static NSTimeInterval lastTime;
    static NSTimeInterval prevTime;
	
	int state = GestureEvent::kChanged;
    switch([recognizer state])
    {
	case UIGestureRecognizerStateBegan:			state = GestureEvent::kBegin; break;
	case UIGestureRecognizerStateChanged:		state = GestureEvent::kChanged; break;
	case UIGestureRecognizerStateEnded:			state = GestureEvent::kEnd; break;
	case UIGestureRecognizerStateCancelled:
	case UIGestureRecognizerStateFailed:		state = GestureEvent::kFailed; break;
	case UIGestureRecognizerStatePossible:		state = GestureEvent::kChanged; break;
    }

	CGPoint loc = [recognizer locationInView:self];
	CCL::PointF where ((float)loc.x, (float)loc.y);

	GestureEvent event (eventType|state, where);
	CCL::Point mousePos (pointFToInt (where));
	GUI.setLastMousePos (window->clientToScreen (mousePos));

	switch(event.getType ())
	{
	case GestureEvent::kSwipe:
		{
			CGPoint velocity = CGPointZero;
			CGPoint translate = [(UIPanGestureRecognizer*)recognizer translationInView:[recognizer view]];
			if(recognizer.state == UIGestureRecognizerStateBegan)
			{
				lastTime = [NSDate timeIntervalSinceReferenceDate];
				lastTranslate = translate;
				prevTime = lastTime;
				prevTranslate = lastTranslate;
				
				velocity = [(UIPanGestureRecognizer*)recognizer velocityInView:[recognizer view]];
			}
			else if(recognizer.state == UIGestureRecognizerStateChanged)
			{
				NSTimeInterval now = [NSDate timeIntervalSinceReferenceDate];
				
				// ignore change at same position during a certain time tolerance (to avoid end velocity 0)
				bool ignore = CGPointEqualToPoint (translate, lastTranslate) && now - lastTime < 0.1;
				if(!ignore)
				{
					prevTime = lastTime;
					prevTranslate = lastTranslate;
					lastTime = now;
					lastTranslate = translate;
				}

				velocity = [(UIPanGestureRecognizer*)recognizer velocityInView:[recognizer view]];
			}
			else if(recognizer.state == UIGestureRecognizerStateEnded)
			{
				NSTimeInterval seconds = [NSDate timeIntervalSinceReferenceDate] - prevTime;
				if(seconds != 0)
					velocity = CGPointMake ((CGFloat)((translate.x - prevTranslate.x) / seconds), (CGFloat)((translate.y - prevTranslate.y) / seconds));
			}

			CGFloat factor = recognizer.view.contentScaleFactor; // from points to pixels
			event.amountX = (float)(velocity.x / factor);
			event.amountY = (float)(velocity.y / factor);
		}
		break;

	case GestureEvent::kSingleTap:
	case GestureEvent::kDoubleTap:
		event.eventType = eventType | GestureEvent::kBegin;
		break;
	case GestureEvent::kZoom:
		event.amountX = (float)((UIPinchGestureRecognizer*) recognizer).scale;
		event.amountY = event.amountX;
		break;
	}
			
	return event;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (CCL::PointF)calculateTouchCenter:(UIGestureRecognizer*)recognizer
{
	CCL::PointF center;

	int numTouches = (int)[recognizer numberOfTouches];
	for(int t = 0; t < numTouches; t++)
	{
		CGPoint loc = [recognizer locationOfTouch:t inView:self];
		center += PointF ((float)loc.x, float(loc.y));
	}
	if(numTouches > 0)
		center *= (1.f / numTouches);

	return center;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (IBAction)onLongPressGesture:(UILongPressGestureRecognizer*)recognizer
{
	if(!window)
		return;
	
	if(GestureInfo* gesture = recognizerManager->getGesture (recognizer))
	{
		// note: in "began", a drag operation might start (handled above)
		LOG_GESTURE (recognizer, "Gesture: ");
		GestureEvent event ([self makeGestureEvent:recognizer type:GestureEvent::kLongPress]);
		window->getTouchInputState ().onGesture (event, *static_cast<Gesture*> (gesture));
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (IBAction)onTapGesture:(UITapGestureRecognizer*)recognizer
{
	if(!window)
		return;
	
	if(GestureInfo* gesture = recognizerManager->getGesture (recognizer))
	{
		LOG_GESTURE (recognizer, "Gesture: ");

		// Note: we don't get notified about the "Began" state
		if([recognizer state] == UIGestureRecognizerStateEnded)
		{
			GestureEvent event ([self makeGestureEvent:recognizer type:GestureEvent::kSingleTap]);
			window->getTouchInputState ().onGesture (event, *static_cast<Gesture*> (gesture));
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (IBAction)onDoubleTapGesture:(UITapGestureRecognizer*)recognizer
{
	if(!window)
		return;
	
	if(GestureInfo* gesture = recognizerManager->getGesture (recognizer))
	{
		LOG_GESTURE (recognizer, "Gesture: ");
		
		if([recognizer state] == UIGestureRecognizerStateEnded)
		{
			GestureEvent event ([self makeGestureEvent:recognizer type:GestureEvent::kDoubleTap]);
			window->getTouchInputState ().onGesture (event, *static_cast<Gesture*> (gesture));
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (IBAction)onSwipeGesture:(UISwipeGestureRecognizer*)recognizer
{
	if(!window)
		return;
	
	if(GestureInfo* gesture = recognizerManager->getGesture (recognizer))
	{
		LOG_GESTURE (recognizer, "Gesture: ");
		// ...
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (IBAction)onPanGesture:(UIPanGestureRecognizer*)recognizer
{
	if(!window)
		return;
	
	if(auto gesture = static_cast<Gesture*> (recognizerManager->getGesture (recognizer)))
	{
		LOG_GESTURE (recognizer, "Gesture: ");
		GestureEvent event ([self makeGestureEvent:recognizer type:GestureEvent::kSwipe]);

		// leave shadow state when only 1 touch of the competing zoom gesture remains (zoom gesture will become shadow)
		if(gesture->isShadow ()
			&& event.getState () == GestureEvent::kChanged
			&& window->getTouchInputState ().countRemainingShadowTouches (*gesture) == 1)
			{
				event.eventType = (event.eventType & ~GestureEvent::kStatesMask) | GestureEvent::kBegin;
			}

		window->getTouchInputState ().onGesture (event, *gesture);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (IBAction)onPinchGesture:(UIPinchGestureRecognizer*)recognizer
{
	if(!window)
		return;

	RecognizerItem* item = recognizerManager->findRecognizerItem (recognizer);
	if(auto gesture = item ? static_cast<Gesture*> (item->getGesture ()) : nullptr)
	{
		LOG_GESTURE (recognizer, "Gesture: ");
		GestureEvent event ([self makeGestureEvent:recognizer type:GestureEvent::kZoom]);
		
		// the "locationInView" reported by the iOS recognizer only delivers integer coord values (although the type is double)
		// for better resolution, we calculate the center of all touches in the gesture (in floating point coords)
		event.setPosition ([self calculateTouchCenter: recognizer]);

		if(event.getState () == GestureEvent::kBegin)
			item->setStartAmount (event);
		
		// leave shadow state when 2 touches are present (swipe gesture will become shadow)
		if(gesture->isShadow ()
			&& event.getState () == GestureEvent::kChanged
			&& recognizer.numberOfTouches >= 2)
			{
				item->setStartAmount (event);
				event.eventType = (event.eventType & ~GestureEvent::kStatesMask) | GestureEvent::kBegin;
			}

		// amount relative to value when gesture started (again)
		event.amountX = event.amountX / item->getStartAmountX ();
		event.amountY = event.amountY / item->getStartAmountY ();
		
		window->getTouchInputState ().onGesture (event, *gesture);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (BOOL)gestureRecognizer:(UIGestureRecognizer*)recognizer shouldReceiveTouch:(UITouch*)touch
{
	if(!window)
		return NO;
	
	//only accept touches originating from this view
	if([touch view] != self)
		return NO;
		
	// note: this is called before touchesBegan
	// before we can decide to which CCL::Gesture the touch belongs, we need to anticipate a touch begin event,
	// if the touch is not already tracked by TouchInputState
	TouchID touchID = makeTouchID (touch);
	if(!window->getTouchInputState ().hasTouch (touchID))
	{
		int64 time = makeTouchTime (touch);
		CCL::Point where ([self makeTouchLocation:touch]);

		// feed the new touch into the CCL::TouchInputState
		TouchInfo info (TouchEvent::kBegin, touchID, where, time);
		TouchInputState::TouchEventData eventData (info.type);
		eventData.inputDevice = TouchEvent::kTouchInput;
		if([touch type] == UITouchTypePencil)
		{
			eventData.inputDevice = TouchEvent::kPenInput;
			eventData.penInfo = [self makePenInfo: touch];
		}

		window->getTouchInputState ().processTouch (info, eventData);
	}

	// only accept touch if it belongs to the corresponding CCL::Gesture
	GestureInfo* gesture = recognizerManager->getGesture (recognizer);
	BOOL result = gesture && gesture->wantsTouch (touchID);
	
	#if LOG_GESTURES
	if(gesture) // ignore spare recognizers
	CCL_PRINTF ("%s \"%s\" shouldReceive [%s]: %s\n", CCL_DEBUG_ID (recognizer), getRecognizerName (recognizer).str (), CCL_DEBUG_ID (touch), result ? "true" : "false")
	#endif

	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (BOOL)gestureRecognizer:(UIGestureRecognizer*)recognizer shouldRecognizeSimultaneouslyWithGestureRecognizer:(UIGestureRecognizer *)otherRecognizer
{
	auto allowsCompetingGesture = [] (Gesture& gesture, Gesture& otherGesture) -> bool
	{
		// allow competing multi-touch gesture to take over from gesture with "exclusive touch"
		if((gesture.isExclusiveTouch ())
			&& gesture.getType () == GestureEvent::kSwipe
			&& otherGesture.getType () == GestureEvent::kZoom)
				return true;

		// try the already accepted handler
		int otherType = otherGesture.getType ();
		if(gesture.getHandler ())
			return gesture.getHandler ()->allowsCompetingGesture (otherType);
			
		// try all pending candidate handlers
		UnknownList handlers;
		gesture.getCandidateHandlers (handlers);
		ForEachUnknown (handlers, unk)
			UnknownPtr<ITouchHandler> handler (unk);
			if(handler && handler->allowsCompetingGesture (otherType))
				return true;
		EndFor
		return false;
	};

	auto gesture = static_cast<Gesture*> (recognizerManager->getGesture (recognizer));
	auto otherGesture = static_cast<Gesture*> (recognizerManager->getGesture (otherRecognizer));

	bool explicitelyAllowed = gesture && otherGesture
		&& (allowsCompetingGesture (*gesture, *otherGesture) || allowsCompetingGesture (*otherGesture, *gesture));

	BOOL result = YES;

	if(!explicitelyAllowed)
	{
		// the TouchInputState class can not process gestures with shared touched
		for(int i = 0; i < [recognizer numberOfTouches]; i++)
		{
			CGPoint p = [recognizer locationOfTouch:i inView:nil];
			for(int j = 0; j < [otherRecognizer numberOfTouches]; j++)
				if(CGPointEqualToPoint (p, [otherRecognizer locationOfTouch:j inView:nil]))
				{
					result = NO;
					break;
				}
		}
	}

	#if LOG_GESTURES
	if(gesture && otherGesture) // ignore spare recognizers
	CCL_PRINTF ("%s \"%s\" shouldRecognizeWith %s \"%s\": %s\n", CCL_DEBUG_ID (recognizer), getRecognizerName (recognizer).str (),
		CCL_DEBUG_ID (otherRecognizer), getRecognizerName (otherRecognizer).str (), result ? "true" : "false")
	#endif
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (BOOL)gestureRecognizerShouldBegin:(UIGestureRecognizer*)recognizer
{
	LOG_GESTURE (recognizer, "gestureShouldBegin: ");
	return !DragSession::isInternalDragActive () && recognizerManager->getGesture (recognizer) != 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (void)safeAreaInsetsDidChange
{
	ThemeManager::instance ().onSystemMetricsChanged ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (void)pencilInteractionDidTap:(UIPencilInteraction*)interaction
{
	GestureEvent event (GestureEvent::kPenPrimary | GestureEvent::kBegin);
	window->onGesture (event);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (void)layoutSublayersOfLayer:(CALayer*)layer
{
	[super layoutSublayersOfLayer:layer];
	if([[UIApplication sharedApplication] applicationState] != UIApplicationStateBackground)
		if(window)
			if(NativeWindowRenderTarget* target = window->getRenderTarget ())
				target->onSize ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (NSArray*)accessibilityElements
{
	if(!window)
		return nil;

	AccessibilityProvider* provider = window->getAccessibilityProvider ();
	if(!provider)
		return nil;

	iOS::UIAccessibilityElementProvider* nativeProvider = iOS::UIAccessibilityElementProvider::toPlatformProvider (provider);
	if(!nativeProvider)
		return nil;

	return nativeProvider->getChildren ();
}

@end
