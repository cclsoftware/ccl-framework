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
// Filename    : ccl/network/web/http/client.cpp
// Description : HTTP Client
//
//************************************************************************************************

#define DEBUG_LOG 0
#define DEBUG_HTTP_TRANSACTION (0 && DEBUG) // debug output during HTTP transaction

#include "ccl/network/web/http/client.h"

#include "ccl/base/message.h"
#include "ccl/base/storage/url.h"
#include "ccl/base/storage/urlencoder.h"
#include "ccl/base/security/cryptomaterial.h"

#include "ccl/public/system/cclerror.h"
#include "ccl/public/base/buffer.h"
#include "ccl/public/base/istream.h"
#include "ccl/public/base/iprogress.h"
#include "ccl/public/system/ifileutilities.h"
#include "ccl/public/system/threadlocal.h"
#include "ccl/public/network/inetwork.h"
#include "ccl/public/network/web/iwebservice.h"
#include "ccl/public/systemservices.h"
#include "ccl/public/netservices.h"
#include "ccl/public/cclversion.h"

namespace CCL {
namespace Web {
namespace HTTP {

//************************************************************************************************
// HTTP::ProgressOffsetter
//************************************************************************************************

class ProgressOffsetter: public Object,
						 public IProgressNotify
{
public:
	ProgressOffsetter (IProgressNotify* _progress = nullptr, int64 _length = 0, int64 _offset = 0)
	: progress (_progress),
	  length (_length),
	  offset (_offset)
	{
		ASSERT (progress)
	}
	
	// IProgressNotify
	void CCL_API setTitle (StringRef title) override { progress->setTitle (title); }
	void CCL_API setCancelEnabled (tbool state) override { progress->setCancelEnabled (state); }
	void CCL_API endProgress () override { progress->endProgress (); }
	IProgressNotify* CCL_API createSubProgress () override { return progress->createSubProgress (); }
	void CCL_API setProgressText (StringRef text) override { progress->setProgressText (text); }
	tbool CCL_API isCanceled () override { return progress->isCanceled (); }
	void CCL_API beginProgress () override { progress->beginProgress (); }
	void CCL_API updateProgress (const State& state) override
	{
		State newState (state);
		newState.value = (offset + state.value * (length - offset)) / length;
		progress->updateProgress (newState);
	}
	
	CLASS_INTERFACE (IProgressNotify, Object)

protected:
	IProgressNotify* progress;
	int64 length;
	int64 offset;
};

} // namespace HTTP
} // namespace Web
} // namespace CCL

using namespace CCL;
using namespace Web;
using namespace HTTP;

//************************************************************************************************
// HTTP::ConnectionManager
//************************************************************************************************

DEFINE_SINGLETON (ConnectionManager)
static const CString kEnableConnectionCheck ("EnableConnectionCheck");

//////////////////////////////////////////////////////////////////////////////////////////////////

