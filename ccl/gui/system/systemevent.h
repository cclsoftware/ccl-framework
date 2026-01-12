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
// Filename    : ccl/gui/system/systemevent.h
// Description : System Event
//
//************************************************************************************************

#ifndef _ccl_systemevent_h
#define _ccl_systemevent_h

#include "ccl/public/gui/framework/guievent.h"

namespace CCL {

/** Event handler return type. */
typedef void* EventResult;

//************************************************************************************************
// SystemEvent
/** System event wrapper */
//************************************************************************************************

struct SystemEvent: GUIEvent
{
	#if CCL_PLATFORM_WINDOWS
	void* hwnd;
	unsigned int msg;
	void* lParam;
	void* wParam;
	tbool notHandled;

	SystemEvent (void* hwnd, unsigned int msg, void* wParam, void* lParam)
	: GUIEvent (kSystemEvent, 0),
	  hwnd (hwnd), 
	  msg (msg),
	  wParam (wParam),
	  lParam (lParam),
	  notHandled (false)
	{}

	bool wasHandled () const { return notHandled == 0; }

	#elif CCL_PLATFORM_MAC || CCL_PLATFORM_IOS
	void* dataRef;
	
	SystemEvent (void* dataRef)
	: GUIEvent (kSystemEvent, 0),
	  dataRef (dataRef)
	{}

	#elif CCL_PLATFORM_LINUX
	enum SystemEventType
	{
		kSeatCapabilitiesChanged,
		kOutputsChanged 
	};

	SystemEvent (SystemEventType type)
	: GUIEvent (kSystemEvent, type)
	{}

	#endif
};

//************************************************************************************************
// SystemEventHandler
/** System event handler */
//************************************************************************************************

class SystemEventHandler
{
public:
	virtual EventResult handleEvent (SystemEvent& e) = 0;
};

} // namespace CCL

#endif // _ccl_systemevent_h
