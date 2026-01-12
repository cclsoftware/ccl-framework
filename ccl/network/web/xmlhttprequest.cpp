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
// Filename    : ccl/network/web/xmlhttprequest.cpp
// Description : XMLHttpRequest class
//
//************************************************************************************************
/*
	XMLHttpRequest
	http://www.w3.org/TR/XMLHttpRequest/
	https://developer.mozilla.org/en/XMLHttpRequest

	IXMLHTTPRequest2 interface on MSDN:
	http://msdn.microsoft.com/en-us/library/windows/desktop/hh831151%28v=vs.85%29.aspx

	Old IXMLHTTPRequest interface on MSDN:
	http://msdn.microsoft.com/en-us/library/windows/desktop/ms760305%28v=vs.85%29.aspx
*/

#include "ccl/network/web/xmlhttprequest.h"

#include "ccl/base/message.h"
#include "ccl/base/collections/stringdictionary.h"

#include "ccl/public/base/memorystream.h"
#include "ccl/public/systemservices.h"

#include "ccl/public/network/web/httpstatus.h"
#include "ccl/public/network/web/iwebservice.h"
#include "ccl/public/netservices.h"

using namespace CCL;
using namespace Web;

//************************************************************************************************
// XMLHttpRequest
//************************************************************************************************

DEFINE_CLASS (XMLHttpRequest, Object)
DEFINE_CLASS_NAMESPACE (XMLHttpRequest, "Network")
DEFINE_CLASS_UID (XMLHttpRequest, 0xeabd31c, 0x6cd1, 0x4ff4, 0xa7, 0xc0, 0xb2, 0xb9, 0xf8, 0x54, 0xa7, 0xf3)
void XMLHttpRequest::forceLinkage () {} // force linkage of this file

//////////////////////////////////////////////////////////////////////////////////////////////////

XMLHttpRequest::XMLHttpRequest ()
: readyState (kUnsent),
  flags (0),
  async (true),
  status (0)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

