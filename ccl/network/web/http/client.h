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
// Filename    : ccl/network/web/http/client.h
// Description : HTTP Client
//
//************************************************************************************************

#ifndef _ccl_httpclient_h
#define _ccl_httpclient_h

#include "ccl/base/singleton.h"
#include "ccl/base/collections/objectarray.h"

#include "ccl/network/web/webclient.h"
#include "ccl/network/web/http/request.h"

#include "ccl/public/system/threadsync.h"
#include "ccl/public/system/ithreadpool.h"
#include "ccl/public/network/isocket.h"

namespace CCL {
namespace Web {
namespace HTTP {

class Content;
class Connection;
class Transaction;

//************************************************************************************************
// HTTP::Client
//************************************************************************************************

class Client: public WebClient
{
public:
	DECLARE_CLASS (Client, WebClient)

	Client (bool useSSL = false);
	~Client ();

	static void setUserAgent (StringID userAgent);
	static StringID getUserAgent (bool useDefault = true);

	static const int kMaxRedirectCount = 3;
	class RedirectCounter;
	PROPERTY_BOOL (autoRedirectEnabled, AutoRedirectEnabled)

	// WebClient
	tresult CCL_API connect (StringRef hostname) override;
	tresult CCL_API disconnect () override;
	tresult CCL_API downloadData (StringRef remotePath, IStream& localStream, IWebHeaderCollection* headers = nullptr, 
								  IProgressNotify* progress = nullptr) override;
	tresult CCL_API uploadData (IWebHeaderCollection* headers, IStream& localStream, StringRef remotePath, IStream& responseStream, 
								StringID method = nullptr, IProgressNotify* progress = nullptr) override;

protected:
	static MutableCString userAgentName;
	static const CString defaultUserAgentName;

	bool useSSL;
	Connection* connection;
	
	void prepare (Transaction& t);
};

//************************************************************************************************
// HTTP::Connection
//************************************************************************************************

class Connection: public Object
{
public:
	DECLARE_CLASS (Connection, Object)

	Connection (StringRef hostname = nullptr, const Net::IPAddress& address = Net::IPAddress (), bool useSSL = false);
	~Connection ();

	static Connection* resolve (StringRef hostname, bool useSSL);

	PROPERTY_STRING (hostname, Hostname)
	PROPERTY_BOOL (useSSL, UseSSL)
	PROPERTY_OBJECT (Net::IPAddress, address, Address)

	IStream* open (IProgressNotify* progress);
	void close ();
	bool isOpen () const;
	IStream* detach ();

	// used for managing persistent connections:
	PROPERTY_BOOL (persistent, Persistent)
	PROPERTY_BOOL (inUse, InUse)
	PROPERTY_VARIABLE (int64, timeLastUsed, TimeLastUsed)

protected:
	IStream* stream;
};

//************************************************************************************************
// HTTP::ConnectionManager
//************************************************************************************************

class ConnectionManager: public Object,
						 public Threading::IPeriodicItem,
						 public Singleton<ConnectionManager>
{
public:
	ConnectionManager ();
	~ConnectionManager ();

	void terminate ();

	Connection* use (StringRef hostname, bool useSSL);
	void unuse (Connection* c);

	// Object
	void CCL_API notify (ISubject* subject, MessageRef msg) override;

	CLASS_INTERFACE (IPeriodicItem, Object)

protected:
	static const int kMaxConnectionsPerHost = 6; // (Firefox defaults to 6)
	static const int kConnectionIdleTimeout = 7 * 1000;
	static const int kConnectionCheckInterval = 2 * 1000;

	Threading::CriticalSection lock;
	ObjectArray connections;
	int64 lastExecutionTime;
	bool checkEnabled;

	void enableCheckDeferred ();
	void checkConnections ();
	void enableCheck (bool state);

	// IPeriodicItem
	int64 CCL_API getExecutionTime () const override;
	void CCL_API execute (int64 now) override;
};

//************************************************************************************************
// HTTP::Content
//************************************************************************************************

class Content
{
public:
	Content (IStream& stream, int64 length = 0, StringID type = nullptr)
	: stream (stream),
	  length (length),
	  type (type)
	{}

	PROPERTY_VARIABLE (int64, length, Length)
	PROPERTY_MUTABLE_CSTRING (type, Type)

	IStream& getStream () const { return stream; }

	#if DEBUG
	void dump () const;
	#endif

protected:
	IStream& stream;
};

//************************************************************************************************
// HTTP::Transaction
//************************************************************************************************

class Transaction
{
public:
	Transaction (Connection& connection, 
				 StringID method, 
				 StringID path, 
				 Content& outContent, 
				 const Content* inContent = nullptr);

	static const int kVersion; ///< supported HTTP protocol version

	enum ProgressMode { kNone, kSend, kReceived };
	PROPERTY_VARIABLE (ProgressMode, progressMode, ProgressMode)
	PROPERTY_POINTER (IProgressNotify, outerProgress, Progress)
	PROPERTY_BOOL (autoRedirectEnabled, AutoRedirectEnabled)

	void setHeaders (IWebHeaderCollection& headers);
	void setUserAgent (StringID userAgent);
	void setHeader (StringID key, StringID value);
	void setCredentials (IWebCredentials* credentials);
	
	bool perform (int& httpStatus);

	HeaderList& getResponseHeaders ();

protected:
	Connection& connection;
	Request request;
	Content& outContent;
	const Content* inContent;
	IStream* stream;

	bool begin ();
	bool sendRequest ();
	bool receiveResponse (int& httpStatus);
	void finish (bool failed);

	bool sendData (IStream& srcStream, int64 length, IProgressNotify* progress = nullptr);
	bool receiveData (IStream& dstStream, int64 length, IProgressNotify* progress = nullptr);
	bool receiveChunked (IStream& dstStream, int64& length, IProgressNotify* progress = nullptr);
};

} // namespace HTTP
} // namespace Web
} // namespace CCL

#endif // _ccl_httpclient_h
