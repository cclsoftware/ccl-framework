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
// Filename    : ccl/platform/cocoa/iosapp/gesturerecognizer.h
// Description : Gesture Recognizer Manager
//
//************************************************************************************************

#include "ccl/gui/touch/gesturemanager.h"
#include "ccl/base/collections/objectarray.h"

namespace CCL {
class Gesture; }

class RecognizerFactory;
class RecognizerItem;
@class CCL_ISOLATED (ContentView);
@class UIGestureRecognizer;

//************************************************************************************************
// GestureRecognizerManager
//************************************************************************************************

class GestureRecognizerManager: public CCL::GestureManagerBase
{
public:
	GestureRecognizerManager ();

	void init (CCL_ISOLATED (ContentView)* view);

	CCL::GestureInfo* getGesture (UIGestureRecognizer* nativeRecognizer);
	RecognizerItem* findRecognizerItem (UIGestureRecognizer* recognizer) const;

	// IGestureManager
	CCL::tbool isRecognizing (const Core::GestureInfo* gesture) const;
	void startRecognizing (Core::GestureInfo* gesture);
	void stopRecognizing (Core::GestureInfo* gesture);

private:
	CCL_ISOLATED (ContentView)* view;
	CCL::ObjectArray recognizerFactories;

	RecognizerFactory* getFactory (const Core::GestureInfo* gesture) const;
	RecognizerFactory* getFactory (int gestureType) const;
};

//************************************************************************************************
// RecognizerItem
//************************************************************************************************

class RecognizerItem: public CCL::Object
{
public:
	PROPERTY_POINTER (CCL::GestureInfo, gesture, Gesture)
	PROPERTY_POINTER (UIGestureRecognizer, nativeRecognizer, NativeRecognizer)
	PROPERTY_VARIABLE (float, startAmountX, StartAmountX)
	PROPERTY_VARIABLE (float, startAmountY, StartAmountY)

	RecognizerItem ()
	: gesture (nullptr),
	  nativeRecognizer (nullptr),
	  startAmountX (1),
	  startAmountY (1)
	{}

	void setStartAmount (CCL::GestureEvent& event)
	{	
		setStartAmountX (event.amountX);
		setStartAmountY (event.amountY);
	}
};
