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
// Filename    : ccl/platform/shared/host/frameworkalert.h
// Description : Alert box implementation using generic framework controls only
//
//************************************************************************************************

#ifndef _ccl_frameworkalert_h
#define _ccl_frameworkalert_h

#include "ccl/base/asyncoperation.h"

#include "ccl/gui/dialogs/alert.h"
#include "ccl/gui/dialogs/dialogbuilder.h"

#include "ccl/public/gui/icontroller.h"
#include "ccl/public/gui/paramlist.h"
#include "ccl/public/gui/iparamobserver.h"

namespace CCL {

//************************************************************************************************
// FrameworkAlertBox
//************************************************************************************************

class FrameworkAlertBox: public AlertBox,
						 public AbstractController,
						 public IParamObserver
{
public:
	DECLARE_CLASS (FrameworkAlertBox, AlertBox)
	
	FrameworkAlertBox ();
	~FrameworkAlertBox ();

	// AlertBox
	void closePlatform () override;
	IAsyncOperation* runAsyncPlatform () override;
	tbool CCL_API getProperty (Variant& var, MemberID propertyId) const override;

	// IParamObserver
	tbool CCL_API paramChanged (IParameter* param) override;
	void CCL_API paramEdit (IParameter* param, tbool begin) override {}
	
	// IController
	DECLARE_PARAMETER_LOOKUP (paramList)
	
	CLASS_INTERFACE2 (IController, IParamObserver, AlertBox)

protected:
	AutoPtr<DialogBuilder> dialogBuilder;
	SharedPtr<AsyncOperation> operation;
	ParamList paramList;
};

} // namespace CCL

#endif // _ccl_frameworkalert_h
