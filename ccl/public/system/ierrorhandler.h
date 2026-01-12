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
// Filename    : ccl/public/system/ierrorhandler.h
// Description : Error Handler Interface
//
//************************************************************************************************

#ifndef _ccl_ierrorhandler_h
#define _ccl_ierrorhandler_h

#include "ccl/public/system/alerttypes.h"

namespace CCL {

//////////////////////////////////////////////////////////////////////////////////////////////////
// Signals::kErrorHandler
//////////////////////////////////////////////////////////////////////////////////////////////////

/** \ingroup ccl_system */
namespace Signals
{
	/** Signals related to Error Handler */
	DEFINE_STRINGID (kErrorHandler, "CCL.ErrorHandler")
		
		/** [OUT] A crash is being reported arg[0]: IStream */
		DEFINE_STRINGID (kCrashReported, "CrashReported")

		/** [OUT] Low memory notification. */
		DEFINE_STRINGID (kLowMemoryWarning, "LowMemoryWarning")
}

//************************************************************************************************
// IErrorContext
/** \ingroup ccl_system */
//************************************************************************************************

interface IErrorContext: IUnknown
{
	virtual int CCL_API getEventCount () const = 0;

	virtual AlertEventRef CCL_API getEvent (int index) const = 0;

	virtual int CCL_API getChildCount () const = 0;

	virtual IErrorContext* CCL_API getChild (int index) const = 0;
	
	virtual void CCL_API removeAll () = 0;

	DECLARE_IID (IErrorContext)
};

DEFINE_IID (IErrorContext, 0xde996142, 0x2846, 0x47c2, 0x86, 0x53, 0x82, 0xa8, 0xf5, 0x24, 0xa8, 0xbf)

//************************************************************************************************
// IErrorHandler
/** Handler interface for structured error handling.

	Threading Policy: 
	Contexts and error events are handled on a per-thread basis.
	
	\ingroup ccl_system */
//************************************************************************************************

interface IErrorHandler: Alert::IReporter
{
	/*
		IReporter::reportEvent(): Raise error event in current context of calling thread.
		The event is swallowed if beginContext() has not been called before.
	*/

	/** Begin a new error context in calling thread. */
	virtual tresult CCL_API beginContext () = 0;
	
	/** End current error context of calling thread. */
	virtual tresult CCL_API endContext () = 0;

	/** Get current error context of calling thread, returns null if none present. */
	virtual IErrorContext* CCL_API peekContext () = 0;

	/** Get depth of error context nesting of calling thread. */
	virtual int CCL_API getContextDepth () = 0;
	
	/** Push events from given error context to its parent. */
	virtual tresult CCL_API pushToParent (IErrorContext* context) = 0;

	DECLARE_IID (IErrorHandler)
};

DEFINE_IID (IErrorHandler, 0x99750c28, 0x9758, 0x4de4, 0xb7, 0xf2, 0xec, 0x5f, 0x15, 0x30, 0x93, 0xef)

} // namespace CCL

#endif // _ccl_ierrorhandler_h
