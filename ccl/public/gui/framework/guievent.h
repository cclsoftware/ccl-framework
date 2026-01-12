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
// Filename    : ccl/public/gui/framework/guievent.h
// Description : GUI Events
//
//************************************************************************************************

#ifndef _ccl_guievent_h
#define _ccl_guievent_h

#include "ccl/public/gui/framework/keycodes.h"

#include "ccl/public/gui/graphics/point.h"

#include "core/public/gui/coremultitouch.h"

namespace CCL {

interface IContextMenu;
interface IDragSession;
interface ITooltipPopup;
interface ITouchCollection;
interface IColorScheme;

//************************************************************************************************
// GUIEvent
/** Basic GUI Event.
\ingroup gui_event  */
//************************************************************************************************

struct GUIEvent
{
	enum EventClass
	{
		kSystemEvent,
		kKeyEvent,
		kMouseEvent,
		kMouseWheelEvent,
		kGestureEvent,
		kTouchEvent,
		kFocusEvent,
		kDragEvent,
		kContextMenuEvent,
		kTooltipEvent,
		kWindowEvent,
		kViewEvent,
		kDisplayChangedEvent,
		kColorSchemeEvent
	};

	int eventClass;
	int eventType;
	double eventTime;

	GUIEvent (int eventClass, int eventType, double eventTime = 0)
	: eventClass (eventClass),
	  eventType (eventType),
	  eventTime (eventTime)
	{}

	template<class Event> const Event* as () const;

private:
	template<class Event> static EventClass getEventClass ();
};

//************************************************************************************************
// SystemEvent
//************************************************************************************************

/** System event class is private to framework. */
struct SystemEvent;

//************************************************************************************************
// KeyState
/** Mouse Button and Keyboard Modifier state.
\ingroup gui_event  */
//************************************************************************************************

struct KeyState
{
	enum Flags
	{
		kLButton		= 1<<0,		///< Left Mouse Button
		kMButton		= 1<<1,		///< Middle Mouse Button
		kRButton		= 1<<2,		///< Right Mouse Button

		kMouseMask = kLButton|kMButton|kRButton,

		kShift			= 1<<3,		///< [Shift]
		kCommand		= 1<<4,		///< [Ctrl on Windows, Apple on MacOS]
		kOption			= 1<<5,		///< [Alt on Windows, Option on MacOS]
		kControl		= 1<<6,		///< [Control key on MacOS only)
		
		kModifierMask = kShift|kCommand|kOption|kControl,

		kRepeat			= 1<<7,		///< flag for key repetition

		kPenBarrel		= 1<<8,		///< pen barrel button
		kPenEraser		= 1<<9,		///< pen eraser button
		kPenInverted	= 1<<10,	///< pen is inverted (e.g. to function as eraser)

		kPenMask = kPenBarrel|kPenEraser|kPenInverted,

		// Mouse Gestures (not part of regular key state)
		kClick			= 1<<16,	///< click (might be the beginning of a double-click)
		kDrag			= 1<<17,	///< drag gesture
		kDoubleClick	= 1<<18,	///< double-click gesture
		kSingleClick	= 1<<19,	///< single-click (when a double-click did not happen)
		kWheel			= 1<<20,	///< mouse wheel gesture

		kGestureMask = kDrag|kDoubleClick|kSingleClick|kWheel
	};

	unsigned int keys;

	KeyState (unsigned int keys = 0)
	: keys (keys)
	{}

	/** Check if mouse button or modifier is set. */
	bool isSet (int key) const;

	/** Get modifier keys. */
	int getModifiers () const;
	
	/** Cast to plain integer. */
	operator const unsigned int () const;

	/** Set modifiers from string. */
	bool fromString (StringRef string);

