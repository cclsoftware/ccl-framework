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
// Filename    : ccl/platform/win/gui/touchhelper.h
// Description : Windows Touch API Helpers
//
//************************************************************************************************

#ifndef _ccl_win32_touchhelper_h
#define _ccl_win32_touchhelper_h

#include "ccl/gui/system/systemevent.h"
#include "ccl/gui/touch/gesturemanager.h"

#include "ccl/base/collections/objectlist.h"

#include "ccl/platform/win/cclwindows.h"

#include "ccl/public/gui/framework/iwindow.h"
#include "ccl/public/collections/vector.h"

#include <InteractionContext.h>

namespace CCL {
class Window;
class Gesture;
class DragSession;

namespace Win32 {

//************************************************************************************************
// TouchHelper
//************************************************************************************************

class TouchHelper
{
public:
	static void initialize ();
	static void onPlatformStarted (bool ownProcess);
	static void prepareWindow (Window& window);

	static bool processGestureEvent (Window& window, SystemEvent& e);
	static bool processPointerEvent (Window& window, SystemEvent& e);

	static bool isButtonMessageFromTouch ();	///< checks if current WM_BUTTON message originated from a touch
	static inline bool isButtonMessageFromTouch (LPARAM extraInfo);

	static bool didHandleButtonMessage (Window& window, PointRef where);	///< checks if current WM_BUTTON message was handled as a touch message
	static bool didHandleCurrentMessage ();

	static bool isTouchDragging ();
	static void setTouchDragging (bool state);
	static bool runDragLoop (DragSession& session);

protected:
	static int lastTouchMessageTime;
	static TouchID lastTouchID;
	static Point lastTouchPosition; // in screen coords
	static KeyState lastKeys;
	static bool touchDragging;
	static Configuration::BoolValue usePenAsMouse;

	static int64 getTouchTime (uint64 time); ///< get touch event time from time reported in touch information structures

	class GestureRecognizer;
	class PointerGestureRecognizer;
	class PositionChangeTracker;

	class RecognizerManager: public GestureManagerBase
	{
	public:
		DECLARE_CLASS_ABSTRACT (RecognizerManager, Object)

		RecognizerManager (Window* window);

		PROPERTY_POINTER (Window, window, Window)

		void processPointerFrames (const POINTER_INFO& pointerInfo);

		GestureRecognizer* findRecognizer (const GestureInfo* gesture) const;
		GestureRecognizer* findRecognizerForTouches (const GestureInfo* gesture) const;

		// IGestureManager
		tbool isRecognizing (const GestureInfo* gesture) const override;
		void startRecognizing (GestureInfo* gesture) override;
		void stopRecognizing (GestureInfo* gesture) override;
		void updateTouchesForGesture (GestureInfo* gesture) override;
		void onTouchEnded (const TouchInfo& touchInfo) override;

		CLASS_INTERFACE (IGestureManager, Object)

	private:
		ObjectList recognizers;
	};

	static RecognizerManager* getRecognizerManager (Window& window);
};

//////////////////////////////////////////////////////////////////////////////////////////////////
// TouchHelper inline
//////////////////////////////////////////////////////////////////////////////////////////////////

inline bool TouchHelper::isButtonMessageFromTouch (LPARAM extraInfo)
{
	#define SIGNATURE_MASK  0xFFFFFF00
	#define MI_WP_SIGNATURE 0xFF515700

	return ((extraInfo & SIGNATURE_MASK) == MI_WP_SIGNATURE) // (pen or touch)
					&& ((extraInfo & 0x80) != 0); // (0 = pen, 1 = touch)
}

inline bool TouchHelper::isButtonMessageFromTouch ()
{
	return isButtonMessageFromTouch (::GetMessageExtraInfo ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline bool TouchHelper::didHandleCurrentMessage ()
{
	return ::GetMessageTime () - lastTouchMessageTime < 200;
}

} // namespace Win32
} // namespace CCL

#endif // _ccl_win32_touchhelper_h
