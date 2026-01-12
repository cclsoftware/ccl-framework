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
// Filename    : ccl/gui/touch/gesturemanager.cpp
// Description : Gesture Manager
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "ccl/gui/touch/gesturemanager.h"
#include "ccl/gui/touch/touchinput.h"

#include "ccl/gui/gui.h"
#include "ccl/gui/windows/window.h"

#include "ccl/public/systemservices.h"

#if !CCL_STATIC_LINKAGE // already in corelib when linked statically
#include "core/gui/coregesturerecognition.impl.h"
#endif

using namespace CCL;
using namespace Core;

//************************************************************************************************
// CustomGestureManager
//************************************************************************************************

CustomGestureManager::CustomGestureManager (Window& window)
: window (window)
{
	gestureRecognition.setGestureSink (this);
	gestureRecognition.setLongPressDelay (TouchInputState::getLongPressDelay ());
	startTimer ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CustomGestureManager::~CustomGestureManager ()
{
	stopTimer ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CustomGestureManager::onTouchBegan (const TouchInfo& touchInfo)
{
	gestureRecognition.onTouchBegan (touchInfo);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CustomGestureManager::onTouchChanged (const TouchInfo& touchInfo)
{
	gestureRecognition.onTouchChanged (touchInfo);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CustomGestureManager::onTouchEnded (const TouchInfo& touchInfo)
{
	gestureRecognition.onTouchEnded (touchInfo);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CustomGestureManager::isRecognizing (const GestureInfo* gesture) const
{
	return gestureRecognition.isRecognizing (gesture);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CustomGestureManager::startRecognizing (GestureInfo* gesture)
{
	gestureRecognition.startRecognizing (gesture);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CustomGestureManager::stopRecognizing (GestureInfo* gesture)
{
	gestureRecognition.stopRecognizing (gesture);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CustomGestureManager::onIdleTimer ()
{
	gestureRecognition.processIdle (System::GetSystemTicks ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CustomGestureManager::onGesture (GestureInfo* gesture, int state, const GestureEventArgs& args)
{
	GestureEvent event (gesture->getType () | (state & GestureEvent::kStatesMask), args.where);
	event.amountX = args.amountX;
	event.amountY = args.amountY;
	GUI.getKeyState (event.keys);
	window.getTouchInputState ().onGesture (event, *static_cast<Gesture*> (gesture));
}