	/** Convert to Unicode string. */
	void toString (String& string, bool translated = false) const;
};

//************************************************************************************************
// KeyEvent
/**	Keyboard Event.

	Notes about composed characters and dead keys:
 
	'character' is the character produced by a single key press, possibly modified by the 'shift' modifier.
	Dead keys are not taken into account.

	'composedCharacter' is the character produced by a sequence of key presses, including dead keys and modifiers.
	On most platforms, 'composedCharacter' will only be valid for kKeyDown events.
 
	Examples (Windows, German locale):
 
	| Keys                      | character         | composedCharacter   |
	|---------------------------|-------------------|---------------------|
	| Alt Gr + E                | e                 | (euro sign)         |
	| Shift + E                 | E                 | E                   |
	| Alt Gr + Shift + (eszett) | ? (question mark) | (uppercase eszett)  |
	| (caret) followed by A     | a                 | (a with circumflex) |


	Repeat behavior:
 
	| macOS                     | Windows              | Linux               |
	|---------------------------|----------------------|---------------------|
	| Alphanumeric characters   | All characters are   | All characters are  |
	| and diacritics are not    | repeated. Characters | repeated.           |
	| repeated.                 | with diacritics are  |                     |
	|                           | repeated without     |                     |
	|                           | diacritics.          |                     |
 
* 
\ingroup gui_event  */
//************************************************************************************************

struct KeyEvent: GUIEvent
{
	enum EventType
	{
		kKeyDown,
		kKeyUp
	};
	
	VirtualKey vKey;
	uchar character;
	uchar composedCharacter;
	KeyState state;

	KeyEvent (int eventType = kKeyDown,
			  VirtualKey vKey = VKey::kUnknown,
			  uchar character = 0,
			  uchar composedCharacter = 0,
			  const KeyState& state = KeyState (),
			  double eventTime = 0)
	: GUIEvent (kKeyEvent, eventType, eventTime),
	  vKey (vKey),
	  character (character),
	  composedCharacter (composedCharacter),
	  state (state)
	{}

	bool isValid () const;
	bool isVKeyValid () const;
	bool isCharValid () const;
	bool isComposedCharValid () const;
	bool isRepeat () const;
	bool isSimilar (const KeyEvent&) const;
	bool operator == (const KeyEvent&) const;
	bool operator != (const KeyEvent&) const;
	
	bool fromString (StringRef string);
	void toString (String& string, bool translated = false) const;
};

//************************************************************************************************
// PointerEvent
/** Common properties of MouseEvent and TouchEvent.
\ingroup gui_event  */
//************************************************************************************************

struct PointerEvent: GUIEvent
{
	DEFINE_ENUM (InputDevice)
	{
		kPointerInput,	// generic / unknown
		kTouchInput,
		kPenInput,
		kMouseInput
	};

	struct PenInfo		// additional info for events originating from a pen
	{
		float tiltX;	///< tilt angle along the x-axis in degrees, positive values indicate a tilt to the right
		float tiltY;	///< tilt angle along the y-axis in degrees, positive values indicate a tilt toward the user
		float twist;	///< rotation / twist angle of the pen in degrees
		float pressure;	///< 0..1

		PenInfo (float tiltX = 0, float tiltY = 0, float twist = 0, float pressure = 0.f)
		: tiltX (tiltX),
		  tiltY (tiltY),
		  twist (twist),
		  pressure (pressure)
		{}
	};

	KeyState keys;
	InputDevice inputDevice;
	PenInfo penInfo;

	PointerEvent (int eventClass, int eventType, double eventTime = 0, const KeyState& keys = KeyState (), InputDevice inputDevice = kPointerInput)
	: GUIEvent (eventClass, eventType, eventTime),
	  keys (keys),
	  inputDevice (inputDevice)
	{}
};

//************************************************************************************************
// MouseEvent
/** Mouse Event. 
\ingroup gui_event  */
//************************************************************************************************

struct MouseEvent: PointerEvent
{
	enum EventType
	{
		kMouseDown,
		kMouseUp,
		kMouseEnter,
		kMouseMove,
		kMouseLeave
	};