ConnectionManager::ConnectionManager ()
: lastExecutionTime (0),
  checkEnabled (false)
{
	connections.objectCleanup (true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ConnectionManager::~ConnectionManager ()
{
	cancelSignals ();
	ASSERT (connections.isEmpty ())
	ASSERT (checkEnabled == false)
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ConnectionManager::terminate ()
{
	CCL_PRINTLN ("-- Terminating persistent connections")
	Threading::ScopedLock scopedLock (lock);
	connections.removeAll ();
	enableCheck (false);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Connection* ConnectionManager::use (StringRef hostname, bool useSSL)
{
	int connectionCount = 0;
	{
		Threading::ScopedLock scopedLock (lock);
		for(auto c : iterate_as<Connection> (connections))
			if(c->getHostname () == hostname && c->isUseSSL () == useSSL)
			{
				if(!c->isInUse ())
				{
					CCL_PRINTF (":) Reusing persistent connection to %s\n", MutableCString (c->getHostname ()).str ())
					c->setInUse (true);
					return c;
				}
				else
					connectionCount++;
			}
	}

	auto c = Connection::resolve (hostname, useSSL);
	if(c)
	{
		if(connectionCount < kMaxConnectionsPerHost)
		{
			CCL_PRINTF ("** Created persistent connection to %s\n", MutableCString (c->getHostname ()).str ())
			Threading::ScopedLock scopedLock (lock);
			c->setPersistent (true);
			c->setInUse (true);
			connections.add (c);
		}
		#if DEBUG_LOG
		else
			CCL_PRINTF ("!! Persistent connection limit reached for %s\n", MutableCString (c->getHostname ()).str ())
		#endif
	}
	return c;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ConnectionManager::unuse (Connection* c)
{
	if(c->isPersistent ())
	{
		Threading::ScopedLock scopedLock (lock);
		if(!c->isOpen ()) // connection was closed by server
		{
			connections.remove (c);
			c->release ();
		}
		else
		{
			CCL_PRINTF ("-- Keeping persistent connection to %s\n", MutableCString (c->getHostname ()).str ())
			c->setInUse (false);
			c->setTimeLastUsed (System::GetSystemTicks ());
			enableCheckDeferred (); // deferred, avoid potential deadlock with thread pool
		}
	}
	else
		c->release ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ConnectionManager::enableCheckDeferred ()
{
	(NEW Message (kEnableConnectionCheck))->post (this, -1);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API ConnectionManager::notify (ISubject* subject, MessageRef msg)
{
	if(msg == kEnableConnectionCheck)
	{
		Threading::ScopedLock scopedLock (lock);
		enableCheck (true);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ConnectionManager::checkConnections ()
{
	int64 now = System::GetSystemTicks ();

	Threading::ScopedLock scopedLock (lock);
	ObjectArray toRemove;
	for(auto c : iterate_as<Connection> (connections))
		if(!c->isInUse ())
		{
			if(now - c->getTimeLastUsed () >= kConnectionIdleTimeout)
				toRemove.add (c);
		}

	if(!toRemove.isEmpty ())
		for(auto c : iterate_as<Connection> (toRemove))
		{
			CCL_PRINTF ("XX Removing persistent connection to %s (timeout)\n", MutableCString (c->getHostname ()).str ())
			connections.remove (c);
			c->release ();
		}

	if(connections.isEmpty ())
		enableCheck (false);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ConnectionManager::enableCheck (bool state)
{
	// we assume lock is held already
	if(state != checkEnabled)
	{
		CCL_PRINTF ("-- Connection checks enabled: %s\n", state ? "true" : "false")
		if(checkEnabled)
			System::GetThreadPool ().removePeriodic (this);
		checkEnabled = state;
		if(checkEnabled)
			System::GetThreadPool ().addPeriodic (this);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int64 CCL_API ConnectionManager::getExecutionTime () const
{
	return lastExecutionTime + kConnectionCheckInterval;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API ConnectionManager::execute (int64 now)
{
	lastExecutionTime = now;
	checkConnections ();
}

//************************************************************************************************
// HTTP::Connection
//************************************************************************************************

Connection* Connection::resolve (StringRef _hostname, bool useSSL)
{
	Net::PortNumber port = useSSL ? 443 : 80;
	String hostname (_hostname);

	// get port from hostname (if present)
	static const String kPortSeparator (":");
	int portIndex = hostname.lastIndex (kPortSeparator);
	if(portIndex != -1)
	{
		int64 value = 0;
		if(hostname.subString (portIndex+1).getIntValue (value))
			port = (Net::PortNumber)value;

		hostname.truncate (portIndex);
	}

	// lookup IP address
	Net::IPAddress address;
	tresult result = System::GetNetwork ().getAddressByHost (address, hostname);
	if(result != kResultOk)
		return nullptr;

	address.port = port;

	return NEW Connection (hostname, address, useSSL);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_CLASS_HIDDEN (Connection, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

Connection::Connection (StringRef hostname, const Net::IPAddress& address, bool useSSL)
: hostname (hostname),
  address (address),
  useSSL (useSSL),
  persistent (false),
  inUse (false),
  timeLastUsed (0),
  stream (nullptr)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

Connection::~Connection ()
{
	close ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IStream* Connection::open (IProgressNotify* progress)
{
	if(stream == nullptr)
	{
		if(useSSL == true)
			stream = System::GetNetwork ().openSSLStream (address, hostname, progress);
		else
			stream = System::GetNetwork ().openStream (address, Net::kTCP);

		if(UnknownPtr<Net::INetworkStream> netStream = stream)
			netStream->setPseudoBlocking (true);
	}
	return stream;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Connection::close ()
{
	safe_release (stream);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Connection::isOpen () const
{
	return stream != nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IStream* Connection::detach ()
{
	IStream* result = stream;
	stream = nullptr;
	return result;
}

//************************************************************************************************
// HTTP::Content
//************************************************************************************************

#if DEBUG
void Content::dump () const
{
	Buffer buffer ((unsigned int)length);
	int numRead = stream.read (buffer, (int)length);
	stream.rewind ();
	char* address = (char*)buffer.getAddress ();

	String string;
	string.appendASCII (address, numRead);
	CCL_PRINTLN (string)
}
#endif

//************************************************************************************************
// HTTP::Client::RedirectCounter
//************************************************************************************************

class HTTP::Client::RedirectCounter: public Threading::ThreadSingleton<RedirectCounter>
{
public:
	RedirectCounter ()
	: value (0)
	{}

	PROPERTY_VARIABLE (int, value, Value)
	void increment () { value++; }
	void decrement () { value--; }

	struct Scope
	{
		RedirectCounter& counter;
		Scope (RedirectCounter& counter):counter (counter) { counter.increment (); }
		~Scope () { counter.decrement (); }
	};
};

DEFINE_THREAD_SINGLETON (HTTP::Client::RedirectCounter)

//************************************************************************************************
// HTTP::Client
//************************************************************************************************

MutableCString Client::userAgentName;
const CString Client::defaultUserAgentName ("WebClient/" CCL_VERSION_STRING " " CCL_PLATFORM_STRING);

//////////////////////////////////////////////////////////////////////////////////////////////////

void HTTP::Client::setUserAgent (StringID userAgent)
{
	userAgentName = userAgent;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringID HTTP::Client::getUserAgent (bool useDefault)
{
	if(!userAgentName.isEmpty ())
		return userAgentName;
	if(useDefault)
		return defaultUserAgentName;
	return CString::kEmpty;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_CLASS_HIDDEN (Client, WebClient)

//////////////////////////////////////////////////////////////////////////////////////////////////

Client::Client (bool useSSL)
: useSSL (useSSL),
  connection (nullptr),
  autoRedirectEnabled (true)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

Client::~Client ()
{
	disconnect ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API Client::connect (StringRef hostname)
{
	disconnect ();

	connection = ConnectionManager::instance ().use (hostname, useSSL);
	if(!connection)
		return kResultFailed;

	return SuperClass::connect (hostname);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API Client::disconnect ()
{
	if(connection)
	{
		ConnectionManager::instance ().unuse (connection);
		connection = nullptr;
	}

	return SuperClass::disconnect ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API Client::downloadData (StringRef remotePath, IStream& localStream, IWebHeaderCollection* headers, IProgressNotify* progress)
{
	ASSERT (connection != nullptr)
	if(!connection)
		return kResultUnexpected;

	Content outContent (localStream);
	MutableCString encodedPath (UrlUtils::toEncodedPath (remotePath));

	Transaction t (*connection, HTTP::kGET, encodedPath, outContent);
	if(headers != nullptr) // inject additional headers
		t.setHeaders (*headers);
	prepare (t);
	t.setProgress (progress);
	t.setProgressMode (Transaction::kReceived);

	lastStatus = 0;
	bool result = t.perform (lastStatus);

	if(HTTP::isRedirectStatus (lastStatus) && isAutoRedirectEnabled ())
	{
		RedirectCounter& counter = RedirectCounter::instance ();
		if(counter.getValue () < kMaxRedirectCount)
		{
			RedirectCounter::Scope scope (counter);
			String locationString;
			locationString.appendCString (Text::kUTF8, t.getResponseHeaders ().getLocation ());

			// Make sure URL-encoding/decoding is symmetric. This is critical for signed URLs!
			Url location;
			UrlUtils::fromEncodedString (location, locationString);
			ASSERT (!location.isEmpty ())

			lastStatus = 0;
			return System::GetWebService ().downloadData (location, localStream, credentials, headers, progress, &lastStatus);
		}
		else
		{
			CCL_WARN ("Maximum number of HTTP redirects exceeded!\n", 0)
			result = false;
		}
	}

	return result ? kResultOk : kResultFailed;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API Client::uploadData (IWebHeaderCollection* headers, IStream& localStream, StringRef remotePath,
									IStream& responseStream, StringID _method, IProgressNotify* progress)
{
	ASSERT (connection != nullptr)
	if(!connection)
		return kResultUnexpected;

	// TODO: use chunked upload instead???
	ASSERT (localStream.isSeekable ())
	if(!localStream.isSeekable ())
		return kResultInvalidArgument;

	int64 length = localStream.seek (0, IStream::kSeekEnd); // content for upload is optional, length can be null
	localStream.rewind ();

	MutableCString contentType;
	if(headers != nullptr)
		contentType = headers->getEntries ().lookupValue (Meta::kContentType);

	Content inContent (localStream, length, contentType);
	Content outContent (responseStream);

	CString method (_method);
	if(method.isEmpty ())
		method = HTTP::kPOST;

	MutableCString encodedPath (UrlUtils::toEncodedPath (remotePath));

	Transaction t (*connection, method, encodedPath, outContent, &inContent);
	if(headers != nullptr) // inject additional headers
		t.setHeaders (*headers);
	prepare (t);
	t.setProgress (progress);
	t.setProgressMode (Transaction::kSend);

	lastStatus = 0;
	bool result = t.perform (lastStatus);

	return result ? kResultOk : kResultFailed;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Client::prepare (Transaction& t)
{
	t.setAutoRedirectEnabled (isAutoRedirectEnabled ());
	t.setUserAgent (getUserAgent ());
	t.setCredentials (credentials);
}

//************************************************************************************************
// HTTP::Transaction
//************************************************************************************************

const int Transaction::kVersion = HTTP::kV1_1;

//////////////////////////////////////////////////////////////////////////////////////////////////

Transaction::Transaction (Connection& connection,
						  StringID method,
						  StringID path,
						  Content& outContent,
						  const Content* inContent)
: connection (connection),
  outContent (outContent),
  inContent (inContent),
  stream (nullptr),
  outerProgress (nullptr),
  progressMode (kNone),
  autoRedirectEnabled (false)
{
	request.setVersion (kVersion);
	request.setMethod (method);
	request.setPath (path);
	request.getHeaders ().setHost (MutableCString (connection.getHostname ()));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Transaction::setHeaders (IWebHeaderCollection& headers)
{
	auto& entries = headers.getEntries ();
	for(int i = 0; i < entries.countEntries (); i++)
		setHeader (entries.getKeyAt (i), entries.getValueAt (i));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Transaction::setUserAgent (StringID userAgent)
{
	request.getHeaders ().setUserAgent (userAgent);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Transaction::setHeader (StringID key, StringID value)
{
	request.getHeaders ().setEntry (key, value);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Transaction::setCredentials (IWebCredentials* credentials)
{
	if(credentials)
	{
		MutableCString authType (credentials->getAuthType ());
		if(authType.isEmpty () || authType == Meta::kBasic)
		{
			MutableCString string;
			string.append (credentials->getUserName ());
			string.append (":");
			string.append (credentials->getPassword ());

			Security::Crypto::Material material (Security::Crypto::Block (string.str (), string.length ()));

			MutableCString basicAuthentication ("Basic ");
			basicAuthentication.append (material.toCBase64 ());

			request.getHeaders ().setAuthorization (basicAuthentication);
		}
		else if(authType == Meta::kBearer)
		{
			MutableCString bearerAuthentication ("Bearer ");
			bearerAuthentication.append (credentials->getPassword ());

			request.getHeaders ().setAuthorization (bearerAuthentication);
		}
		else if(authType == Meta::kOAuth)
		{
			MutableCString oauthAuthentication ("OAuth ");
			oauthAuthentication.append (credentials->getPassword ());

			request.getHeaders ().setAuthorization (oauthAuthentication);
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

HeaderList& Transaction::getResponseHeaders ()
{
	return request.getResponse ().getHeaders ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Transaction::perform (int& httpStatus)
{
	static const int kNumRetries = 2;

	bool result = false;
	for(int i = 0; i < kNumRetries; i++)
	{
		ErrorContextGuard guard;

		result = begin ();
		if(result)
			result &= sendRequest ();
		if(result)
			result &= receiveResponse (httpStatus);
		finish (!result);

		if(result)
			break;

		tresult resultCode = guard.getResultCode ();
		if(resultCode != Net::kResultConnectionReset && resultCode != Net::kResultConnectionAborted)
			break;
		// Otherwise, retry
	}
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Transaction::begin ()
{
	ASSERT (stream == nullptr)
	stream = connection.open (outerProgress);
	request.setStream (stream);

	if(UnknownPtr<Net::INetworkStream> netStream = stream)
		netStream->setCancelCallback (outerProgress);

	return stream != nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Transaction::sendRequest ()
{
	if(inContent)
	{
		request.getHeaders ().setContentType (inContent->getType ());
		request.getHeaders ().setContentLength (inContent->getLength ());
	}

	#if DEBUG_HTTP_TRANSACTION
	request.dump ();
	#endif

	if(!request.send ())
		return false;

	if(inContent)
	{
		#if DEBUG_HTTP_TRANSACTION
		//inContent->dump ();
		#endif

		IProgressNotify* progress = progressMode == kSend ? outerProgress : nullptr; // send progress
		if(!sendData (inContent->getStream (), inContent->getLength (), progress))
			return false;
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Transaction::receiveResponse (int& httpStatus)
{
	Response& response = request.getResponse ();
	int status = 0;
	do
	{
		if(!response.receive ())
			return false;

		#if DEBUG_HTTP_TRANSACTION
		response.dump ();
		#endif

		status = response.getStatus ();
	}
	while(status == HTTP::kContinue);

	httpStatus = status;
	// No, don't bail out here. Copy response data for errors, too.
	//if(HTTP::isErrorStatus (status))
	//	return false;

	// head requests do not carry content
	if(request.getMethod () == HTTP::kHEAD)
	{
		outContent.setLength (response.getHeaders ().getContentLength ());
		outContent.setType (response.getHeaders ().getContentType ());
		return true;
	}

	bool copied = false;
	SharedPtr<IStream> dstStream = &outContent.getStream ();
	IProgressNotify* progress = progressMode == kReceived ? outerProgress : nullptr; // received progress

	// swallow response in case of auto-redirect
	if(HTTP::isRedirectStatus (status) && isAutoRedirectEnabled ())
	{
		dstStream = NEW MemoryStream;
		dstStream->release (); // shared!
		progress = nullptr; // no progress
	}

	if(response.getHeaders ().hasContentLength ())
	{
		int64 length = response.getHeaders ().getContentLength ();
		int64 requestedStart = 0, requestedEnd = 0;
		request.getHeaders ().getRangeBytes (requestedStart, requestedEnd);
		int64 rangeStart = 0, rangeEnd = 0, totalLength = 0;
		response.getHeaders ().getContentRangeBytes (rangeStart, rangeEnd, totalLength);
		if(totalLength == 0)
			totalLength = length;
		outContent.setLength (totalLength);

		if(requestedStart != rangeStart || requestedEnd != rangeEnd)
		{
			if(requestedStart > 0 && rangeStart == 0)
				dstStream->seek (0, IStream::kSeekSet); // rewind, because servers sends all the data
			// TODO treat more issues
		}
		
		if(UnknownPtr<IObserver> progressObserver = progress)
			progressObserver->notify (nullptr, Message (Meta::kContentLengthNotify, totalLength, response.getWebHeaders ()));
		if(rangeStart > 0 && progress)
		{
			ProgressOffsetter offsetter (progress, totalLength, rangeStart);
			copied = receiveData (*dstStream, length, &offsetter);
		}
		else
			copied = receiveData (*dstStream, totalLength, progress);
	}
	else
	{
		if(response.getHeaders ().isChunkedTransfer ())
		{
			int64 length = 0;
			copied = receiveChunked (*dstStream, length, progress);
			outContent.setLength (length);
		}
		else
			copied = true; // no content???
	}

	outContent.setType (response.getHeaders ().getContentType ());
	return copied;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Transaction::finish (bool failed)
{
	if(UnknownPtr<Net::INetworkStream> netStream = stream)
		netStream->setCancelCallback (nullptr);

	bool closed = request.getResponse ().getHeaders ().getConnection () == "close";
	if(closed || failed)
		connection.close ();
	stream = nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Transaction::sendData (IStream& srcStream, int64 length, IProgressNotify* progress)
{
	ASSERT (stream != nullptr)
	return System::GetFileUtilities ().copyStream (*stream, srcStream, progress, length) != 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Transaction::receiveData (IStream& dstStream, int64 length, IProgressNotify* progress)
{
	ASSERT (stream != nullptr)
	return System::GetFileUtilities ().copyStream (dstStream, *stream, progress, length) != 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Transaction::receiveChunked (IStream& dstStream, int64& length, IProgressNotify* progress)
{
	ASSERT (stream != nullptr)
	Streamer s (*stream);
	while(1)
	{
		MutableCString line;
		if(!s.readLine (line))
			return false;

		int64 chunkSize = 0;
		line.getHexValue (chunkSize);
		if(chunkSize == 0) // end of chunks
			break;

		bool copied = receiveData (dstStream, chunkSize);
		if(!copied)
			return false;

		length += chunkSize;

		if(progress)
		{
			if(progress->isCanceled ())
				return false;
			progress->updateAnimated ();

			if(UnknownPtr<IObserver> progressObserver = progress)
				progressObserver->notify (nullptr, Message (Meta::kContentLengthNotify, length, request.getResponse ().getWebHeaders ()));
		}

		line.empty ();
		if(!s.readLine (line))
			return false;
		ASSERT (line.isEmpty () == true)
	}

	// receive optional footers
	HeaderList footers;
	if(!footers.receive (*stream))
		return false;

	// TODO: merge with response headers???

	#if DEBUG_HTTP_TRANSACTION
	footers.dump ();
	#endif
	return true;
}
