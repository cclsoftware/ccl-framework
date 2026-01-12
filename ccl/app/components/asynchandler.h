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
// Filename    : ccl/app/components/asynchandler.h
// Description : Asynchronous Operation Handler
//
//************************************************************************************************

#ifndef _asynchandler_h
#define _asynchandler_h

#include "ccl/app/component.h"

#include "ccl/base/asyncoperation.h"

namespace CCL {

interface IModalProgressDialog;

//************************************************************************************************
// AsyncCallHandler
//************************************************************************************************
// LATER TODO: This is very similar to AsyncBatchTask, except cancelation - see batchoperation.h!

class AsyncCallHandler: public Component
{
public:
	DECLARE_CLASS (AsyncCallHandler, Component)

	AsyncCallHandler (StringRef name = nullptr);
	~AsyncCallHandler ();

	IAsyncOperation* performAsync (IAsyncCall* call, StringRef description, StringRef title = nullptr);
	Variant performModal (IAsyncCall* call, StringRef description, StringRef title = nullptr);

	template <typename T>
	IAsyncOperation* performAsync (const T& lambda, StringRef description, StringRef title = nullptr)
	{
		return performAsync (static_cast<IAsyncCall*> (AsyncCall::make (lambda)), description, title);
	}

	template <typename T>
	Variant performModal (const T& lambda, StringRef description, StringRef title = nullptr)
	{
		return performModal (static_cast<IAsyncCall*> (AsyncCall::make (lambda)), description, title);
	}

	// Component
	void CCL_API notify (ISubject* subject, MessageRef msg) override;

private:
	IModalProgressDialog* currentProgressDialog;
	SharedPtr<IAsyncOperation> pendingOperation;

	void onCallCompleted (IAsyncOperation& operation);
};

} // namespace CCL

#endif // _asynchandler_h
