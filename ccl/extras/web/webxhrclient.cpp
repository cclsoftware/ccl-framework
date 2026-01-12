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
// Filename    : ccl/extras/web/webxhrclient.cpp
// Description : XHR (XMLHttpRequest) Client
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "ccl/extras/web/webxhrclient.h"

#include "ccl/base/message.h"
#include "ccl/base/storage/jsonarchive.h"

#include "ccl/public/storage/iurl.h"
#include "ccl/public/base/memorystream.h"
#include "ccl/public/gui/iparameter.h"
#include "ccl/public/network/web/iwebrequest.h"
#include "ccl/public/plugservices.h"

using namespace CCL;
using namespace Web;

//************************************************************************************************
// IXHRCallback
//************************************************************************************************

DEFINE_IID_ (IXHRCallback, 0xccb9d574, 0x75b9, 0x4b25, 0x85, 0x2e, 0x3b, 0x8e, 0x38, 0xd3, 0x7c, 0xba)

//************************************************************************************************
// XHRClient
//************************************************************************************************

DEFINE_CLASS_HIDDEN (XHRClient, Component)

//////////////////////////////////////////////////////////////////////////////////////////////////

XHRClient::XHRClient (StringRef name)
: Component (name)
{
	request = ccl_new<IXMLHttpRequest> (ClassID::XMLHttpRequest);
	signalSlots.advise (UnknownPtr<ISubject> (request), nullptr, this, &XHRClient::onRequestEvent);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

XHRClient::~XHRClient ()
{
	signalSlots.unadvise (UnknownPtr<ISubject> (request));
	request.release ();
	
	cancelSignals ();
}

/////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API XHRClient::terminate ()
{
	request->abort ();
	callback.release ();

	return SuperClass::terminate ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool XHRClient::isBusy () const
{
	return request->getState () == IAsyncInfo::kStarted;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool XHRClient::startRequest (IXHRCallback* _callback, StringID method, UrlRef url, IStream* data, StringID contentType)
{
	ASSERT (!isBusy ())
	if(isBusy ())
		return false;

	ASSERT (callback == nullptr)
	callback = _callback;
	
	if(credentials)
		request->open (method, url, true, credentials->getUserName (), credentials->getPassword (), credentials->getAuthType ());
	else
		request->open (method, url);

	if(!contentType.isEmpty ())
		request->setRequestHeader (Meta::kContentType, contentType);

	request->send (data);
	signal (Message (kPropertyChanged)); // "isBusy" property
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool XHRClient::startJsonRequest (IXHRCallback* callback, StringID method, UrlRef url, const Attributes& data)
{
	// serialize JSON
	AutoPtr<MemoryStream> jsonStream = NEW MemoryStream;
	JsonArchive (*jsonStream).saveAttributes (nullptr, data);
	jsonStream->rewind ();

	#if DEBUG_LOG
	String urlString;
	url.getUrl (urlString, true);
	CCL_PRINTF ("=== POST %s ===\n", MutableCString (urlString).str ())
	String jsonString;
	jsonString.appendCString (Text::kUTF8, (CStringPtr)jsonStream->getMemoryAddress (), jsonStream->getBytesWritten ());
	CCL_PRINTLN (jsonString)
	#endif

	return startRequest (callback, method, url, jsonStream, JsonArchive::kMimeType);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void XHRClient::abortRequest ()
{
	request->abort ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void XHRClient::onRequestEvent (MessageRef msg)
{
	if(callback)
		callback->onEvent (*request, msg);

	if(msg == IXMLHttpRequest::kOnLoadEnd || msg == IXMLHttpRequest::kOnAbort)
	{
		callback.release ();
		signal (Message (kPropertyChanged)); // "isBusy" property
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API XHRClient::getProperty (Variant& var, MemberID propertyId) const
{
	if(propertyId == "isBusy")
	{
		var = isBusy ();
		return true;
	}
	return SuperClass::getProperty (var, propertyId);
}
