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
// Filename    : ccl/extras/web/webxhrclient.h
// Description : XHR (XMLHttpRequest) Client
//
//************************************************************************************************

#ifndef _ccl_webxhrclient_h
#define _ccl_webxhrclient_h

#include "ccl/app/component.h"

#include "ccl/public/network/web/ixmlhttprequest.h"
#include "ccl/public/network/web/iwebcredentials.h"

namespace CCL {
namespace Web {

//************************************************************************************************
// IXHRCallback
//************************************************************************************************

interface IXHRCallback: IUnknown
{
	virtual void onEvent (IXMLHttpRequest& request, MessageRef msg) = 0;

	DECLARE_IID (IXHRCallback)
};

//************************************************************************************************
// XHRCallback
//************************************************************************************************

template <class T>
class XHRCallback: public Object,
				   public IXHRCallback
{
public:
	typedef void (T::*CallbackMethod) (IXMLHttpRequest& request, MessageRef msg);

	XHRCallback (T* handler, CallbackMethod method)
	: handler (handler),
	  method (method)
	{}

	static AutoPtr<IXHRCallback> make (T* handler, CallbackMethod method)
	{
		return AutoPtr<IXHRCallback> (NEW XHRCallback<T> (handler, method));
	}

	// IXHRCallback
	void onEvent (IXMLHttpRequest& request, MessageRef msg) override
	{
		(handler->*method) (request, msg);
	}

	CLASS_INTERFACE (IXHRCallback, Object)

protected:
	T* handler;
	CallbackMethod method;
};

//************************************************************************************************
// XHRClientCallback
//************************************************************************************************

template <class T>
class XHRClientCallback
{
public:
	typedef CCL::Web::XHRCallback<T> Callback;
};

//************************************************************************************************
// XHRClient
//************************************************************************************************

class XHRClient: public Component
{
public:
	DECLARE_CLASS (XHRClient, Component)

	XHRClient (StringRef name = nullptr);
	~XHRClient ();

	bool isBusy () const;
	
	/** Perform an asynchronous HTTP request. */
	bool startRequest (IXHRCallback* callback, StringID method, UrlRef url, IStream* data = nullptr, StringID contentType = nullptr);

	/** Perform an asynchronous HTTP request with JSON data. */
	bool startJsonRequest (IXHRCallback* callback, StringID method, UrlRef url, const Attributes& data);

	/** Abort any request previously started */
	void abortRequest ();

	/** Credentials to be sent in the Authorization header*/
	void setCredentials (IWebCredentials* c) { credentials.share (c); }
	
	// Component
	tresult CCL_API terminate () override;
	tbool CCL_API getProperty (Variant& var, MemberID propertyId) const override;

private:
	AutoPtr<IXMLHttpRequest> request;
	SharedPtr<IXHRCallback> callback;
	AutoPtr<IWebCredentials> credentials;
	
	void onRequestEvent (MessageRef msg);
};

} // naemspace Web
} // namespace CCL

#endif // _ccl_webxhrclient_h
