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
// Filename    : ccl/public/gui/framework/iwindowmanager.h
// Description : Window Manager Interface
//
//************************************************************************************************

#ifndef _ccl_iwindowmanager_h
#define _ccl_iwindowmanager_h

#include "ccl/public/base/iunknown.h"
#include "ccl/public/gui/graphics/rect.h"

namespace CCL {

interface IView;
interface IWindow;
interface IMenuBar;
interface IParameter;
interface IAliasParameter;
interface IAttributeList;
class MutableCString;

//************************************************************************************************
// IWindowClassVerifier
/** Tells if a window class is currently active (can be used).
	\ingroup gui_view */
//************************************************************************************************

interface IWindowClassVerifier: IUnknown
{
	virtual tbool CCL_API isWindowClassActive () = 0;

	DECLARE_IID (IWindowClassVerifier)
};

DEFINE_IID (IWindowClassVerifier, 0x24BB5C11, 0xF4A0, 0x4FA4, 0xA7, 0x13, 0x6A, 0xEB, 0x46, 0x67, 0xC8, 0xAC)

//************************************************************************************************
// IWindowClass
/** Used for identification of registered window classes.
	\ingroup gui_view */
//************************************************************************************************

interface IWindowClass: IUnknown
{
	virtual StringID CCL_API getClassID () const = 0;

	virtual void CCL_API setVerifier (IWindowClassVerifier* verifier) = 0; ///< verifer is not shared!

	virtual void CCL_API setCommand (StringID category, StringID name) = 0;

	virtual void CCL_API getCommand (MutableCString& category, MutableCString& name) const = 0;

	DECLARE_IID (IWindowClass)
};

DEFINE_IID (IWindowClass, 0x42E25FAC, 0xE37F, 0x43E4, 0xAB, 0x68, 0xA4, 0xA6, 0xAD, 0xFF, 0x5D, 0x18)

//************************************************************************************************
// IWindowManager
/**
	\ingroup gui_view */
//************************************************************************************************

interface IWindowManager: IUnknown
{
	//////////////////////////////////////////////////////////////////////////////////////////////
	// Application Window
	//////////////////////////////////////////////////////////////////////////////////////////////

	/** Primary form name for application window in skin. */
	DECLARE_STRINGID_MEMBER (kApplicationFormName)

	/** Create main application view. */
	virtual IView* CCL_API createApplicationView (const Rect& bounds) = 0;

	/** Create application window. */
	virtual IWindow* CCL_API createApplicationWindow (tbool show = true) = 0;

	/** Create main application menu bar.
		This can be a custom menu bar provided by the framework or a platform implementation. */
	virtual IMenuBar* CCL_API createApplicationMenuBar (tbool variant) = 0;

	/** Init application without main window (alternative to createApplicationWindow()). */
	virtual void CCL_API initWindowlessApplication () = 0;

	//////////////////////////////////////////////////////////////////////////////////////////////
	// Window management
	//////////////////////////////////////////////////////////////////////////////////////////////

	/** Check if given window is open. */
	virtual tbool CCL_API isWindowOpen (StringID windowClassId) = 0;
	virtual tbool CCL_API isWindowOpen (IWindowClass* windowClass) = 0;

	/** Open window by identifier. */
	virtual tbool CCL_API openWindow (StringID windowClassId, tbool toggle = false, IAttributeList* arguments = nullptr) = 0;
	virtual tbool CCL_API openWindow (IWindowClass* windowClass, tbool toggle = false, IAttributeList* arguments = nullptr) = 0;

	/** Close window by identifier. */
	virtual tbool CCL_API closeWindow (StringID windowClassId, tbool forceNow = false) = 0;
	virtual tbool CCL_API closeWindow (IWindowClass* windowClass, tbool forceNow = false) = 0;

	/** Replace content of named window with another one. */
	virtual tbool CCL_API replaceWindow (StringID oldClassId, StringID newClassId) = 0;
	virtual tbool CCL_API replaceWindow (IWindowClass* oldClass, IWindowClass* newClass) = 0;

