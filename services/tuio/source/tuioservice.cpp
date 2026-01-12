//************************************************************************************************
//
// TUIO Support
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
// Filename    : tuioservice.cpp
// Description : TUIO Service
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "tuioservice.h"
#include "tuiouseroption.h"
#include "plugversion.h"

#include "ccl/public/plugservices.h"
#include "ccl/public/guiservices.h"
#include "ccl/public/systemservices.h"
#include "ccl/public/plugins/classfactory.h"
#include "ccl/public/gui/framework/iview.h"
#include "ccl/public/gui/framework/iwindow.h"
#include "ccl/public/gui/graphics/point.h"
#include "ccl/public/gui/framework/iwindowmanager.h"
#include "ccl/public/gui/framework/iuserinterface.h"
#include "ccl/public/gui/ipluginview.h"

#include "ccl/base/storage/settings.h"

#include "ccl/main/cclargs.h"

#include "ccl/gui/touch/touchcollection.h"

#include "ccl/public/base/ccldefpush.h"
#include "TuioClient.h"
#include "ccl/public/base/ccldefpop.h"

using namespace CCL;
using namespace TUIO;

//************************************************************************************************
// TUIOTouchInfo
//************************************************************************************************

class CCL::TUIOTouchInfo : public TouchInfo
{
public:
	TUIOTouchInfo (TuioCursor& tcur, Point where)
	: TouchInfo (translateEventType (tcur.getTuioState ()), tcur.getSessionID (), where, System::GetSystemTicks ())
	{
	}
	
protected:
	inline int translateEventType (int tuioState) const
	{
		switch(tuioState)
		{
		case TUIO_ADDED:
			return TouchEvent::EventType::kBegin;
			break;
		case TUIO_ACCELERATING:
		case TUIO_DECELERATING:
		case TUIO_STOPPED:
			return TouchEvent::EventType::kMove;
			break;
		case TUIO_REMOVED:
			return TouchEvent::EventType::kEnd;
			break;
		default:
			ASSERT (0)
			break;
		}
		return 0;
	}
};

//************************************************************************************************
// TUIOService
//************************************************************************************************

const Configuration::BoolValue TUIOService::clientEnabled ("TUIO", "clientEnabled_2", false);
const Configuration::IntValue TUIOService::clientPort ("TUIO", "clientPort", 3333);
const Configuration::IntValue TUIOService::monitorNumber ("TUIO", "monitorNumber", 0);
const Configuration::IntValue TUIOService::monitorCount ("TUIO", "monitorCount", 1);

TUIOService* TUIOService::theInstance = nullptr;

//////////////////////////////////////////////////////////////////////////////////////////////////

