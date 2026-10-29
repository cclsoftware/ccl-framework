//************************************************************************************************
//
// This file is part of Crystal Class Library (R)
// Copyright (c) 2026 CCL Software Licensing GmbH.
// All Rights Reserved.
//
// Licensed for use under either:
//  1. a Commercial License provided by CCL Software Licensing GmbH, or
//  2. GNU Affero General Public License v3.0 (AGPLv3).
//
// You must choose and comply with one of the above licensing options.
// For more information, please visit ccl.dev.
//
// Filename    : ccl/platform/wasm/network/httpclient.wasm.cpp
// Description : WebAssembly HTTP Client using Emscripten fetch API
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "ccl/platform/wasm/network/httpclient.wasm.h"
#include "ccl/platform/wasm/network/webengine.wasm.h"

#include "ccl/base/security/cryptomaterial.h"
#include "ccl/base/storage/attributes.h"
#include "ccl/base/storage/jsonarchive.h"

#include "ccl/network/web/webrequest.h"

#include "ccl/public/base/iprogress.h"
#include "ccl/public/text/istringdict.h"
#include "ccl/public/network/web/iwebrequest.h"
#include "ccl/public/systemservices.h"

#include <emscripten/threading.h>
#include <emscripten/proxying.h>

#define CCLNET_USE_LOCAL_PROXY_SERVER (0 && DEBUG)

namespace CCL {
namespace Web {

//////////////////////////////////////////////////////////////////////////////////////////////////
// JS Helpers
//////////////////////////////////////////////////////////////////////////////////////////////////

EM_JS_DEPS (httpClientDependencies, "$dynCall");

/**
  Start a request.
  Only ever called while already running on the main thread, since fetch() and its response stream are main-thread JS APIs.
  Headers/progress/completion are delivered back via the deliverHeaders/deliverProgress/deliverComplete function pointers.
  The calling thread waits for them inside emscripten_thread_sleep, which pumps Emscripten's own proxying queue.
*/
EM_JS (void, startFetch, (void* request, CStringPtr _url, CStringPtr _method, CStringPtr _headersJson,
			const uint8_t* bodyData, int bodyLength, void* deliverHeaders, void* deliverProgress, void* deliverComplete),
{
	url = UTF8ToString (_url);
	method = UTF8ToString (_method);
	headersJson = _headersJson ? UTF8ToString (_headersJson) : null;

	if(!CCL.fetchTransactions)
		CCL.fetchTransactions = new Map ();
	const controller = new AbortController ();
	CCL.fetchTransactions.set (request, controller);

	(async () =>
	{
		try
		{
			const options = { method, signal: controller.signal };

			if(headersJson)
			{
				try
				{
					options.headers = JSON.parse (headersJson);
				}
				catch(e)
				{
					console.error ('[HTTP-PROXY] Header parse error:', e);
				}
			}

			if(bodyLength > 0)
				options.body = HEAPU8.slice (bodyData, bodyData + bodyLength);

			const response = await fetch (url, options);

			const contentLength = +response.headers.get ('Content-Length') || 0;

			const responseHeaders = {};
			response.headers.forEach ((value, key) =>
			{
				responseHeaders[key] = value;
			});

			const headersStr = Object.entries (responseHeaders)
				.map (([k, v]) => `${k}: ${v}`)
				.join ('\r\n');
			const headersPtr = stringToNewUTF8 (headersStr);

			// Deliver headers as soon as they're known, before reading any of the body
			dynCall ('viiii', deliverHeaders, [request, response.status, contentLength, headersPtr]);

			// Deliver each chunk to deliverProgress as it arrives, rather than buffering the
			// full response, so peak memory stays around chunk size instead of response size.
			const reader = response.body.getReader ();
			reader.closed.catch (() => {});
			let receivedLength = 0;

			while(true)
			{
				const { done, value } = await reader.read ();
				if(done)
					break;

				receivedLength += value.length;

				const chunkPtr = _malloc (value.length);
				HEAPU8.set (value, chunkPtr);

				dynCall ('viiiii', deliverProgress, [request, receivedLength, contentLength || receivedLength, chunkPtr, value.length]);
			}

			dynCall ('vii', deliverComplete, [request, response.status]);
		}
		catch(error)
		{
			if(error.name === 'AbortError')
				console.log ('[HTTP-PROXY] Fetch canceled');
			else
				console.error ('[HTTP-PROXY] Fetch failed:', error);
			dynCall ('vii', deliverComplete, [request, 0]);
		}
		finally
		{
			CCL.fetchTransactions.delete (request);
		}
	}) ();
});

//////////////////////////////////////////////////////////////////////////////////////////////////

EM_JS (void, cancelFetch, (void* request),
{
	const controller = CCL.fetchTransactions?.get (request);
	if(controller)
		controller.abort ();
});

//************************************************************************************************
// FetchContext
//************************************************************************************************

class WasmHTTPClient::FetchContext: public Object
{
public:
	FetchContext (IStream* downloadStream, IWebHeaderCollection* headers, IProgressNotify* progress)
	: downloadStream (downloadStream),
	  headers (headers),
	  progress (progress),
	  status (0),
	  succeeded (false),
	  contentLength (0)
	{
		responseHeaders = NEW WebHeaderCollection;
	}