	Point where;

	mutable int dragged;		///< -1: not checked yet, 0: not dragged, 1: dragged
	mutable int doubleClicked;	///< -1: not checked yet, 0: no doubleclick, 1: doubleClicked

	MouseEvent (int eventType = kMouseDown, 
				const Point& where = Point (),
				const KeyState& keys = KeyState (),
				double eventTime = 0)
	: PointerEvent (kMouseEvent, eventType, eventTime, keys, kMouseInput),
	  where (where),
	  dragged (-1),
	  doubleClicked (-1)
	{}

	tbool wasTouchEvent () const { return inputDevice == kTouchInput; }	///< mouse event originates from a touch event
	tbool wasPenEvent () const { return inputDevice == kPenInput; }		///< mouse event originates from a pen event
};

//************************************************************************************************
// MouseWheelEvent
/** Mouse Wheel Event. 
\ingroup gui_event  */
//************************************************************************************************

struct MouseWheelEvent: GUIEvent
{
	enum EventType
	{
		kWheelUp,
		kWheelDown,
		kWheelLeft,
		kWheelRight
	};

	Point where;
	KeyState keys;
	float delta;
	float deltaX;
	float deltaY;
	
	enum WheelFlags
	{
		kContinuous 	= 1<<0,		///< indicates that scrolling deltas are continuous (not rastered)
		kRollOutPhase 	= 1<<1,		///< indicates that continuous scroll-events belong to the rollout-phase.
		kAxisToggled 	= 1<<2,		///< indicates that the original axis was toggled between horizontal and vertical (when shift is pressed)
		kAxisInverted 	= 1<<3		///< indicates that the original axis direction was mirrored (inverted by device - e.g. Apple "natural scrolling")
	};
	int wheelFlags;

	MouseWheelEvent (int eventType = kWheelUp, 
					 const Point& where = Point (),
					 const KeyState& keys = KeyState (),
					 float distance = 1.f)
	: GUIEvent (kMouseWheelEvent, eventType),
	  where (where),
	  keys (keys),
	  delta (distance),
	  deltaX (0.f),
	  deltaY (0.f),
	  wheelFlags (0)
	{}
	
	bool isContinuous () const;
	bool isRollOutPhase () const;
	bool isAxisToggled () const;
	bool isAxisInverted () const;
	bool isVertical () const;
	bool isHorizontal () const;
	int getOriginalDirection () const; ///< returns original wheel direction, reverting the axisToggled flag
};

//************************************************************************************************
// GestureEvent
/** Gesture Event. 
\ingroup gui_event  */
//************************************************************************************************

struct GestureEvent: GUIEvent
{
	enum EventType
	{
		// Touch Gestures
		kSwipe     = Core::kGestureSwipe,
		kZoom      = Core::kGestureZoom,
		kRotate    = Core::kGestureRotate,
        kLongPress = Core::kGestureLongPress,
        kSingleTap = Core::kGestureSingleTap,
		kDoubleTap = Core::kGestureDoubleTap,

		// Pen Gestures
		kPenPrimary = Core::kGesturePenPrimary,

        // States (optional)
        kBegin      = Core::kGestureBegin,
        kChanged    = Core::kGestureChanged,
        kEnd        = Core::kGestureEnd,
		kFailed		= Core::kGestureFailed,
		kPossible	= Core::kGesturePossible, ///< e.g. for kDoubleTap: sent after first tap

		// Constraints (optional)
		kHorizontal = Core::kGestureHorizontal,
		kVertical	= Core::kGestureVertical,
		kExclusiveTouch = Core::kGestureExclusiveTouch,
		kSuppressContextMenu = Core::kGestureSuppressContextMenu,
        
		kConstraintsMask = Core::kGestureConstraintsMask,
        kStatesMask = Core::kGestureStatesMask,
		kTypeMask = Core::kGestureTypeMask
	};