XMLHttpRequest::~XMLHttpRequest ()
{
	if(async)
	{
		cancel ();
		cancelSignals ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IAsyncInfo::State CCL_API XMLHttpRequest::getState () const
{
	if(isError ())
		return kFailed;
	if(readyState == kDone)
		return kCompleted;
	if(isSending ())
		return kStarted;
	return IAsyncInfo::kNone;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

XMLHttpRequest::ReadyState CCL_API XMLHttpRequest::getReadyState () const
{
	return readyState;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IStream* CCL_API XMLHttpRequest::getResponseStream () const
{
	return responseStream;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API XMLHttpRequest::getStatus () const
{
	return status;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void XMLHttpRequest::setState (State state)
{
	if(readyState != state)
	{
		readyState = state;
		signalEvent (Message (kOnReadyStateChange));
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void XMLHttpRequest::signalEvent (MessageRef msg)
{
	if(async)
	{
		if(System::IsInMainThread ())
			signal (msg);
		else
			deferSignal (NEW Message (msg));
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void XMLHttpRequest::cancel ()
{
	if(isSending ())
	{
		ASSERT (async == true)
		if(!async)
			return;

		System::GetWebService ().cancelOperation (this);
		isSending (false);
		isError (true);
		signalEvent (Message (kOnAbort));
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void XMLHttpRequest::reset ()
{
	//url = Url ();
	method.empty ();
	credentials.release ();
	requestHeaders.release ();
	responseStream.release ();
	responseHeaders.release ();
	flags = 0;
	status = 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API XMLHttpRequest::abort ()
{
	cancel ();
	reset ();
	setState (kUnsent);
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API XMLHttpRequest::open (StringID method, UrlRef url, tbool async, 
									  StringRef user, StringRef password, StringRef authType)
{
	abort ();

	this->async = async != 0;
	this->method = method;
	this->url.assign (url);

	if(!user.isEmpty () || !password.isEmpty ())
	{
		credentials = System::GetWebService ().createCredentials ();
		credentials->assign (user, password, authType);
	}

	setState (kOpened);
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IWebHeaderCollection& XMLHttpRequest::getRequestHeaders ()
{
	if(!requestHeaders)
		requestHeaders = System::GetWebService ().createHeaderCollection ();
	return *requestHeaders;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API XMLHttpRequest::setRequestHeader (StringID header, StringID value)
{
	ASSERT (readyState == kOpened)
	if(readyState != kOpened)
		return kResultUnexpected;

	getRequestHeaders ().getEntries ().setEntry (header, value);
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IStream* XMLHttpRequest::createStream (MutableCString& contentType, VariantRef data) const
{
	IStream* stream = nullptr;
	if(requestHeaders) // content type can be set via setRequestHeader()
		contentType = requestHeaders->getEntries ().lookupValue (Meta::kContentType);

	if(data.isObject ())
		stream = UnknownPtr<IStream> (data.asUnknown ()).detach ();

	if(!stream)
	{
		stream = NEW MemoryStream;
		if(data.isString ())
		{
			MutableCString utf8 (data.asString (), Text::kUTF8);
			stream->write (utf8.str (), utf8.length ());	
			contentType = "text/plain;charset=UTF-8";
		}
	}

	if(contentType.isEmpty ())
		contentType = Meta::kBinaryContentType;

	return stream;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API XMLHttpRequest::send (VariantRef data, IProgressNotify* progress)
{
	ASSERT (readyState == kOpened)
	if(readyState != kOpened)
		return kResultUnexpected;

	ASSERT (!async || progress == nullptr)
	return async ? sendAsync (data) : sendBlocking (data, progress);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult XMLHttpRequest::sendAsync (VariantRef data)
{
	if(method == HTTP::kPOST || method == HTTP::kPUT || method == HTTP::kPATCH || method == HTTP::kDELETE)
	{
		MutableCString contentType;
		AutoPtr<IStream> localData = createStream (contentType, data);		
		IWebHeaderCollection& headers = getRequestHeaders ();
		headers.getEntries ().setEntry (Meta::kContentType, contentType);

		System::GetWebService ().uploadInBackground (this, url, *localData, &headers, method, credentials); 
	}
	else
	{
		ASSERT (method == HTTP::kGET)		
		ASSERT (data.isNil ())
		responseStream = NEW MemoryStream;

		System::GetWebService ().downloadInBackground (this, url, *responseStream, credentials);
	}

	isSending (true);
	signalEvent (Message (kOnLoadStart));
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult XMLHttpRequest::sendBlocking (VariantRef data, IProgressNotify* progress)
{
	isSending (true);
	responseStream = NEW MemoryStream;
	bool success = false;

	if(method == HTTP::kPOST || method == HTTP::kPUT)
	{
		MutableCString contentType;
		AutoPtr<IStream> localData = createStream (contentType, data);
		IWebHeaderCollection& headers = getRequestHeaders ();
		headers.getEntries ().setEntry (Meta::kContentType, contentType);

		success = System::GetWebService ().uploadData (url, *localData, &headers, *responseStream, 
													   method, credentials, progress, &status) == kResultOk;
	}
	else
	{
		ASSERT (method == HTTP::kGET)
		ASSERT (data.isNil ())

		// TODO: add support for additional headers!
		success = System::GetWebService ().downloadData (url, *responseStream, credentials, nullptr, progress, &status) == kResultOk;
	}

	isSending (false);
	isError (!success);
	setState (kDone);
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IWebHeaderCollection* CCL_API  XMLHttpRequest::getAllResponseHeaders () const
{
	return responseHeaders;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API XMLHttpRequest::getResponseHeader (CString& result, StringID id) const
{
	if(responseHeaders)
	{
		CStringRef value = responseHeaders->getEntries ().lookupValue (id);
		if(!value.isEmpty ())
		{
			result = value;
			return kResultOk;
		}
	}
	return kResultFailed;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API XMLHttpRequest::notify (ISubject* subject, MessageRef msg)
{
	SharedPtr<Object> keeper (this); // keep this alive during notify() call!

	if(msg == Meta::kBackgroundProgressNotify)
	{
		double value = msg[0];
		int flags = msg[1];

		signalEvent (Message (kOnProgress, value, flags));
	}
	else if(msg == Meta::kContentLengthNotify)
	{
		UnknownPtr<IWebHeaderCollection> headers (msg[1].asUnknown ());
		if(readyState < kHeadersReceived)
		{
			responseHeaders = headers.detach ();
			ASSERT (responseHeaders.isValid ())
			setState (kHeadersReceived);

			// now the response body is being received
			setState (kLoading);
		}
		#if DEBUG
		else // chunked transfers notify multiple times
			SOFT_ASSERT (headers && headers->isChunkedTransfer (), "Chunked transfer expected!\n")
		#endif
	}
	else if(msg == Meta::kDownloadComplete || msg == Meta::kUploadComplete)
	{
		bool success = msg[0].asResult () == kResultOk; // error check at network level
		this->status = msg[1].asInt (); // status at application level, can be an HTTP error code
		if(msg == Meta::kUploadComplete)
		{
			responseStream.share (UnknownPtr<IStream> (msg[2].asUnknown ()));
			//ASSERT (responseStream.isValid ()) can be null when request failed
		}

		isSending (false);
		isError (!success);
		setState (kDone);
		signalEvent (Message (success ? kOnLoad : kOnError));
		signalEvent (Message (kOnLoadEnd));
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_PROPERTY_NAMES (XMLHttpRequest)
	DEFINE_PROPERTY_NAME ("readyState")
	DEFINE_PROPERTY_NAME ("status")
	// TODO: response, responseText, responseXML???
END_PROPERTY_NAMES (XMLHttpRequest)

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API XMLHttpRequest::getProperty (Variant& var, MemberID propertyId) const
{
	if(propertyId == "readyState")
	{
		var = readyState;
		return true;
	}
	if(propertyId == "status")
	{
		var = status;
		return true;
	}
	return SuperClass::getProperty (var, propertyId);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_METHOD_NAMES (XMLHttpRequest)
	DEFINE_METHOD_NAME ("abort")
	DEFINE_METHOD_ARGS ("open", "method, url, async=true, user='', password=''")
	DEFINE_METHOD_ARGS ("setRequestHeader", "header, value")
	DEFINE_METHOD_ARGS ("send", "data=0")
END_METHOD_NAMES (XMLHttpRequest)

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API XMLHttpRequest::invokeMethod (Variant& returnValue, MessageRef msg)
{
	if(msg == "abort")
	{
		returnValue = abort ();
		return true;
	}
	else if(msg == "open")
	{
		MutableCString method (msg[0].asString ());
		
		AutoPtr<IUrl> url;
		if(msg[1].isObject ())
			url = UnknownPtr<IUrl> (msg[1].asUnknown ()).detach ();
		if(url == nullptr)
			url = NEW Url (msg[1].asString ());

		bool async = msg.getArgCount () > 2 ? msg[2].asBool () : true;
		String user (msg.getArgCount () > 3 ? msg[3].asString () : "");
		String password (msg.getArgCount () > 4 ? msg[4].asString () : "");

		returnValue = open (method, *url, async, user, password);
		return true;
	}
	else if(msg == "setRequestHeader")
	{
		MutableCString header (msg[0].asString (), Text::kUTF8);
		MutableCString value (msg[1].asString (), Text::kUTF8);

		returnValue = setRequestHeader (header, value);
		return true;
	}
	else if(msg == "send")
	{
		returnValue = send (msg.getArgCount () > 0 ? msg[0] : Variant ());
		return true;
	}
	return SuperClass::invokeMethod (returnValue, msg);
}
