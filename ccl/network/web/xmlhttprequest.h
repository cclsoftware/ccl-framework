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
// Filename    : ccl/network/web/xmlhttprequest.h
// Description : XMLHttpRequest class
//
//************************************************************************************************

#ifndef _ccl_xmlhttprequest_h
#define _ccl_xmlhttprequest_h

#include "ccl/base/storage/url.h"

#include "ccl/public/base/istream.h"
#include "ccl/public/network/web/ixmlhttprequest.h"
#include "ccl/public/network/web/iwebcredentials.h"
#include "ccl/public/network/web/iwebrequest.h"

namespace CCL {
namespace Web {

//************************************************************************************************
// XMLHttpRequest
//************************************************************************************************

class XMLHttpRequest: public Object,
					  public IXMLHttpRequest
{
public:
	DECLARE_CLASS (XMLHttpRequest, Object)
	DECLARE_METHOD_NAMES (XMLHttpRequest)
	DECLARE_PROPERTY_NAMES (XMLHttpRequest)

	XMLHttpRequest ();
	~XMLHttpRequest ();

	static void forceLinkage ();
	
	// IAsyncInfo
	State CCL_API getState () const override;

	// IXMLHttpRequest
	ReadyState CCL_API getReadyState () const override;
	IStream* CCL_API getResponseStream () const override;
	int CCL_API getStatus () const override;
	tresult CCL_API abort () override;
	tresult CCL_API open (StringID method, UrlRef url, tbool async = true, 
						  StringRef user = nullptr, StringRef password = nullptr, StringRef authType = nullptr) override;
	tresult CCL_API setRequestHeader (StringID header, StringID value) override;
	tresult CCL_API send (VariantRef data = 0, IProgressNotify* progress = nullptr) override;
	IWebHeaderCollection* CCL_API getAllResponseHeaders () const override;
	tresult CCL_API getResponseHeader (CString& result, StringID id) const override;

	// Object
	void CCL_API notify (ISubject* subject, MessageRef msg) override;

	CLASS_INTERFACE2 (IXMLHttpRequest, IAsyncInfo, Object)

protected:
	ReadyState readyState;
	AutoPtr<IWebHeaderCollection> responseHeaders;
	int flags;
	bool async;
	MutableCString method;
	AutoPtr<IWebCredentials> credentials;
	AutoPtr<IWebHeaderCollection> requestHeaders;
	Url url;
	AutoPtr<IStream> responseStream;
	int status;

	void setState (State state);
	void signalEvent (MessageRef msg);
	void cancel ();
	void reset ();

	IWebHeaderCollection& getRequestHeaders ();
	IStream* createStream (MutableCString& contentType, VariantRef data) const;
	tresult sendAsync (VariantRef data);
	tresult sendBlocking (VariantRef data, IProgressNotify* progress);

	enum Flags
	{
		kIsSending = 1<<0,
		kIsError = 1<<1
	};

	PROPERTY_FLAG (flags, kIsSending, isSending)
	PROPERTY_FLAG (flags, kIsError, isError)

	// IObject
	tbool CCL_API getProperty (Variant& var, MemberID propertyId) const override;
	tbool CCL_API invokeMethod (Variant& returnValue, MessageRef msg) override;
};

} // naemspace Web
} // namespace CCL

#endif // _ccl_xmlhttprequest_h
