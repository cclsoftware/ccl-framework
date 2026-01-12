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
// Filename    : ccl/public/system/cclerror.h
// Description : Structured Error Handling Helpers
//
//************************************************************************************************

#ifndef _cclerror_h
#define _cclerror_h

#include "ccl/public/system/ierrorhandler.h"

namespace CCL {

//************************************************************************************************
// ErrorContextGuard
/** \ingroup ccl_system */
//************************************************************************************************

class ErrorContextGuard
{
public:
	ErrorContextGuard ();
	~ErrorContextGuard ();
	
	bool hasErrors (bool deep = false) const;
	tresult getResultCode (bool deep = true) const;
	void reset ();
	void removeAll ();
	
	void pushToParent ();

	operator IErrorContext* ();
	IErrorContext* operator -> ();
	
private:
	IErrorContext* context;
	void beginContext ();
	void endContext ();
	
	static bool hasErrors (IErrorContext* context, bool deep);
	static tresult getResultCode (IErrorContext* context, bool deep);
};

//************************************************************************************************
// Helper functions
//************************************************************************************************

/** Shorcut to raise an error event via global Error Handler. 
	\ingroup ccl_system */
void ccl_raise (AlertEventRef e);

/** Shorcut to raise an error event via global Error Handler.
	\ingroup ccl_system */
void ccl_raise (StringRef message, tresult errorCode = kResultFailed);

} // namespace CCL

#endif // _cclerror_h
