//************************************************************************************************
//
// This file is part of Crystal Class Library (R)
// Copyright (c) 2026 CCL Software Licensing GmbH.
// All Rights Reserved.
//
// Licensed for use under either:
//  1. a Commercial License provided by CCL Software Licensing GmbH, or
//  2. GNU Affero General Public License v3.0 (AGPLv3).
// 
// You must choose and comply with one of the above licensing options.
// For more information, please visit ccl.dev.
//
// Filename    : ccl/platform/wasm/gui/gui.wasm.cpp
// Description : Platform-specific GUI implementation
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "ccl/platform/wasm/gui/keyevent.wasm.h"
#include "ccl/platform/wasm/gui/exceptionhandler.wasm.h"

#include "ccl/gui/gui.h"
#include "ccl/gui/graphics/imaging/bitmap.h"
#include "ccl/gui/windows/desktop.h"
#include "ccl/gui/windows/windowmanager.h"
#include "ccl/gui/system/mousecursor.h"
#include "ccl/gui/system/systemtimer.h"

#include "ccl/base/storage/settings.h"

#include "ccl/public/gui/iapplication.h"

#include <emscripten/html5.h>

namespace CCL {

//************************************************************************************************
// WasmUserInterface
//************************************************************************************************

class WasmUserInterface: public UserInterface
{
public:
	WasmUserInterface ();

	// UserInterface
	bool startupPlatform (ModuleRef module) override;
	void quitPlatform () override;
	tresult CCL_API queryInterface (UIDRef iid, void** ptr) override;
	int CCL_API runEventLoop () override;
	ITimer* CCL_API createTimer (unsigned int period) const override;
	void CCL_API setMousePosition (const Point& pos) override;
	void CCL_API getKeyState (KeyState& keys) const override;
	tresult CCL_API detectKeyPressed (VirtualKey vKey, uchar character) const override;
	void resetCursor () override;

protected:
	FixedSizeVector<bool, VKey::kNumVirtualKeys> pressedKeys;
	KeyState modifierKeys;
	AutoPtr<MouseCursor> defaultCursor;

	static bool onMouseEvent (int eventType, const EmscriptenMouseEvent* mouseEvent, void* data);
	static bool onKeyEvent (int eventType, const EmscriptenKeyboardEvent* keyEvent, void* data);
	static bool onVisibilityChange (int eventType, const EmscriptenVisibilityChangeEvent* visibilityChangeEvent, void* data);
	static CStringPtr onBeforeUnload (int eventType, const void* reserved, void* userData);
	static void nextEvent (void* arg);

	void nextEvent ();
};

//************************************************************************************************
// WasmTimer
//************************************************************************************************

class WasmTimer: public SystemTimer
{
public:
	WasmTimer (unsigned int period);
	~WasmTimer ();
};

//////////////////////////////////////////////////////////////////////////////////////////////////

WasmUserInterface wasmGUI;
UserInterface& GUI = wasmGUI;

} // namespace CCL

using namespace CCL;
using namespace Wasm;

//************************************************************************************************
// WasmUserInterface
//************************************************************************************************