	enum Priorities { kPriorityLow, kPriorityNormal, kPriorityHigh, kPriorityHighest, kPriorityUltimate };

	Point where;
	PointF whereF;
    float amountX;
    float amountY;
	KeyState keys;

	GestureEvent (int eventType = kBegin, 
				  const Point& where = Point (),
				  float amount = 1.,
				  const KeyState& keys = KeyState ())
	: GUIEvent (kGestureEvent, eventType),
	  where (where),
	  whereF (pointIntToF (where)),
	  amountX (amount),
      amountY (amount),
	  keys (keys)
	{}
    
	GestureEvent (int eventType, 
				  const PointF& whereF,
				  float amount = 1.,
				  const KeyState& keys = KeyState ())
	: GUIEvent (kGestureEvent, eventType),
	  where (pointFToInt (whereF)),
	  whereF (whereF),
	  amountX (amount),
      amountY (amount),
	  keys (keys)
	{}

    int getType () const;
    int getState () const;
	
    bool isVertical () const;
	bool isHorizontal () const;

	void setPosition (PointRef p);
	void setPosition (PointFRef p);
};

//************************************************************************************************
// TouchEvent
/** Touch Event.
\ingroup gui_event  */
//************************************************************************************************

/** ID of one touch . */
using Core::TouchID;

struct TouchEvent: PointerEvent
{
	enum EventType
	{
		kBegin  = Core::kTouchBegin,
		kMove   = Core::kTouchMove,
		kEnd    = Core::kTouchEnd,

		kEnter  = Core::kTouchEnter,
		kHover  = Core::kTouchHover,
		kLeave  = Core::kTouchLeave,

		kCancel = Core::kTouchCancel
	};

	ITouchCollection& touches;
	TouchID touchID;			///< (optional) id of the touch that has begun or changed
	
	enum { kNoTouchId = 0 };

	TouchEvent (ITouchCollection& touches,
				int eventType = kBegin,
				const KeyState& keys = KeyState (),
				InputDevice inputDevice = kPointerInput)
	: PointerEvent (kTouchEvent, eventType, 0, keys, inputDevice),
	  touches (touches),
	  touchID (kNoTouchId)
	{}

	bool isHoverEvent () const;
};

//************************************************************************************************
// FocusEvent
/** Focus Event. 
\ingroup gui_event  */
//************************************************************************************************

struct FocusEvent: GUIEvent
{
	enum EventType
	{
		kSetFocus,
		kKillFocus
	};

	tbool directed;	///< caused by a directed user action (mouse click in view, keypress), opposed to window (de)activation

	FocusEvent (int eventType = kSetFocus, tbool directed = true)
	: GUIEvent (kFocusEvent, eventType),
	  directed (directed)
	{}
};

//************************************************************************************************
// DragEvent
/** Drag Event. 
\ingroup gui_event  */
//************************************************************************************************

struct DragEvent: GUIEvent
{
	enum EventType
	{
		kDragEnter,
		kDragOver,
		kDragLeave,
		kDrop
	};

	Point where;
	KeyState keys;
	IDragSession& session;

	DragEvent  (IDragSession& session,
				int type = kDragEnter,
				const Point& where = Point (),
				const KeyState& keys = KeyState ())
	: GUIEvent (kDragEvent, type),
	  where (where),
	  keys (keys),
	  session (session)
	{}
};

//************************************************************************************************
// ContextMenuEvent
/** Context Menu Event. 
\ingroup gui_event  */
//************************************************************************************************

struct ContextMenuEvent: GUIEvent
{
	IContextMenu& contextMenu;
	Point where;
	tbool wasKeyPressed;

	ContextMenuEvent (IContextMenu& contextMenu,
					  const Point& where = Point (), 
					  tbool wasKeyPressed = false)
	: GUIEvent (kContextMenuEvent, 0),
	  contextMenu (contextMenu),
	  where (where),
	  wasKeyPressed (wasKeyPressed)
	{}

