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
// Filename    : ccl/platform/cocoa/iosapp/gesturerecognizer.cpp
// Description : Gesture Recognizer Manager
//
//************************************************************************************************

#define DEBUG_LOG 0

#include <UIKit/UIKit.h>

#include "ccl/platform/cocoa/iosapp/gesturerecognizer.h"
#include "ccl/platform/cocoa/iosapp/contentview.h"

using namespace CCL;

//************************************************************************************************
// RecognizerFactory
//************************************************************************************************

class RecognizerFactory: public Object
{
public:
	RecognizerFactory (CCL_ISOLATED (ContentView)* view, int gestureType);
	~RecognizerFactory ();

	PROPERTY_VARIABLE (int, gestureType, GestureType)

	RecognizerItem* startRecognizing (GestureInfo* gesture);
	void stopRecognizing (GestureInfo* gesture);

	RecognizerItem* findRecognizerItem (UIGestureRecognizer* recognizer) const;
	RecognizerItem* findRecognizerItem (const GestureInfo* gesture) const;

	const CCL::ObjectList& getActiveRecognizers () const { return activeRecognizers; }

private:
	CCL::ObjectList spareRecognizers;
	CCL::ObjectList activeRecognizers;
	CCL_ISOLATED (ContentView)* view;

	enum { kNumSpareRecognizers = 2 };

	void addSpareRecognizers ();
	void limitSpareRecognizers (int maxCount);
	void terminateItem (RecognizerItem* item);
	UIGestureRecognizer* createNativeRecognizer ();
};

//************************************************************************************************
// GestureRecognizerManager
//************************************************************************************************