	/** Center window */
	virtual tbool CCL_API centerWindow (StringID windowClassId) = 0;
	virtual tbool CCL_API centerWindow (IWindowClass* windowClass) = 0;

	/** Check if given window can be reused for other content. */
	virtual tbool CCL_API canReuseWindow (IWindowClass* oldClass) = 0;

	/** Check if given window can be opened. */
	virtual tbool CCL_API canOpenWindow (StringID windowClassId) = 0;

	/** Suspend automatic activation of opened windows. Returns the old suspend state. */
	virtual tbool CCL_API suspendActivation (tbool state) = 0;

	struct ActivationSuspender;

	//////////////////////////////////////////////////////////////////////////////////////////////
	// Window classes
	//////////////////////////////////////////////////////////////////////////////////////////////

	/** Register a new window class. */
	virtual IWindowClass* CCL_API registerClass (StringID windowClassId, StringRef formName, StringRef controllerUrl, StringRef groupID, StringID workspaceID, StringID themeID, StringID storageID = nullptr) = 0;

	/** Unregister a window class. */
	virtual void CCL_API unregisterClass (IWindowClass* windowClass) = 0;

	/** Find registered window class. */
	virtual IWindowClass* CCL_API findWindowClass (StringID windowClassId) = 0;

	//////////////////////////////////////////////////////////////////////////////////////////////
	// Window states
	//////////////////////////////////////////////////////////////////////////////////////////////

	/** Store window states. */
	virtual void CCL_API storeWindowStates () = 0;

	/** Restore window states. */
	virtual void CCL_API restoreWindowStates () = 0;

	//////////////////////////////////////////////////////////////////////////////////////////////////

	/** Get the parameter representing the state of the given window class. */
	virtual IParameter* CCL_API getOpenParameter (IWindowClass* windowClass) = 0;

	/** Get (or create) an alias parameter for some externally managed visibility state (similar to window class parameters). */
	virtual IAliasParameter* CCL_API getVisibilityAliasParameter (StringID externalClassId) = 0;

	//////////////////////////////////////////////////////////////////////////////////////////////
	// Signals
	//////////////////////////////////////////////////////////////////////////////////////////////

	/** Sent before a window is explicitely opened by the user (e.g. via command or parameter); arg[0]: windows class id (String) */
	DECLARE_STRINGID_MEMBER (kBeforeOpenWindow)

	/** arg[0]: windows class id (String) */
	DECLARE_STRINGID_MEMBER (kWindowOpened)

	/** arg[0]: windows class id (String) */
	DECLARE_STRINGID_MEMBER (kWindowClosed)
	

	DECLARE_IID (IWindowManager)
};

DEFINE_IID (IWindowManager, 0x8b6703a5, 0xad1b, 0x446e, 0x97, 0x1c, 0x78, 0x7e, 0x22, 0x6a, 0x6b, 0x66)
DEFINE_STRINGID_MEMBER (IWindowManager, kApplicationFormName, "ApplicationWindow")
DEFINE_STRINGID_MEMBER (IWindowManager, kBeforeOpenWindow, "BeforeOpenWindow")
DEFINE_STRINGID_MEMBER (IWindowManager, kWindowOpened, "WindowOpened")
DEFINE_STRINGID_MEMBER (IWindowManager, kWindowClosed, "WindowClosed")

//////////////////////////////////////////////////////////////////////////////////////////////
// IWindowManager::ActivationSuspender
//////////////////////////////////////////////////////////////////////////////////////////////

struct IWindowManager::ActivationSuspender
{
	ActivationSuspender (IWindowManager& m, tbool suspend = true): windowManager (m), suspend (suspend)
	{
		wasSuspended = suspend ? windowManager.suspendActivation (true) : false;
	}
	~ActivationSuspender ()
	{
		 if(suspend)
			 windowManager.suspendActivation (wasSuspended);
	}
	IWindowManager& windowManager;
	tbool wasSuspended;
	tbool suspend;
};

} // namespace CCL

#endif // _ccl_iwindowmanager_h
