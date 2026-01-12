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
// Filename    : ccl/public/guiservices.h
// Description : User Interface Service APIs
//
//************************************************************************************************

#ifndef _ccl_guiservices_h
#define _ccl_guiservices_h

#include "ccl/public/cclexports.h"
#include "ccl/public/base/iunknown.h"

namespace CCL {

interface IUserInterface;
interface ICommandTable;
interface IHelpManager;
interface ISystemShell;
interface IDesktop;
interface IThemeManager;
interface IAlertService;
interface IWindowManager;
interface IWorkspaceManager;
interface IClipboard;
interface IView;
interface IPrintService;
interface IWindow;
interface INotificationCenter;
interface IAccessibilityManager;

namespace Configuration {
interface IRegistry; }

namespace System {

//////////////////////////////////////////////////////////////////////////////////////////////////
// GUI Service APIs
////////////////////////////////////////////////////////////////////////////////////////////////////
/** \addtogroup gui
@{ */

/** Check if host process is using CCL. */
CCL_EXPORT tbool CCL_API CCL_ISOLATED (IsFrameworkHostProcess) ();
inline tbool IsFrameworkHostProcess () { return CCL_ISOLATED (IsFrameworkHostProcess) (); }

/** Get GUI Management singleton */
CCL_EXPORT IUserInterface& CCL_API CCL_ISOLATED (GetGUI) ();
inline IUserInterface& GetGUI () { return CCL_ISOLATED (GetGUI) (); }

/** Get Command Table singleton */
CCL_EXPORT ICommandTable& CCL_API CCL_ISOLATED (GetCommandTable) ();
inline ICommandTable& GetCommandTable () { return CCL_ISOLATED (GetCommandTable) (); }

/** Get Help Manager singleton */
CCL_EXPORT IHelpManager& CCL_API CCL_ISOLATED (GetHelpManager) ();
inline IHelpManager& GetHelpManager () { return CCL_ISOLATED (GetHelpManager) (); }

/** Get System Shell singleton */
CCL_EXPORT ISystemShell& CCL_API CCL_ISOLATED (GetSystemShell) ();
inline ISystemShell& GetSystemShell () { return CCL_ISOLATED (GetSystemShell) (); }

/** Get Desktop singleton */
CCL_EXPORT IDesktop& CCL_API CCL_ISOLATED (GetDesktop) ();
inline IDesktop& GetDesktop () { return CCL_ISOLATED (GetDesktop) (); }

/** Get Theme Manager singleton */
CCL_EXPORT IThemeManager& CCL_API CCL_ISOLATED (GetThemeManager) ();
inline IThemeManager& GetThemeManager () { return CCL_ISOLATED (GetThemeManager) (); }

/** Get Alert Service singleton */
CCL_EXPORT IAlertService& CCL_API CCL_ISOLATED (GetAlertService) ();
inline IAlertService& GetAlertService () { return CCL_ISOLATED (GetAlertService) (); }

/** Get Window Manager singleton */
CCL_EXPORT IWindowManager& CCL_API CCL_ISOLATED (GetWindowManager) ();
inline IWindowManager& GetWindowManager () { return CCL_ISOLATED (GetWindowManager) (); }

/** Get Workspace Manager singleton */
CCL_EXPORT IWorkspaceManager& CCL_API CCL_ISOLATED (GetWorkspaceManager) ();
inline IWorkspaceManager& GetWorkspaceManager () { return CCL_ISOLATED (GetWorkspaceManager) (); }

/** Get Clipboard singleton */
CCL_EXPORT IClipboard& CCL_API CCL_ISOLATED (GetClipboard) ();
inline IClipboard& GetClipboard () { return CCL_ISOLATED (GetClipboard) (); }

/** Create framework view for plug-in */
CCL_EXPORT IView* CCL_API CCL_ISOLATED (CreateFrameworkView) (IUnknown* plugIn, StringID name);
inline IView* CreateFrameworkView (IUnknown* plugIn, StringID name) { return CCL_ISOLATED (CreateFrameworkView) (plugIn, name); }

/** Create child window for view */
CCL_EXPORT IWindow* CCL_API CCL_ISOLATED (CreateChildWindow) (IView* view, void* nativeParent);
inline IWindow* CreateChildWindow (IView* view, void* nativeParent) { return CCL_ISOLATED (CreateChildWindow) (view, nativeParent); }

/** Get framework configuration singleton */
CCL_EXPORT Configuration::IRegistry& CCL_API CCL_ISOLATED (GetFrameworkConfiguration) ();
inline Configuration::IRegistry& GetFrameworkConfiguration () { return CCL_ISOLATED (GetFrameworkConfiguration) (); }

/** Get Print Service singleton */
CCL_EXPORT IPrintService& CCL_API CCL_ISOLATED (GetPrintService) ();
inline IPrintService& GetPrintService () { return CCL_ISOLATED (GetPrintService) (); }

/** Get notification center singleton. */
CCL_EXPORT INotificationCenter& CCL_API CCL_ISOLATED (GetNotificationCenter) ();
inline INotificationCenter& GetNotificationCenter () { return CCL_ISOLATED (GetNotificationCenter) (); }

/** Get accessibility manager singleton. */
CCL_EXPORT IAccessibilityManager& CCL_API CCL_ISOLATED (GetAccessibilityManager) ();
inline IAccessibilityManager& GetAccessibilityManager () { return CCL_ISOLATED (GetAccessibilityManager) (); }

//////////////////////////////////////////////////////////////////////////////////////////////////
/** @}*/

#if CCL_STATIC_LINKAGE
/** GUI Framework Initialization. */
tbool CCL_API InitializeGUIFramework (tbool state);
#endif

} // namespace System
} // namespace CCL

#endif // _ccl_guiservices_h