GestureRecognizerManager::GestureRecognizerManager ()
: view (nil)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void GestureRecognizerManager::init (CCL_ISOLATED (ContentView)* view)
{
	this->view = view;

	recognizerFactories.objectCleanup ();
	recognizerFactories.add (NEW RecognizerFactory (view, GestureEvent::kSwipe));
	recognizerFactories.add (NEW RecognizerFactory (view, GestureEvent::kSingleTap));
	recognizerFactories.add (NEW RecognizerFactory (view, GestureEvent::kDoubleTap));
	recognizerFactories.add (NEW RecognizerFactory (view, GestureEvent::kLongPress));
	recognizerFactories.add (NEW RecognizerFactory (view, GestureEvent::kZoom));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

RecognizerFactory* GestureRecognizerManager::getFactory (const GestureInfo* gesture) const
{
	return getFactory (gesture->getType ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

RecognizerFactory* GestureRecognizerManager::getFactory (int gestureType) const
{
	ArrayForEach (recognizerFactories, RecognizerFactory, factory)
		if(factory->getGestureType () == gestureType)
			return factory;
	EndFor
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void GestureRecognizerManager::startRecognizing (GestureInfo* gesture)
{
	if(RecognizerFactory* factory = getFactory (gesture))
	{
		RecognizerItem* newItem = factory->startRecognizing (gesture);

		// a single tab should only trigger if any active double tab recognizer fails
		// establish this relationship between the new recognizer and any existing single/double tab recognizers
		if(gesture->getType () == GestureEvent::kDoubleTap)
		{
			if(RecognizerFactory* singleTapFactory = getFactory (GestureEvent::kSingleTap))
				ForEach (singleTapFactory->getActiveRecognizers (), RecognizerItem, singleTapItem)
					if(singleTapItem->getNativeRecognizer ())
						[singleTapItem->getNativeRecognizer () requireGestureRecognizerToFail:newItem->getNativeRecognizer ()];
				EndFor
		}
		else if(gesture->getType () == GestureEvent::kSingleTap)
		{
			if(RecognizerFactory* doubleTapFactory = getFactory (GestureEvent::kDoubleTap))
				ForEach (doubleTapFactory->getActiveRecognizers (), RecognizerItem, doubleTapItem)
				if(doubleTapItem->getNativeRecognizer ())
					[newItem->getNativeRecognizer () requireGestureRecognizerToFail:doubleTapItem->getNativeRecognizer ()];
				EndFor
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void GestureRecognizerManager::stopRecognizing (GestureInfo* gesture)
{
	if(RecognizerFactory* factory = getFactory (gesture))
		factory->stopRecognizing (gesture);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool GestureRecognizerManager::isRecognizing (const GestureInfo* gesture) const
{
	RecognizerFactory* factory = getFactory (gesture);
	return factory ? factory->findRecognizerItem (gesture) != 0 : false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

GestureInfo* GestureRecognizerManager::getGesture (UIGestureRecognizer* nativeRecognizer)
{
	RecognizerItem* item = findRecognizerItem (nativeRecognizer);
	return item ? item->getGesture () : nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

RecognizerItem* GestureRecognizerManager::findRecognizerItem (UIGestureRecognizer* recognizer) const
{
	for(auto factory : iterate_as<RecognizerFactory> (recognizerFactories))
		if(RecognizerItem* item = factory->findRecognizerItem (recognizer))
			return item;

	return nullptr;
}

//************************************************************************************************
// RecognizerFactory
//************************************************************************************************

RecognizerFactory::RecognizerFactory (CCL_ISOLATED (ContentView)* view, int gestureType)
: view (view), gestureType (gestureType)
{
	spareRecognizers.objectCleanup ();
	activeRecognizers.objectCleanup ();

	addSpareRecognizers ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

RecognizerFactory::~RecognizerFactory ()
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

RecognizerItem* RecognizerFactory::findRecognizerItem (UIGestureRecognizer* recognizer) const
{
	ForEach (activeRecognizers, RecognizerItem, item)
		if(item->getNativeRecognizer () == recognizer)
			return item;
	EndFor
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

RecognizerItem* RecognizerFactory::findRecognizerItem (const GestureInfo* gesture) const
{
	ForEach (activeRecognizers, RecognizerItem, item)
		if(item->getGesture () == gesture)
			return item;
	EndFor
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void RecognizerFactory::terminateItem (RecognizerItem* item)
{
	if(item)
	{
		ASSERT (!item->getGesture ()) // otherwise we are still observing
		if(UIGestureRecognizer* recognizer = item->getNativeRecognizer ())
			[view removeGestureRecognizer: recognizer];

		item->release ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void RecognizerFactory::addSpareRecognizers ()
{
	for(int i = spareRecognizers.count (); i < kNumSpareRecognizers; i++)
	{
		RecognizerItem* item = NEW RecognizerItem;
		spareRecognizers.add (item);

		if(UIGestureRecognizer* recognizer = createNativeRecognizer ())
		{
			recognizer.cancelsTouchesInView = FALSE;
			recognizer.delegate = view;
			[view addGestureRecognizer: recognizer];
			[recognizer release];

			item->setNativeRecognizer (recognizer);
		}
	}
	CCL_PRINTF ("recognizers type %d: %d (%d spare)\n", gestureType, activeRecognizers.count (), spareRecognizers.count ())
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void RecognizerFactory::limitSpareRecognizers (int maxCount)
{
	int toRemove = spareRecognizers.count () - maxCount;
	for(int i = 0; i < toRemove; i++)
	{
		RecognizerItem* item = (RecognizerItem*)spareRecognizers.removeLast ();
		ASSERT (item)
		terminateItem (item);
	}
	CCL_PRINTF ("recognizers type %d: %d (%d spare)\n", gestureType, activeRecognizers.count (), spareRecognizers.count ())
}

//////////////////////////////////////////////////////////////////////////////////////////////////

RecognizerItem* RecognizerFactory::startRecognizing (GestureInfo* gesture)
{
	RecognizerItem* item = (RecognizerItem*)spareRecognizers.removeFirst ();
	ASSERT (item)
	if(item)
	{
		CCL_PRINTF ("%s startRecognizing\n", CCL_DEBUG_ID (item->getNativeRecognizer ()))

		item->setGesture (gesture);

		activeRecognizers.append (item);
	}
	addSpareRecognizers ();
	return item;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void RecognizerFactory::stopRecognizing (GestureInfo* gesture)
{
	RecognizerItem* item = findRecognizerItem (gesture);
	ASSERT (item)
	if(item)
	{
		CCL_PRINTF ("%s stopRecognizing\n", CCL_DEBUG_ID (item->getNativeRecognizer ()))

		ASSERT (activeRecognizers.contains (item))
		if(activeRecognizers.remove (item))
		{
			item->setGesture (0);
		#if 0
			terminateItem (item);
			ASSERT (spareRecognizers.count () == kNumSpareRecognizers)
		#else
			spareRecognizers.prepend (item);
		#endif
		}
	}
	limitSpareRecognizers (kNumSpareRecognizers);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

UIGestureRecognizer* RecognizerFactory::createNativeRecognizer ()
{
	switch(gestureType)
	{
	case GestureEvent::kSwipe:
		return [[UIPanGestureRecognizer alloc] initWithTarget: view action:@selector(onPanGesture:)];

	case GestureEvent::kZoom:
		return [[UIPinchGestureRecognizer alloc] initWithTarget: view action:@selector(onPinchGesture:)];

	case GestureEvent::kRotate:
		// todo
		return nil;

	case GestureEvent::kSingleTap:
		{
			UITapGestureRecognizer* recognizer = [[UITapGestureRecognizer alloc] initWithTarget: view action:@selector(onTapGesture:)];
			recognizer.delaysTouchesEnded = FALSE;
			return recognizer;
		}

	case GestureEvent::kDoubleTap:
		{
			UITapGestureRecognizer* doubleTap = [[UITapGestureRecognizer alloc] initWithTarget: view action:@selector(onDoubleTapGesture:)];
			doubleTap.numberOfTapsRequired = 2;
			doubleTap.delaysTouchesEnded = FALSE;
			return doubleTap;
		}
	case GestureEvent::kLongPress:
		{
			UILongPressGestureRecognizer* longPress = [[UILongPressGestureRecognizer alloc] initWithTarget: view action:@selector(onLongPressGesture:)];
			longPress.minimumPressDuration = 0.001 * TouchInputState::getLongPressDelay ();
			return longPress;
		}
	}
	return nil;
}
