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
// Filename    : ccl/gui/touch/gesturemanager.h
// Description : Gesture Manager
//
//************************************************************************************************

#ifndef _ccl_gesturemanager_h
#define _ccl_gesturemanager_h

#include "ccl/gui/touch/touchinput.h"
#include "ccl/public/gui/framework/idleclient.h"

#include "core/gui/coregesturerecognition.h"

#include "ccl/base/object.h"

namespace CCL {

class Window;

//************************************************************************************************
// GestureManagerBase
/** Base class for Gesture recognition managers.
	\ingroup gui */
//************************************************************************************************

class GestureManagerBase: public Object,
						  public IGestureManager
{
public:
	// IGestureManager
	void onTouchBegan (const TouchInfo& touchInfo) override {};
	void onTouchChanged (const TouchInfo& touchInfo) override {};
	void onTouchEnded (const TouchInfo& touchInfo) override {};
	void updateTouchesForGesture (GestureInfo* gesture) override {};

	CLASS_INTERFACE (IGestureManager, Object)
};

//************************************************************************************************
// CustomGestureManager
/** Gesture manager using Core::GestureRecognition.
	\ingroup gui */
//************************************************************************************************

class CustomGestureManager: public GestureManagerBase,
							public IdleClient,
							public Core::GestureRecognition::GestureSink
{
public:
	CustomGestureManager (Window& window);
	~CustomGestureManager ();

	// IGestureManager
	void onTouchBegan (const TouchInfo& touchInfo) override;
	void onTouchChanged (const TouchInfo& touchInfo) override;
	void onTouchEnded (const TouchInfo& touchInfo) override;
	tbool isRecognizing (const GestureInfo* gesture) const override;
	void startRecognizing (GestureInfo* gesture) override;
	void stopRecognizing (GestureInfo* gesture) override;

	// IdleClient
	void onIdleTimer () override;

	// GestureSink
	void onGesture (GestureInfo* gesture, int state, const Core::GestureEventArgs& args) override;
	
	CLASS_INTERFACE (ITimerTask, Object)

private:
	Core::GestureRecognition gestureRecognition;
	Window& window;
};

} // namespace CCL

#endif // _ccl_gesturemanager_h