WasmUserInterface::WasmUserInterface ()
{
	pressedKeys.setCount (pressedKeys.getCapacity ());
	pressedKeys.zeroFill ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool WasmUserInterface::startupPlatform (ModuleRef module)
{
	#if EXCEPTION_HANDLER_ENABLED
	WasmExceptionHandler::instance ().install ();
	#endif

	emscripten_set_mousedown_callback (EMSCRIPTEN_EVENT_TARGET_WINDOW, this, false, onMouseEvent);
	emscripten_set_mouseup_callback (EMSCRIPTEN_EVENT_TARGET_WINDOW, this, false, onMouseEvent);
	emscripten_set_mousemove_callback (EMSCRIPTEN_EVENT_TARGET_WINDOW, this, false, onMouseEvent);
	emscripten_set_keydown_callback (EMSCRIPTEN_EVENT_TARGET_WINDOW, this, false, onKeyEvent);
	emscripten_set_keyup_callback (EMSCRIPTEN_EVENT_TARGET_WINDOW, this, false, onKeyEvent);

	emscripten_set_visibilitychange_callback (this, false, onVisibilityChange);
	emscripten_set_beforeunload_callback (this, onBeforeUnload);

	Bitmap::setResolutionNamingMode (Bitmap::kMultiResolution);

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WasmUserInterface::quitPlatform ()
{
	emscripten_html5_remove_event_listener (EMSCRIPTEN_EVENT_TARGET_WINDOW, this, EMSCRIPTEN_EVENT_MOUSEDOWN, (void*)onMouseEvent);
	emscripten_html5_remove_event_listener (EMSCRIPTEN_EVENT_TARGET_WINDOW, this, EMSCRIPTEN_EVENT_MOUSEUP, (void*)onMouseEvent);
	emscripten_html5_remove_event_listener (EMSCRIPTEN_EVENT_TARGET_WINDOW, this, EMSCRIPTEN_EVENT_MOUSEMOVE, (void*)onMouseEvent);
	emscripten_html5_remove_event_listener (EMSCRIPTEN_EVENT_TARGET_WINDOW, this, EMSCRIPTEN_EVENT_KEYDOWN, (void*)onKeyEvent);
	emscripten_html5_remove_event_listener (EMSCRIPTEN_EVENT_TARGET_WINDOW, this, EMSCRIPTEN_EVENT_KEYUP, (void*)onKeyEvent);

	emscripten_set_visibilitychange_callback (nullptr, false, nullptr);
	emscripten_set_beforeunload_callback (nullptr, nullptr);

	#if EXCEPTION_HANDLER_ENABLED
	WasmExceptionHandler::instance ().uninstall ();
	WasmExceptionHandler::cleanupInstance ();
	#endif
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API WasmUserInterface::queryInterface (UIDRef iid, void** ptr)
{
	#if EXCEPTION_HANDLER_ENABLED
	if(iid == ccl_iid<IDiagnosticDataProvider> ())
		return WasmExceptionHandler::instance ().queryInterface (iid, ptr);
	#endif
	return UserInterface::queryInterface (iid, ptr);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API WasmUserInterface::runEventLoop ()
{
	if(!finishStartup ())
		return kExitError;

	if(appProvider)
		if(!appProvider->onInit ())
			return exitCode;

	#if EXCEPTION_HANDLER_ENABLED
	WasmExceptionHandler::instance ().updateDumpFileName ();
	#endif

	Window* applicationWindow = unknown_cast<Window> (WindowManager::instance ().createApplicationWindow (false));
	if(applicationWindow)
	{
		StyleFlags style = applicationWindow->getStyle ();
		style.setCustomStyle (Styles::kWindowAppearanceTitleBar, false);
		style.setCustomStyle (Styles::kWindowBehaviorSizable, false);
		applicationWindow->setStyle (style);
		applicationWindow->show ();
		applicationWindow->maximize (true);
		onAppStateChanged (IApplication::kUIInitialized);
	}

	if(quitDone) // quit has been requested during startup
	{
		// see also: UserInterface::quit()
		Desktop.closeAll ();
		quitPlatform ();
	}

	emscripten_set_main_loop_arg (nextEvent, this, -1, true);

	return kExitSuccess;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WasmUserInterface::nextEvent (void* arg)
{
	WasmUserInterface* This = static_cast<WasmUserInterface*> (arg);
	This->nextEvent ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WasmUserInterface::nextEvent ()
{
	CCL_PRINTF ("%s\n", __FUNCTION__)

	if(!isTimerBlocked ())
		SystemTimer::serviceTimers ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ITimer* CCL_API WasmUserInterface::createTimer (unsigned int period) const
{
	return NEW WasmTimer (period);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API WasmUserInterface::getKeyState (KeyState& keys) const
{
	keys = modifierKeys;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API WasmUserInterface::detectKeyPressed (VirtualKey vKey, uchar character) const
{
	if(vKey != VKey::kUnknown)
	{
		return pressedKeys[vKey] ? kResultTrue : kResultFalse;
	}
	return kResultNotImplemented;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API WasmUserInterface::setMousePosition (const Point& pos)
{
	lastMousePos = pos;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool WasmUserInterface::onMouseEvent (int eventType, const EmscriptenMouseEvent* mouseEvent, void* data)
{
	WasmUserInterface* This = static_cast<WasmUserInterface*> (data);
	Point p (mouseEvent->clientX, mouseEvent->clientY);
	This->lastMousePos = p;
	This->modifierKeys = 0;
	VKey::fromSystemModifiers (This->modifierKeys, mouseEvent->ctrlKey, mouseEvent->shiftKey, mouseEvent->altKey, false, mouseEvent->buttons);
	return false; // don't swallow
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool WasmUserInterface::onKeyEvent (int eventType, const EmscriptenKeyboardEvent* keyEvent, void* data)
{
	WasmUserInterface* This = static_cast<WasmUserInterface*> (data);
	This->modifierKeys = This->modifierKeys & KeyState::kMouseMask;
	VKey::fromSystemModifiers (This->modifierKeys, keyEvent->ctrlKey, keyEvent->shiftKey, keyEvent->altKey, false);

	VirtualKey vKey = VKey::fromSystemKey (keyEvent->code);
	if(vKey != VKey::kUnknown)
	{
		if(eventType == EMSCRIPTEN_EVENT_KEYDOWN)
			This->pressedKeys[vKey] = true;
		else	
			This->pressedKeys[vKey] = false;
	}

	return false; // don't swallow
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool WasmUserInterface::onVisibilityChange (int eventType, const EmscriptenVisibilityChangeEvent* visibilityChangeEvent, void* data)
{
	WasmUserInterface* This = static_cast<WasmUserInterface*> (data);

	if(visibilityChangeEvent->hidden)
	{
		// notify application of being deactivated
		This->onAppStateChanged (IApplication::kAppDeactivated);

		// save settings (tab may be closed, reloaded or discarded while hidden)
		Settings::autoSaveAll ();
	}
	else
	{
		// notify application of being activated
		This->onAppStateChanged (IApplication::kAppActivated);
	}

	return false; // don't swallow
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CStringPtr WasmUserInterface::onBeforeUnload (int eventType, const void* reserved, void* userData)
{
	// notify application of real termination
	GUI.onAppStateChanged (IApplication::kAppTerminates);

	// safety net in case visibilitychange did not fire before unload
	Settings::autoSaveAll ();

	return nullptr; // don't prompt the user to confirm leaving
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WasmUserInterface::resetCursor ()
{
	if(!defaultCursor.isValid ())
		defaultCursor = MouseCursor::createCursor (ThemeElements::kArrowCursor);
	safe_release (cursor);
	if(defaultCursor)
		defaultCursor->makeCurrent ();
}

//************************************************************************************************
// WasmTimer
//************************************************************************************************

WasmTimer::WasmTimer (unsigned int period)
: SystemTimer (period)
{
	systemTimer = this;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

WasmTimer::~WasmTimer ()
{}
