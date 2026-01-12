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
// Filename    : ccl/system/errorhandler.h
// Description : Error Handler
//
//************************************************************************************************

#ifndef _ccl_errorhandler_h
#define _ccl_errorhandler_h

#include "ccl/base/object.h"

#include "ccl/public/system/ierrorhandler.h"

namespace CCL {

//************************************************************************************************
// ErrorHandler
//************************************************************************************************

class ErrorHandler:	public Object,
					public IErrorHandler
{
public:
	DECLARE_CLASS (ErrorHandler, Object)

	static ErrorHandler& instance ();

	// Alert::IReporter
	void CCL_API reportEvent (const Alert::Event& e) override;
	void CCL_API setReportOptions (Severity minSeverity, int eventFormat) override;

	// IErrorHandler
	tresult CCL_API beginContext () override;
	tresult CCL_API endContext () override;
	IErrorContext* CCL_API peekContext () override;
	int CCL_API getContextDepth () override;
	tresult CCL_API pushToParent (IErrorContext* context) override;

	CLASS_INTERFACE2 (IErrorHandler, IReporter, Object)
};

} // namespace CCL

#endif // _ccl_errorhandler_h
