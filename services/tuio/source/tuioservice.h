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
// Filename    : tuioservice.h
// Description : TUIO Service
//
//************************************************************************************************

#ifndef _tuioservice_h
#define _tuioservice_h

#include "TuioListener.h"

#include "ccl/public/plugins/serviceplugin.h"
#include "ccl/public/gui/framework/imultitouch.h"
#include "ccl/public/gui/framework/idleclient.h"
#include "ccl/public/gui/framework/iwindow.h"
#include "ccl/public/gui/graphics/rect.h"
#include "ccl/public/collections/hashmap.h"
#include "ccl/public/collections/vector.h"
#include "ccl/public/collections/linkedlist.h"
#include "ccl/public/system/threadsync.h"

#include "ccl/base/storage/configuration.h"

namespace TUIO {

class TuioClient;

} // namespace TUIO

namespace CCL {

class TouchCollection;
class TUIOTouchInfo;

//************************************************************************************************
// TUIOService
//************************************************************************************************

class TUIOService: 	public ServicePlugin,
					public IdleClient, 
					public IWindowEventHandler,
					public TUIO::TuioListener
{
public:
	TUIOService ();
	~TUIOService ();

	static void applyConfiguration ();

	// ServicePlugin
	tresult CCL_API initialize (IUnknown* context = nullptr) override;
	tresult CCL_API terminate () override;

	// IWindowEventHandler
	tbool CCL_API onWindowEvent (WindowEvent& windowEvent) override;
			
	// TuioListener
	void addTuioObject (TUIO::TuioObject* tobj) override;
	void updateTuioObject (TUIO::TuioObject* tobj) override;
	void removeTuioObject (TUIO::TuioObject* tobj) override;
	
	void addTuioCursor (TUIO::TuioCursor* tcur) override;
	void updateTuioCursor (TUIO::TuioCursor* tcur) override;
	void removeTuioCursor (TUIO::TuioCursor* tcur) override;

	void addTuioBlob (TUIO::TuioBlob* tblb) override;
	void updateTuioBlob (TUIO::TuioBlob* tblb) override;
	void removeTuioBlob (TUIO::TuioBlob* tblb) override;

	void refresh (TUIO::TuioTime frameTime) override;

	CLASS_INTERFACE2 (ITimerTask, IWindowEventHandler,ServicePlugin)
	
protected:
	static const CCL::Configuration::BoolValue clientEnabled;
	static const CCL::Configuration::IntValue clientPort;
	static const CCL::Configuration::IntValue monitorNumber;	
	static const CCL::Configuration::IntValue monitorCount;
	static TUIOService* theInstance;
	static int hashIDKey (const TouchID& key, int size);
	static const int kMaxMonitorCount = 16;
	
	int currentPort;
	int currentMonitor;
	int currentMonitorCount;
	TUIO::TuioClient* tuioClient;
	AutoPtr<ITouchInputManager> touchInputManager;
	Rect monitorSize[kMaxMonitorCount];
	PointerHashMap<TouchCollection*> touchCollectionForWindow;
	HashMap<TouchID, IWindow*> windowForTouch;
	LinkedList<TUIOTouchInfo*> touchQueue;
	CCL::Threading::SpinLock queueLock;
	
	void processCursor (TUIO::TuioCursor& tcur);
	IWindow* findWindow (const Point& cclPoint) const;
	void flushQueue ();
	
	void setClientEnabled (bool state);
	void setTUIOPort (int port);
	void setMonitorConfiguration (int count, int monitor);
	void setGeometry ();
	void onWindowRemoved (IWindow* window);
	void simulateMouse (const TUIOTouchInfo& touchInfo);
		
	// IdleClient
	void onIdleTimer () override;
};

} // namespace CCL

#endif // _tuioservice_h