int TUIOService::hashIDKey (const TouchID& key, int size)
{
	return key % size;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

TUIOService::TUIOService () 
: touchInputManager (nullptr),
  tuioClient (nullptr),
  windowForTouch (128, hashIDKey),
  currentPort (3333),
  currentMonitor (0),
  currentMonitorCount (0)
{
	ASSERT (theInstance == nullptr)
	theInstance = this;
	for(int i = 0; i < kMaxMonitorCount ; i++)
		monitorSize[i] = Rect ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

TUIOService::~TUIOService ()
{
	ASSERT (theInstance == this)
	theInstance = nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TUIOService::applyConfiguration ()
{
	if(theInstance)
	{
		theInstance->setMonitorConfiguration (monitorCount, monitorNumber);
		theInstance->setTUIOPort (clientPort);
		theInstance->setClientEnabled (clientEnabled);
	}
}
	
//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API TUIOService::initialize (IUnknown* context)
{
	// register classes
	ClassFactory& classFactory = getClassFactory ();
	
	static const ClassDesc optionClass 
	(
		UID (0xB2EB81B3, 0x2A15, 0x3043, 0xAF, 0x3B, 0x50, 0x1F, 0x48, 0xF6, 0x3D, 0xFC),
		PLUG_CATEGORY_USEROPTION,
		PLUG_NAME " User Option"
	);
	
	classFactory.registerClass (optionClass, PluginConstructor<TUIOUserOption, IUserOption>::createInstance);
	
	touchInputManager = ccl_new<ITouchInputManager> (ClassID::TouchInputManager);
	ASSERT (touchInputManager != nullptr)
	if(touchInputManager == nullptr)
		return kResultFailed;

	// restore settings
	Settings& settings = Settings::instance ();
	settings.init (PLUG_ID);
	settings.isBackupEnabled (true);
	settings.restore ();
	settings.addSaver (NEW ConfigurationSaver ("TUIO", "clientEnabled_2"));
	settings.addSaver (NEW ConfigurationSaver ("TUIO", "clientPort"));
	settings.addSaver (NEW ConfigurationSaver ("TUIO", "monitorNumber"));	
	settings.addSaver (NEW ConfigurationSaver ("TUIO", "monitorCount"));
	applyConfiguration ();
	
	return ServicePlugin::initialize (context);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API TUIOService::terminate ()
{
	setClientEnabled (false);
	touchInputManager.release ();
	
	return ServicePlugin::terminate ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API TUIOService::onWindowEvent (WindowEvent& windowEvent)
{
	if(windowEvent.eventType == WindowEvent::kClose || windowEvent.eventType == WindowEvent::kDestroy)
	{
		IWindow& targetWindow = windowEvent.window;
		if(TouchCollection* touchCollection = touchCollectionForWindow.lookup (&targetWindow))
			onWindowRemoved (&targetWindow);
	}
	return  true;
}
	
//////////////////////////////////////////////////////////////////////////////////////////////////

void TUIOService::addTuioObject (TuioObject* tobj)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TUIOService::updateTuioObject (TuioObject* tobj)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TUIOService::removeTuioObject (TuioObject* tobj)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TUIOService::addTuioCursor (TuioCursor* tcur)
{
	if(!tcur)
		return;
		
	processCursor (*tcur);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TUIOService::updateTuioCursor (TuioCursor* tcur)
{
	if(!tcur)
		return;

	processCursor (*tcur);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TUIOService::removeTuioCursor (TuioCursor* tcur)
{
	if(!tcur)
		return;

	processCursor (*tcur);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TUIOService::addTuioBlob (TuioBlob* tblb)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////
///
void TUIOService::updateTuioBlob (TuioBlob* tblb)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////
///
void TUIOService::removeTuioBlob (TuioBlob* tblb)
{}
	
//////////////////////////////////////////////////////////////////////////////////////////////////

void TUIOService::refresh (TuioTime frameTime)
{
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TUIOService::processCursor (TuioCursor& tcur)
{
	if(!tuioClient)
		return;

	Coord x = monitorSize[0].left + ccl_to_int<Coord> (ccl_min (tcur.getX () * monitorCount, 1.0f) * monitorSize[0].getWidth ());
	Coord y = monitorSize[0].top + ccl_to_int<Coord> (tcur.getY () * monitorSize[0].getHeight ());
	
	for(int i = 1; i < monitorCount; i++)
	{
		float monitorX = ccl_min ((tcur.getX () * monitorCount) - i , 1.0f);
		if(monitorX < 0)
			break;
		x += ccl_to_int<Coord> (monitorX * monitorSize[i].getWidth ());
	}

	TUIOTouchInfo* touch = NEW TUIOTouchInfo (tcur, Point (x, y));
	queueLock.lock ();
	touchQueue.append (touch);
	queueLock.unlock ();	
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IWindow* TUIOService::findWindow (const Point& cclPoint) const
{
	return System::GetDesktop ().findWindow (cclPoint, IDesktop::kEnforceOcclusionCheck);	
}


//////////////////////////////////////////////////////////////////////////////////////////////////

void TUIOService::onIdleTimer ()
{
	flushQueue ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TUIOService::flushQueue ()
{
	if(touchQueue.isEmpty ())
		return;
	
	while(true)
	{
		queueLock.lock ();
		TUIOTouchInfo* touchInfo = touchQueue.removeFirst ();
		queueLock.unlock ();
		if(!touchInfo)
			break;

		// coalesce move events from same finger
		if(touchInfo->type == TouchEvent::kMove)
		{
			queueLock.lock ();
			while(TUIOTouchInfo* nextTouchInfo = touchQueue.removeFirst ())
			{
				if(nextTouchInfo->type == TouchEvent::kMove && nextTouchInfo->id == touchInfo->id)
				{
					delete touchInfo;
					touchInfo = nextTouchInfo;
				}
				else
				{
					touchQueue.prepend (nextTouchInfo);
					break;
				}
			}
			queueLock.unlock ();
		}
			
		TouchEvent::EventType eventType = TouchEvent::kMove;
		IWindow* targetWindow = nullptr;

		if(touchInfo->type == TouchEvent::kBegin)
		{
			targetWindow = findWindow (touchInfo->where);
			if(IView* view = UnknownPtr<IView> (targetWindow))
			{
				Rect clientSize = view->getSize ();
				clientSize.moveTo (Point ());
				Point clientPoint (touchInfo->where);
				view->screenToClient (clientPoint);

				// 1) check for title bar etc.
				if(!clientSize.pointInside (clientPoint))
					targetWindow = nullptr;
					
				// 2) check for 3rd party plugin view
				else if(IPlugInViewFrame* plugView = UnknownPtr<IPlugInViewFrame> (view->getChildren ().findChildView (clientPoint, true)))
					targetWindow = nullptr;
			}
			
			windowForTouch.add (touchInfo->id, targetWindow);

			// kBegin if at least one touch was added to the event
			eventType = TouchEvent::kBegin;
		}
		else
			targetWindow = windowForTouch.lookup (touchInfo->id);
		
		if(!targetWindow)
		{
			simulateMouse (*touchInfo);
			if(touchInfo->type == TouchEvent::kEnd)
				windowForTouch.remove (touchInfo->id);
			delete touchInfo;
			continue;
		}

		TouchCollection* touchCollection = touchCollectionForWindow.lookup (targetWindow);
		
		if(touchInfo->type == TouchEvent::kBegin)
		{
			if(!touchCollection)
			{
				touchCollection = NEW TouchCollection ();
				touchCollectionForWindow.add (targetWindow, touchCollection);
				targetWindow->addHandler (this);
			}
			
			if(touchCollection->isEmpty ())
				System::GetGUI ().activateApplication (false, ArgumentList ());
		}
		else
		{
			if(!touchCollection)
			{
				// we got a move or end without a begin: try to repair
				if(touchInfo->type == TouchEvent::kEnd)
					continue;
				
				targetWindow = findWindow (touchInfo->where);
				windowForTouch.add (touchInfo->id, targetWindow);
				touchCollection = NEW TouchCollection ();
				touchCollectionForWindow.add (targetWindow, touchCollection);
				targetWindow->addHandler (this);
			}
			else
			{
				const TouchInfo* oldTouchInfo = touchCollection->getTouchInfoByID (touchInfo->id);
				if(oldTouchInfo)
					touchCollection->remove (*oldTouchInfo);
			}
		}

		if(IView* view = UnknownPtr<IView> (targetWindow))
        {
            Point p (touchInfo->where);
            view->screenToClient (p);
            touchInfo->setPosition (p);
        }

		touchCollection->add (*touchInfo);
		
		// kEnd if every touch was removed
		if(touchInfo->type == TouchEvent::kEnd && touchCollection->count () == 1)
			eventType = TouchEvent::kEnd;
		
		if(targetWindow)
		{
			CCL_PRINTF("touchInputManager->processTouches : %d\n", touchInfo->type)
			TouchCollection touchCollectionCopy;
			touchCollectionCopy.copyFrom (*touchCollection);
			TouchEvent touchEvent (touchCollectionCopy, eventType);
            touchEvent.inputDevice = TouchEvent::kTouchInput;
			touchEvent.eventTime = System::GetProfileTime ();
		 	System::GetGUI ().getKeyState (touchEvent.keys);
			touchInputManager->processTouches (targetWindow, touchEvent);
		}
					
		// processTouches could have lead to setClientEnabled (false) -> dangling touchCollection
		// need to get the touchCollection anew here
		if(tuioClient)
			if(touchCollection = touchCollectionForWindow.lookup (targetWindow))
				if(touchInfo->type == TouchEvent::kEnd)
				{
					touchCollection->remove (*touchInfo);
					windowForTouch.remove (touchInfo->id);
				}
				
		delete touchInfo;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TUIOService::setClientEnabled (bool state)
{
	bool enabled = tuioClient != nullptr;
	if(state != enabled)
	{
		if(state)
		{
			tuioClient = NEW TuioClient (currentPort);
			tuioClient->addTuioListener (this);
			tuioClient->connect ();
			
			startTimer ();
		}
		else
		{
			stopTimer ();
			
			tuioClient->disconnect ();
			tuioClient->removeAllTuioListeners ();
			delete tuioClient;
			tuioClient = nullptr;
			
			PointerHashMap<TouchCollection*>::Iterator iter (touchCollectionForWindow);
			while(!iter.done ())
				if(IWindow* window = (IWindow*)iter.nextAssociation ().key)
					onWindowRemoved (window);

			queueLock.lock ();
			touchQueue.removeAll ();
			queueLock.unlock ();
		}
	}
}


//////////////////////////////////////////////////////////////////////////////////////////////////

void TUIOService::setTUIOPort (int port)
{
	if(port != currentPort)
	{
		currentPort = port;
		if(tuioClient != nullptr)
		{
			setClientEnabled (false);
			setClientEnabled (true);
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TUIOService::setMonitorConfiguration (int count, int monitor)
{
	currentMonitorCount = ccl_bound (count, 1, ccl_min (System::GetDesktop ().countMonitors (), kMaxMonitorCount));
	currentMonitor = ccl_bound (monitor, 0, System::GetDesktop ().countMonitors () - 1);
	setGeometry ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TUIOService::setGeometry ()
{
	// the user selects the leftmost monitor
	System::GetDesktop ().getMonitorSize (monitorSize[0], currentMonitor, false);
	int monitorsFound = 1;
	for(int i = 1; i < currentMonitorCount; i++)
	{
		int nextMonitor = System::GetDesktop ().findMonitor (Point (monitorSize[i-1].right + 1, (monitorSize[i-1].top + monitorSize[i-1].bottom) / 2), false);
		if(nextMonitor != -1)
		{
			System::GetDesktop ().getMonitorSize (monitorSize[i], nextMonitor, false);
			monitorsFound++;
		}
		else
			break;
	}
	currentMonitorCount = monitorsFound;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TUIOService::onWindowRemoved (IWindow* targetWindow)
{
	if(TouchCollection* touchCollection = touchCollectionForWindow.lookup (targetWindow))
	{
		touchInputManager->discardTouches (targetWindow);	
		touchCollection->removeAll ();
		delete touchCollection;
		touchCollectionForWindow.remove (targetWindow);
	}
	
	HashMapIterator<TouchID, IWindow*> iter (windowForTouch);
	while(!iter.done ())
	{
		HashMapIterator<TouchID, IWindow*>::Association current = iter.nextAssociation ();
		IWindow* window = current.value;
		if(window == targetWindow)
			windowForTouch.remove (current.key);
	}	
	
	targetWindow->removeHandler (this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TUIOService::simulateMouse (const TUIOTouchInfo& touchInfo)
{
	MouseEvent mouseEvent (MouseEvent::kMouseDown, touchInfo.where, 0, touchInfo.time / 1000.);
	mouseEvent.keys.keys |= KeyState::kLButton;
	mouseEvent.inputDevice = MouseEvent::kTouchInput;
	mouseEvent.dragged = 0;
	mouseEvent.doubleClicked = 0;			
	switch(touchInfo.type)
	{
		case TouchEvent::kBegin:
			mouseEvent.eventType = MouseEvent::kMouseDown;
			break;
		case TouchEvent::kMove:
			mouseEvent.eventType = MouseEvent::kMouseMove;
			mouseEvent.dragged = 1;
			break;
		case TouchEvent::kEnd:
			mouseEvent.eventType = MouseEvent::kMouseUp;
			windowForTouch.remove (touchInfo.id);
			break;								
	}
	System::GetGUI ().simulateEvent (mouseEvent);
}
