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
// Filename    : ccl/public/system/isafetymanager.h
// Description : Safety Manager Interface
//
//************************************************************************************************

#ifndef _ccl_isafetymanager_h
#define _ccl_isafetymanager_h

#include "ccl/public/base/irecognizer.h"
#include "ccl/public/base/iarrayobject.h"
#include "ccl/public/collections/iunknownlist.h"
#include "ccl/public/text/cstring.h"
#include "ccl/public/text/cclstring.h"

namespace CCL {

//////////////////////////////////////////////////////////////////////////////////////////////////
// Safety Management Signals
//////////////////////////////////////////////////////////////////////////////////////////////////

namespace Signals 
{
	/** Signals related to safety management */
	DEFINE_STRINGID (kSafetyManagement, "CCL.Safety")

		/** (OUT) Safety options changed. */
		DEFINE_STRINGID (kSafetyOptionsChanged, "SafetyOptionsChanged")

		/** (OUT) One or more modules behaved unexpectedly. arg[0]: List of IUrl's, module paths. */
		DEFINE_STRINGID (kModuleException, "ModuleException")
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// Safety IDs
//////////////////////////////////////////////////////////////////////////////////////////////////

namespace SafetyID
{ 
	const CStringPtr kStartupSafetyOption = "startupSafetyOption";
}
	
//************************************************************************************************
// ICrashReport
/** \ingroup ccl_system */
//************************************************************************************************

interface ICrashReport: IUnknown
{
	/** Get descriptions of actions that were pending when the application crashed. */
	virtual const IArrayObject& CCL_API getLastActionsBeforeCrash () const = 0;
	
	/** Get the path to the module that caused the crash. */
	virtual UrlRef CCL_API getModuleCausingCrash () const = 0;

	/** Get the path to a crash dump file. */
	virtual UrlRef CCL_API getSystemDumpPath () const = 0;
	
	/** Get paths to modules that behaved unexpectedly during the last session. */
	virtual const IUnknownList& CCL_API getUnstableModules () const = 0;

	/** Get paths to modules that were in the callstack during the crash. */
	virtual const IUnknownList& CCL_API getCallingModules () const = 0;
	
	/** Check if the application terminated cleanly despite crashes or unexpected behavior in modules. */
	virtual tbool CCL_API didShutdownCleanly () const = 0;

	DECLARE_IID (ICrashReport)
};

DEFINE_IID (ICrashReport, 0xea8da010, 0x8ff7, 0x488b, 0x88, 0xf6, 0xd8, 0xea, 0x5, 0x64, 0x9c, 0xf0)

//************************************************************************************************
// ISafetyManager
/** \ingroup ccl_system */
//************************************************************************************************

interface ISafetyManager: IUnknown
{
	enum Features
	{
		kObjectFilters = 1 << 0, //< Use object filters to prevent instantiating unsafe objects
		kCrashDetection = 1 << 1, //< Detect crashes and provide crash information
		kCrashRecovery = 1 << 2, //< Try to recover from crashes, e.g. restart the application

		kEnableAll = kObjectFilters|kCrashDetection|kCrashRecovery
	};

	virtual void CCL_API setSafetyOptions (int features) = 0;

	//////////////////////////////////////////////////////////////////////////////////////////////
	// Safety Options
	//////////////////////////////////////////////////////////////////////////////////////////////
	
	/** Set the value of a safety option. */
	virtual void CCL_API setValue (CStringRef safetyOptionId, tbool state) = 0;

	/** Get the current value of a safety option. */
	virtual tbool CCL_API getValue (CStringRef safetyOptionId) const = 0;
	
	//////////////////////////////////////////////////////////////////////////////////////////////
	// Object Filters
	//////////////////////////////////////////////////////////////////////////////////////////////
	
	/** Add an object filter which can be used to filter unsafe objects. Takes ownership. */
	virtual tresult CCL_API addFilter (IObjectFilter* filter) = 0;

	/** Get the number of object filters. */
	virtual int CCL_API countFilters () const = 0;

	/** Get the object filter at given index. */
	virtual const IObjectFilter* CCL_API getFilter (int index) const = 0;

	/** Get a combined filter which matches if any of the safety manager's filters matches. */
	virtual const IObjectFilter& CCL_API getCombinedFilter () const = 0;
	
	//////////////////////////////////////////////////////////////////////////////////////////////
	// Crash Detection
	//////////////////////////////////////////////////////////////////////////////////////////////
	
	/** Register an action context id with a localized title. */
	virtual void CCL_API registerAction (CStringRef actionId, StringRef title) = 0;

	/** Register the begin of an action. */
	virtual void CCL_API beginAction (CStringRef actionId, const String arguments[], int argumentCount) = 0;

	/** End a previously registered action. */
	virtual void CCL_API endAction () = 0;
	
	/** Check if the application crashed the last time it ran. */
	virtual ICrashReport* CCL_API detectCrash () = 0;
	
	/** Check for unexpected behavior in the current process. To be called periodically. */
	virtual tresult CCL_API checkStability () = 0;
	
	/** Report an exception. */
	virtual void CCL_API reportException (void* exceptionInformation, const uchar* systemDumpFile) = 0;

	/** Handle an exception. Call in a catch block. */
	virtual tbool CCL_API handleException () = 0;
	
	DECLARE_IID (ISafetyManager)
};

DEFINE_IID (ISafetyManager, 0x113d2354, 0x9e87, 0x4f79, 0x98, 0x1, 0x8c, 0x31, 0x19, 0x3f, 0x39, 0x86)

} // namespace CCL

#endif // _ccl_isafetymanager_h