	PROPERTY_SHARED_POINTER (IStream, downloadStream, DownloadStream)
	PROPERTY_SHARED_POINTER (IWebHeaderCollection, headers, Headers)
	PROPERTY_SHARED_POINTER (IProgressNotify, progress, Progress)
	PROPERTY_VARIABLE (int, status, Status)
	PROPERTY_BOOL (succeeded, Success)
	PROPERTY_VARIABLE (int64, contentLength, ContentLength)
	IWebHeaderCollection& getResponseHeaders () const { return *responseHeaders; }

private:
	AutoPtr<WebHeaderCollection> responseHeaders;
};

//************************************************************************************************
// HttpRequest
//************************************************************************************************

struct WasmHTTPClient::HttpRequest
{
	typedef void(*HeadersCallback) (void*, int, int, CStringPtr);
	typedef void(*ProgressCallback) (void*, int64_t, int64_t, const uint8_t*, int);
	typedef void(*CompleteCallback) (void*, int);

	MutableCString url;
	MutableCString method;
	MutableCString headersJson;
	const uint8_t* bodyData = nullptr;
	int bodyLength = 0;
	void* userData = nullptr;
	pthread_t originThread = 0;
	bool isComplete = false;
	bool isAbandoned = false;
	HeadersCallback onHeaders = nullptr;
	ProgressCallback onProgress = nullptr;
	CompleteCallback onComplete = nullptr;

	HttpRequest ()
	: bodyData (nullptr),
	  bodyLength (0),
	  userData (nullptr),
	  originThread (pthread_self ()),
	  isComplete (false),
	  isAbandoned (false),
	  onHeaders (nullptr),
	  onProgress (nullptr),
	  onComplete (nullptr)
	{}
};

//************************************************************************************************
// FetchEvent
//************************************************************************************************

struct WasmHTTPClient::FetchEvent
{
	enum EventType 
	{
		kHeaders, 
		kProgress, 
		kComplete 
	};

	HttpRequest* request = nullptr;
	int type = kProgress;
	int loaded = 0;
	int total = 0;
	uint8_t* data = nullptr;
	int dataLength = 0;
	int status = 0;
	char* headers = nullptr;
};

} // namespace Web
} // namespace CCL

using namespace CCL;
using namespace Web;

//************************************************************************************************
// WasmHTTPClient
//************************************************************************************************

DEFINE_CLASS (WasmHTTPClient, WebClient)

//////////////////////////////////////////////////////////////////////////////////////////////////

WasmHTTPClient::WasmHTTPClient (bool useSSL)
: useSSL (useSSL),
  currentContext (nullptr)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