	void setPosition (PointRef position) const; ///< in view coords
};

//************************************************************************************************
// TooltipEvent
/** Tooltip Event. 
\ingroup gui_event  */
//************************************************************************************************

struct TooltipEvent: GUIEvent
{
	enum EventType
	{
		kShow,
		kHide,
		kMove
	};

	ITooltipPopup& tooltip;
	Point where;

	TooltipEvent (ITooltipPopup& tooltip,
				  int type = kShow,
				  const Point& where = Point ())
	: GUIEvent (kTooltipEvent, type),
	  tooltip (tooltip),
	  where (where)
	 {}
};

//************************************************************************************************
// DisplayChangeEvent
/** Screen resolution change Event. 
 \ingroup gui_event  */
//************************************************************************************************

struct DisplayChangedEvent: GUIEvent
{
	enum EventType
	{
		kResolutionChanged
	};
	
	float contentScaleFactor;
	
	DisplayChangedEvent (float scale, int type = kResolutionChanged)
	: GUIEvent (kDisplayChangedEvent, type),
	  contentScaleFactor (scale)
	{}
};

//************************************************************************************************
// ColorSchemeEvent
/** Color Scheme Event. 
 \ingroup gui_event  */
//************************************************************************************************

struct ColorSchemeEvent: GUIEvent
{
	enum EventType
	{
		kChanged
	};
	
	IColorScheme& scheme;
	
