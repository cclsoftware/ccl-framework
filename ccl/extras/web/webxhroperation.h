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
// Filename    : ccl/extras/web/webxhroperation.h
// Description : Web XHR (XMLHttpRequest) Operation
//
//************************************************************************************************

#ifndef _ccl_webxhroperation_h
#define _ccl_webxhroperation_h

#include "ccl/base/asyncoperation.h"

#include "ccl/public/base/iprogress.h"
#include "ccl/public/network/web/ixmlhttprequest.h"

namespace CCL {
namespace Web {

//************************************************************************************************
// AsyncXHROperation
/** Wraps IXMLHttpRequest into IAsyncOperation. */
//************************************************************************************************

class AsyncXHROperation: public CCL::AsyncOperation
{
public:
	AsyncXHROperation (IXMLHttpRequest* httpRequest) ///< takes ownership!
	: httpRequest (httpRequest)
	{
		ISubject::addObserver (httpRequest, this);
	}

	~AsyncXHROperation ()
	{
		ISubject::removeObserver (httpRequest, this);
	
		setProgressHandler (nullptr);
	}

	// AsyncOperation
	tresult CCL_API queryInterface (UIDRef iid, void** ptr) override
	{
		if(iid == ccl_iid<IXMLHttpRequest> ()) // make HTTP request accessible
			return httpRequest->queryInterface (iid, ptr);

		return AsyncOperation::queryInterface (iid, ptr);
	}

	void CCL_API cancel () override
	{ 
		SharedPtr<AsyncXHROperation> keeper (this);
		httpRequest->abort ();
	}

	void CCL_API notify (ISubject* subject, MessageRef msg) override
	{
		SharedPtr<Object> keeper (this); // keep this alive during notify() call!

		if(msg == IXMLHttpRequest::kOnProgress)
		{
			IProgressNotify* progressHandler = getProgressHandler ();
			if(progressHandler)
			{
				double value = msg[0];
				int flags = msg[1];
				progressHandler->updateProgress (IProgressNotify::State (value, flags));
			}
		}	
		else if(msg == IXMLHttpRequest::kOnAbort)
		{
			setState (kCanceled);
		}

		// keep canceled state when XHR object is reset, operation result remains undefined in this case
		if(getState () == kCanceled)
			return;

		State state = httpRequest->getState ();
		if(state != getState ()) // XHR object signals more often than state changes!
		{
			if(state == kCompleted || state == kFailed)
				onHttpRequestFinished ();
			
			setState (state);
		}
	}

protected:
	AutoPtr<IXMLHttpRequest> httpRequest;

	virtual void onHttpRequestFinished ()
	{
		// default behavior is to store HTTP status as operation result
		setResult (httpRequest->getStatus ());
	}
};

} // namespace Web
} // namespace CCL

#endif // _ccl_webxhroperation_h