WasmHTTPClient::~WasmHTTPClient ()
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult WasmHTTPClient::performRequest (StringID method, StringRef remotePath, IStream* uploadStream,
										IStream& downloadStream, IWebHeaderCollection* headers, IProgressNotify* progress)
{
	if(!isConnected ())
		return kResultFailed;

	ASSERT (currentContext == nullptr)
	if(currentContext)
		return kResultFailed;
	currentContext = NEW FetchContext (&downloadStream, headers, progress);

	HttpRequest* request = NEW HttpRequest;

	request->method = method ? method : "GET";

	request->url = useSSL ? "https" : "http";
	request->url += "://";
	request->url += MutableCString (hostname, Text::kUTF8);
	request->url += MutableCString (remotePath, Text::kUTF8);

	#if CCLNET_USE_LOCAL_PROXY_SERVER
	bool isLocalhost = hostname == "localhost" || hostname == "127.0.0.1" || hostname.startsWith ("localhost:");
	if(!isLocalhost)
	{
		MutableCString proxiedUrl = "http://localhost:8080/proxy/";
		proxiedUrl += request->url;
		request->url = proxiedUrl;
		CCL_PRINTF ("Routing through proxy: %s\n", request->url.str ())
	}
	#endif // CCLNET_USE_LOCAL_PROXY_SERVER

	CCL_PRINTF ("[HTTP] === Starting HTTP proxy request: %s %s ===\n",
		request->method.str (), request->url.str ())

	prepareHeaders (request->headersJson, headers);
	CCL_PRINTF ("[HTTP] Headers JSON: %s\n", request->headersJson.str ())

	AutoPtr<MemoryStream> uploadBuffer;
	if(uploadStream)
	{
		UnknownPtr<IMemoryStream> memoryStream (uploadStream);
		if(memoryStream)
		{
			request->bodyData = static_cast<const uint8_t*> (memoryStream->getMemoryAddress ());
			request->bodyLength = int(memoryStream->getBytesWritten ());
		}
		else
		{
			uploadBuffer = NEW MemoryStream;
			char buffer[4096] = {};
			while(true)
			{
				int64 bytesRead = uploadStream->read (buffer, int(sizeof(buffer)));
				if(bytesRead <= 0)
					break;
				uploadBuffer->write (buffer, int(bytesRead));
			}

			if(uploadBuffer->getBytesWritten () > 0)
			{
				request->bodyData = static_cast<const uint8_t*> (uploadBuffer->getMemoryAddress ());
				request->bodyLength = int(uploadBuffer->getBytesWritten ());
			}
		}

		if(request->bodyData && request->bodyLength > 0)
		{
			CCL_PRINTF ("Request body: %d bytes\n", request->bodyLength)
		}
	}

	bool isMainThread = System::IsInMainThread ();
	Threading::ThreadID threadId = System::GetThreadSelfID ();

	CCL_PRINTF ("[HTTP] Thread info: ID=%lu, isMainThread=%d, hasProgress=%d\n",
		(unsigned long)threadId, isMainThread ? 1 : 0, progress ? 1 : 0);


	request->userData = this;
	request->onHeaders = onFetchHeaders;
	request->onProgress = onFetchProgress;
	request->onComplete = onFetchComplete;

	// Proxy the fetch to the main thread; completion/progress are delivered back via
	// emscripten_proxy_async (see proxyResult/proxyProgress above), so they reach us
	// while blocked in emscripten_thread_sleep below.
	emscripten_proxy_async (emscripten_proxy_get_system_queue (), emscripten_main_runtime_thread_id (), dispatchFetch, request);

	CCL_PRINTF ("[HTTP] Proxy request started, waiting for completion...\n", 0)

	// Wait for main thread to complete the fetch.
	int iterations = 0;
	while(!request->isComplete)
	{
		// Pumps Emscripten's proxying queue, so that callbacks reach this thread.
		emscripten_thread_sleep (50);

		if(progress && progress->isCanceled ())
		{
			CCL_PRINTF ("[HTTP] Progress canceled; abandoning wait and requesting cancellation on main thread\n", 0)
			emscripten_proxy_async (emscripten_proxy_get_system_queue (), emscripten_main_runtime_thread_id (), dispatchCancel, request);
			request->onHeaders = nullptr;
			request->onProgress = nullptr;
			request->onComplete = nullptr;
			request->isAbandoned = true;
			safe_release (currentContext);
			return kResultFailed;
		}

		iterations++;
		if(iterations % 200 == 0)  // Log every 10 seconds
		{
			CCL_PRINTF ("[HTTP] Still waiting... (%d iterations)\n", iterations)
		}
	}

	CCL_PRINTF ("[HTTP] Fetch completed after %d iterations\n", iterations)

	delete request;

	tresult result = currentContext->isSuccess () ? kResultOk : kResultFailed;
	lastStatus = currentContext->getStatus ();

	CCL_PRINTF ("[HTTP] === Request finished: %s, status=%d ===\n",
		currentContext->isSuccess () ? "SUCCESS" : "FAILED", lastStatus)

	safe_release (currentContext);

	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WasmHTTPClient::dispatchFetch (void* arg)
{
	// Runs on the main thread, reached via emscripten_proxy_async from the origin thread.
	HttpRequest* request = static_cast<HttpRequest*> (arg);
	if(request->isAbandoned)
	{
		delete request;
		return;
	}
	startFetch (request, request->url.str (), request->method.str (),
		request->headersJson.isEmpty () ? nullptr : request->headersJson.str (),
		request->bodyData, request->bodyLength,
		(void*)proxyHeaders, (void*)proxyProgress, (void*)proxyResult);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WasmHTTPClient::dispatchCancel (void* arg)
{
	// Runs on the main thread, reached via emscripten_proxy_async from the origin thread.
	HttpRequest* request = static_cast<HttpRequest*> (arg);
	cancelFetch (request);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API WasmHTTPClient::downloadData (StringRef remotePath, IStream& localStream,
											  IWebHeaderCollection* headers, IProgressNotify* progress)
{
	return performRequest ("GET", remotePath, nullptr, localStream, headers, progress);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API WasmHTTPClient::uploadData (IWebHeaderCollection* headers, IStream& localStream,
											StringRef remotePath, IStream& responseStream,
											StringID method, IProgressNotify* progress)
{
	CString methodStr = method ? method : "POST";
	return performRequest (methodStr, remotePath, &localStream, responseStream, headers, progress);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WasmHTTPClient::onFetchHeaders (void* userData, int status, int contentLength, CStringPtr headersStr)
{
	CCL_PRINTF ("[JS-CALLBACK] Headers: status=%d, contentLength=%d\n", status, contentLength)

	WasmHTTPClient* This = static_cast<WasmHTTPClient*> (userData);
	if(!This->currentContext)
		return;

	This->currentContext->setStatus (status);
	This->currentContext->setContentLength (contentLength);

	IWebHeaderCollection& responseHeaders = This->currentContext->getResponseHeaders ();
	responseHeaders.getEntries ().removeAll ();

	IWebHeaderCollection* callerHeaders = This->currentContext->getHeaders ();
	if(callerHeaders && callerHeaders != &responseHeaders)
		callerHeaders->getEntries ().removeAll ();

	if(headersStr)
	{
		CCL_PRINTF ("[JS-CALLBACK] Parsing response headers\n", 0)

		CString headers (headersStr);
		MutableCString remaining = headers;

		while(remaining.length () > 0)
		{
			int lineEnd = remaining.index ('\n');
			MutableCString line;

			if(lineEnd < 0)
			{
				// Last line
				line = remaining;
				remaining.empty ();
			}
			else
			{
				line = remaining.subString (0, lineEnd);
				remaining = remaining.subString (lineEnd + 1);
			}

			if(line.length () > 0 && line[line.length () - 1] == '\r')
				line = line.truncate (line.length () - 1);

			int colonPosition = line.index (':');
			if(colonPosition > 0)
			{
				CCL_PRINTF ("[JS-CALLBACK] Received header: %s\n", line.str ())

				CString key = line.subString (0, colonPosition);
				CString value = line.subString (colonPosition + 1);
				// Trim leading space from value
				while(value.length() > 0 && value[0] == ' ')
					value = value.subString (1);

				responseHeaders.getEntries ().setEntry (key, value);
				if(callerHeaders && callerHeaders != &responseHeaders)
					callerHeaders->getEntries ().setEntry (key, value);
			}

			if(remaining.isEmpty ())
				break;
		}
	}

	// Notify as soon as headers are known, before any body data
	WebTransaction ().receiveMetaData (This->currentContext->getDownloadStream (), This->currentContext->getProgress (),
		contentLength, responseHeaders);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WasmHTTPClient::onFetchProgress (void* userData, int64_t loaded, int64_t total, const uint8_t* data, int dataLength)
{
	CCL_PRINTF ("[JS-CALLBACK] Progress: loaded=%lld, total=%lld, chunk=%d\n", (long long)loaded, (long long)total, dataLength)

	WasmHTTPClient* This = static_cast<WasmHTTPClient*> (userData);
	if(!This->currentContext)
		return;

	if(data && dataLength > 0)
	{
		int64 written = This->currentContext->getDownloadStream ()->write (data, dataLength);

		if(written != dataLength)
			CCL_PRINTF ("[JS-CALLBACK] Chunk write failed: wrote %lld of %d bytes\n", (long long)written, dataLength)

		// Ongoing notification while data is coming in
		WebTransaction ().receiveMetaData (This->currentContext->getDownloadStream (), This->currentContext->getProgress (),
			total, This->currentContext->getResponseHeaders ());
	}

	IProgressNotify* progress = This->currentContext->getProgress ();
	if(!progress)
		return;

	if(total > 0)
	{
		double progressValue = double(loaded) / double(total);
		IProgressNotify::State state(progressValue);
		progress->updateProgress (state);
	}
	else
	{
		progress->updateAnimated ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WasmHTTPClient::onFetchComplete (void* userData, int status)
{
	CCL_PRINTF ("[JS-CALLBACK] Complete: status=%d\n", status)

	WasmHTTPClient* This = static_cast<WasmHTTPClient*> (userData);
	if(!This->currentContext)
		return;

	This->currentContext->setStatus (status);

	// Data was already written to the stream as it was streamed in via onFetchProgress.
	bool success = (status >= 200 && status < 300);
	if(!success)
		CCL_PRINTF ("[JS-CALLBACK] HTTP error status: %d\n", status)

	This->currentContext->setSuccess (success);

	// Reliably notify of completion, even on failure
	WebTransaction ().receiveMetaData (This->currentContext->getDownloadStream (), This->currentContext->getProgress (),
		This->currentContext->getContentLength (), This->currentContext->getResponseHeaders (), WebTransaction::kEnd);

	// Update progress to 100%
	if(This->currentContext->getProgress ())
		This->currentContext->getProgress ()->updateProgress (1.0);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WasmHTTPClient::onFetchEvent (void* arg)
{
	// Runs on the origin thread, reached via emscripten_thread_sleep's internal pumping of
	// Emscripten's proxying queue.

	FetchEvent* event = static_cast<FetchEvent*> (arg);
	HttpRequest* request = event->request;

	switch(event->type)
	{
	case FetchEvent::kHeaders :
		if(request->onHeaders)
			request->onHeaders (request->userData, event->status, event->total, event->headers);
		break;

	case FetchEvent::kProgress :
		if(request->onProgress)
			request->onProgress (request->userData, event->loaded, event->total, event->data, event->dataLength);
		break;

	case FetchEvent::kComplete :
		if(request->onComplete)
			request->onComplete (request->userData, event->status);
		request->isComplete = true;
		break;
	}

	if(event->headers)
		::free (event->headers);
	if(event->data)
		::free (event->data);

	delete event;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WasmHTTPClient::proxyHeaders (HttpRequest* request, int status, int contentLength, char* headers)
{
	// Called via dynCall from startFetch on main thread; hop back to the origin thread.
	if(request->isAbandoned)
	{
		if(headers)
			::free (headers);
		return;
	}
	FetchEvent* event = NEW FetchEvent { request, FetchEvent::kHeaders, 0, contentLength, nullptr, 0, status, headers };
	emscripten_proxy_async (emscripten_proxy_get_system_queue (), request->originThread, onFetchEvent, event);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WasmHTTPClient::proxyProgress (HttpRequest* request, int loaded, int total, uint8_t* data, int dataLength)
{
	// Called via dynCall from startFetch on main thread; hop back to the origin thread.
	if(request->isAbandoned)
	{
		if(data)
			::free (data);
		return;
	}
	FetchEvent* event = NEW FetchEvent { request, FetchEvent::kProgress, loaded, total, data, dataLength, 0, nullptr };
	emscripten_proxy_async (emscripten_proxy_get_system_queue (), request->originThread, onFetchEvent, event);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WasmHTTPClient::proxyResult (HttpRequest* request, int status)
{
	// Called via dynCall from startFetch on main thread; hop back to the origin thread.
	if(request->isAbandoned)
	{
		delete request;
		return;
	}
	FetchEvent* event = NEW FetchEvent { request, FetchEvent::kComplete, 0, 0, nullptr, 0, status, nullptr };
	emscripten_proxy_async (emscripten_proxy_get_system_queue (), request->originThread, onFetchEvent, event);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WasmHTTPClient::prepareHeaders (MutableCString& result, IWebHeaderCollection* headers) const
{
	Attributes headerAttributes;

	// Add authentication headers
	if(credentials)
	{
		MutableCString authType (credentials->getAuthType ());
		if(authType.isEmpty () || authType == Meta::kBasic)
		{
			MutableCString string;
			string.append (credentials->getUserName ());
			string.append (":");
			string.append (credentials->getPassword ());

			Security::Crypto::Material material (Security::Crypto::Block (string, string.length ()));

			MutableCString basicAuthentication ("Basic ");
			basicAuthentication.append (material.toCBase64 ());

			headerAttributes.set ("Authorization", basicAuthentication, Text::kUTF8);
			CCL_PRINTF ("Added Basic authentication header\n", 0)
		}
		else if(authType == Meta::kBearer)
		{
			MutableCString bearerAuthentication ("Bearer ");
			bearerAuthentication.append (credentials->getPassword ());

			headerAttributes.set ("Authorization", bearerAuthentication, Text::kUTF8);
			CCL_PRINTF ("Added Bearer authentication header\n", 0)
		}
		else if(authType == Meta::kOAuth)
		{
			MutableCString oauthAuthentication ("OAuth ");
			oauthAuthentication.append (credentials->getPassword ());

			headerAttributes.set ("Authorization", oauthAuthentication, Text::kUTF8);
			CCL_PRINTF ("Added OAuth authentication header\n", 0)
		}
		else
		{
			ASSERT (false)
		}
	}

	// Add custom headers
	if(headers)
	{
		ICStringDictionary& entries = headers->getEntries ();
		int count = entries.countEntries ();
		CCL_PRINTF ("Adding %d custom headers\n", count)
		for(int i = 0; i < count; i++)
		{
			CStringPtr key = entries.getKeyAt (i);
			CStringPtr value = entries.getValueAt (i);
			CCL_PRINTF ("  Header: %s: %s\n", key, value)
			headerAttributes.set (key, value, Text::kUTF8);
		}
	}

	result.empty ();
	AutoPtr<IStream> jsonStream = JsonUtils::serialize (headerAttributes);
	UnknownPtr<IMemoryStream> memoryStream (jsonStream);
	if(memoryStream)
		result.append (static_cast<CStringPtr> (memoryStream->getMemoryAddress ()), int(memoryStream->getBytesWritten ()));
}