	ColorSchemeEvent (IColorScheme& scheme)
	: GUIEvent (kColorSchemeEvent, kChanged),
	  scheme (scheme)
	{}
};

//////////////////////////////////////////////////////////////////////////////////////////////////
// GUIEvent inline
//////////////////////////////////////////////////////////////////////////////////////////////////

template<class Event>
inline const Event* GUIEvent::as () const
{ return this->eventClass == GUIEvent::getEventClass<Event> () ? static_cast<const Event*> (this) : nullptr; }

template<>
inline const PointerEvent* GUIEvent::as<PointerEvent> () const
{
	// PointerEvent has no EventClass flag, can be MouseEvent or TouchEvent
	const PointerEvent* pointerEvent = as<MouseEvent> ();
	if(!pointerEvent)
		pointerEvent = as<TouchEvent> ();
	return pointerEvent;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// KeyState inline
//////////////////////////////////////////////////////////////////////////////////////////////////

inline bool KeyState::isSet (int key) const	
{ return (keys & key) != 0; }

inline int KeyState::getModifiers () const
{ return keys & kModifierMask; }

inline KeyState::operator const unsigned int () const 
{ return keys; }

//////////////////////////////////////////////////////////////////////////////////////////////////
// KeyEvent inline
//////////////////////////////////////////////////////////////////////////////////////////////////

inline bool KeyEvent::isValid () const
{ return isVKeyValid () || isCharValid (); }

inline bool KeyEvent::isVKeyValid () const
{ return vKey != VKey::kUnknown; }

inline bool KeyEvent::isCharValid () const	
{ return character != 0; }

inline bool KeyEvent::isComposedCharValid () const
{ return composedCharacter != 0; }

inline bool KeyEvent::isRepeat () const	
{ return (state & KeyState::kRepeat) != 0; }

inline bool KeyEvent::operator == (const KeyEvent& k) const
{ return vKey == k.vKey && state == k.state && character == k.character; }

inline bool KeyEvent::operator != (const KeyEvent& k) const
{ return !(*this == k); }

//////////////////////////////////////////////////////////////////////////////////////////////////
// ContextMenuEvent
//////////////////////////////////////////////////////////////////////////////////////////////////

inline void ContextMenuEvent::setPosition (PointRef position) const
{ const_cast<ContextMenuEvent*> (this)->where = position; }

//////////////////////////////////////////////////////////////////////////////////////////////////
// MouseWheelEvent
//////////////////////////////////////////////////////////////////////////////////////////////////

inline bool MouseWheelEvent::isContinuous () const
{ return (wheelFlags & kContinuous); }

inline bool MouseWheelEvent::isRollOutPhase () const
{ return (wheelFlags & kRollOutPhase); }

inline bool MouseWheelEvent::isAxisToggled () const
{ return (wheelFlags & kAxisToggled); }

inline bool MouseWheelEvent::isAxisInverted () const
{ return (wheelFlags & kAxisInverted); }

inline bool MouseWheelEvent::isVertical () const
{ return (eventType == kWheelUp) || (eventType == kWheelDown); }

inline bool MouseWheelEvent::isHorizontal () const	
{ return (eventType == kWheelLeft) || (eventType == kWheelRight); }

inline int MouseWheelEvent::getOriginalDirection () const
{ 
	int afterToggle = isAxisToggled () ? (eventType + 2) % 4 : eventType;
	return isAxisInverted () ? afterToggle ^ 1 : afterToggle;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// TouchEvent
//////////////////////////////////////////////////////////////////////////////////////////////////

inline bool TouchEvent::isHoverEvent () const { return eventType >= TouchEvent::kEnter && eventType <= TouchEvent::kLeave; }

//////////////////////////////////////////////////////////////////////////////////////////////////
// GestureEvent
//////////////////////////////////////////////////////////////////////////////////////////////////

inline int GestureEvent::getType () const { return eventType & kTypeMask; }
inline int GestureEvent::getState () const { return eventType & kStatesMask; }
inline bool GestureEvent::isVertical () const { return ccl_abs (amountY) > ccl_abs(amountX); }
inline bool GestureEvent::isHorizontal () const { return ccl_abs (amountX) > ccl_abs(amountY); }
inline void GestureEvent::setPosition (PointRef p) { where = p; whereF = pointIntToF (p); }
inline void GestureEvent::setPosition (PointFRef p) { where = pointFToInt (p); whereF = p; }

//////////////////////////////////////////////////////////////////////////////////////////////////
// Map EventClass constants to corresponding types
//////////////////////////////////////////////////////////////////////////////////////////////////

template<> inline GUIEvent::EventClass GUIEvent::getEventClass<SystemEvent> ()			{ return kSystemEvent; }
template<> inline GUIEvent::EventClass GUIEvent::getEventClass<KeyEvent> ()				{ return kKeyEvent; }
template<> inline GUIEvent::EventClass GUIEvent::getEventClass<MouseEvent> ()			{ return kMouseEvent; }
template<> inline GUIEvent::EventClass GUIEvent::getEventClass<MouseWheelEvent> ()		{ return kMouseWheelEvent; }
template<> inline GUIEvent::EventClass GUIEvent::getEventClass<GestureEvent> ()			{ return kGestureEvent; }
template<> inline GUIEvent::EventClass GUIEvent::getEventClass<TouchEvent> ()			{ return kTouchEvent; }
template<> inline GUIEvent::EventClass GUIEvent::getEventClass<FocusEvent> ()			{ return kFocusEvent; }
template<> inline GUIEvent::EventClass GUIEvent::getEventClass<DragEvent> ()			{ return kDragEvent; }
template<> inline GUIEvent::EventClass GUIEvent::getEventClass<ContextMenuEvent> ()		{ return kContextMenuEvent; }
template<> inline GUIEvent::EventClass GUIEvent::getEventClass<TooltipEvent> ()			{ return kTooltipEvent; }
template<> inline GUIEvent::EventClass GUIEvent::getEventClass<DisplayChangedEvent> ()	{ return kDisplayChangedEvent; }
template<> inline GUIEvent::EventClass GUIEvent::getEventClass<ColorSchemeEvent> ()		{ return kColorSchemeEvent; }

} // namespace CCL

#endif // _ccl_guievent_h
