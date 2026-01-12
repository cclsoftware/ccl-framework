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
// Filename    : ccl/public/gui/iapplication.h
// Description : Application Interface
//
//************************************************************************************************

#ifndef _ccl_iapplication_h
#define _ccl_iapplication_h

#include "ccl/public/base/iunknown.h"

namespace CCL {

struct DragEvent;
interface IMenuBar;
interface ITheme;
interface IDragHandler;
interface IView;

//************************************************************************************************
// IApplication
/** Application interface. 
	\ingroup gui */
//************************************************************************************************

interface IApplication: IUnknown
{
	/** Get application identifier. */
	virtual StringID CCL_API getApplicationID () const = 0;

	/** Get application name. */
	virtual StringRef CCL_API getApplicationTitle () const = 0;
	
	/** Get application package identifier. */
	virtual StringID CCL_API getApplicationPackageID () const = 0;	

	/** Get application theme. */
	virtual ITheme* CCL_API getApplicationTheme () const = 0;

	/** Create the main menu bar. */
	virtual IMenuBar* CCL_API createMenuBar () = 0;

	/** Process command line received by this or other instance. */
	virtual void CCL_API processCommandLine (ArgsRef args) = 0;
	
	/** The OS has send an open file message */
	virtual tbool CCL_API openFile (UrlRef path) = 0;
	
	/** Create a drag handler for something dragged e.g. on the application window. */
	virtual IDragHandler* CCL_API createDragHandler (const DragEvent& event, IView* view) = 0;

	/** Try to quit application. */
	virtual tbool CCL_API requestQuit () = 0;
	
	/** Check if quit is requested. */
	virtual tbool CCL_API isQuitRequested () const = 0;	

	DECLARE_STRINGID_MEMBER (kComponentName)	///< application component name
	DECLARE_STRINGID_MEMBER (kAppUriScheme)		///< application URI scheme (optional, via IObject)

	// Signals received via IObserver:
	DECLARE_STRINGID_MEMBER (kAppActivated)		///< application has been activated and is in foreground now
	DECLARE_STRINGID_MEMBER (kUIInitialized)	///< called after UI has been initialized
	DECLARE_STRINGID_MEMBER (kAppDeactivated)	///< application will become inactive (in background, but still executing)
	DECLARE_STRINGID_MEMBER (kAppSuspended)		///< application execution will be suspended
	DECLARE_STRINGID_MEMBER (kAppResumed)		///< application execution has been resumed
	DECLARE_STRINGID_MEMBER (kAppTerminates)	///< application will be killed
	
	DECLARE_IID (IApplication)
};

DEFINE_IID (IApplication, 0x108db946, 0xa7dc, 0x4ed4, 0x8a, 0xa3, 0xeb, 0x9, 0xbf, 0xb5, 0xa4, 0x97)
DEFINE_STRINGID_MEMBER (IApplication, kComponentName, "Application")
DEFINE_STRINGID_MEMBER (IApplication, kAppUriScheme, "appUriScheme")
DEFINE_STRINGID_MEMBER (IApplication, kAppActivated, "appActivated")
DEFINE_STRINGID_MEMBER (IApplication, kUIInitialized, "uiInitialized")
DEFINE_STRINGID_MEMBER (IApplication, kAppDeactivated, "appDeactivated")
DEFINE_STRINGID_MEMBER (IApplication, kAppSuspended, "appSuspended")
DEFINE_STRINGID_MEMBER (IApplication, kAppResumed, "appResumed")
DEFINE_STRINGID_MEMBER (IApplication, kAppTerminates, "appTerminates")

} // namespace CCL

#endif // _ccl_iapplication_h
