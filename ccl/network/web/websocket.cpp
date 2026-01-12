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
// Filename    : ccl/network/web/websocket.cpp
// Description : WebSocket class
//
//************************************************************************************************

#define DEBUG_LOG 1

#include "ccl/network/web/websocket.h"
#include "ccl/network/web/http/client.h"
#include "ccl/public/network/isocket.h"

#include "ccl/base/message.h"
#include "ccl/base/storage/url.h"
#include "ccl/base/collections/objectlist.h"
#include "ccl/base/security/cryptomaterial.h"

#include "ccl/public/base/streamer.h"
#include "ccl/public/base/memorystream.h"
#include "ccl/public/system/ithreadpool.h"
#include "ccl/public/base/datetime.h"
#include "ccl/public/systemservices.h"

namespace CCL {
namespace Web {

/*
	The WebSocket Protocol - https://www.rfc-editor.org/rfc/rfc6455
*/

//************************************************************************************************
// WebSocketFrame
//************************************************************************************************

struct WebSocketFrame
{
	static constexpr ByteOrder kByteOrder = kBigEndian; // network byte order is big endian

	enum Opcodes
	{
		kContinue = 0x0,
		kText = 0x1,
		kBinary = 0x2,
		kClose = 0x8,
		kPing = 0x9,
		kPong = 0xA,

		kControlFrameBit = 1<<3
	};

	static inline bool isControlFrame (uint8 opcode) { return get_flag<uint8> (opcode, kControlFrameBit); }
	
	enum Flags
	{
		kFinal = 1<<0,
		kRSV1 = 1<<1,
		kRSV2 = 1<<2,
		kRSV3 = 1<<3
	};
	
	union
	{
		struct
		{
			uint8 flags: 4;
			uint8 opcode: 4;
			uint8 mask: 1;
			uint8 playloadLength: 7;
		} basicHeader;
		uint8 basicHeaderData[2] = {};
	};

	PROPERTY_FLAG (basicHeader.flags, kFinal, isFinal)
	void isMasked (bool state) { basicHeader.mask = state ? 1 : 0; }

	static constexpr uint8 kMaxPayloadLength7Bit = 125;
	static constexpr uint8 kPlayloadLength16Bit = 126;
	static constexpr uint8 kPlayloadLength64Bit = 127;

	struct
	{
		uint16 playloadLength16 = 0;
		uint64 payloadLength64 = 0;
		uint32 maskingKey = 0;	///< present if masking bit is set, client to server only
	} optionalHeader;
};

//************************************************************************************************
// WebSocketReader
//************************************************************************************************

class WebSocketReader
{
public:
	WebSocketReader (IStream& stream)
	: stream (stream)
	{}

	bool canRead (uint8& firstByte)
	{
		if(UnknownPtr<Net::INetworkStream> netStream = &stream)
			if(auto netSocket = netStream->getSocket ())
				return netSocket->receive (&firstByte, 1) == 1;

		return false;
	}
	
	bool readHeader (uint8 firstByte)
	{
		Streamer s (stream, WebSocketFrame::kByteOrder);
		frame.basicHeaderData[0] = firstByte;
		if(!s.read (frame.basicHeaderData[1]))
			return false;

		if(frame.basicHeader.playloadLength == WebSocketFrame::kPlayloadLength16Bit)
		{
			if(!s.read (frame.optionalHeader.playloadLength16))
				return false;
		}
		else if(frame.basicHeader.playloadLength == WebSocketFrame::kPlayloadLength64Bit)
		{
			if(!s.read (frame.optionalHeader.payloadLength64))
				return false;
		}

		if(frame.basicHeader.mask) // client to server only
		{
			if(!s.read (frame.optionalHeader.maskingKey))
				return false;
		}

		return true;
	}
	
	bool isFinal () const
	{
		return frame.isFinal ();
	}

	uint8 getOpcode () const
	{
		return frame.basicHeader.opcode;
	}

	uint64 getPayloadLength () const
	{
		switch(frame.basicHeader.playloadLength)
		{
		case WebSocketFrame::kPlayloadLength16Bit : return frame.optionalHeader.playloadLength16;
		case WebSocketFrame::kPlayloadLength64Bit : return frame.optionalHeader.payloadLength64;
		default : return frame.basicHeader.playloadLength;
		}
	}

protected:
	IStream& stream;
	WebSocketFrame frame;
};

//************************************************************************************************
// WebSocketWriter
//************************************************************************************************

class WebSocketWriter
{
public:
	WebSocketWriter (IStream& stream, bool clientMode)
	: stream (stream),
	  clientMode (clientMode)
	{}

	bool writeFrame (uint8 opcode, void* data, int length, bool final = true)
	{
		WebSocketFrame frame;
		frame.isFinal (final);
		frame.basicHeader.opcode = opcode;
		frame.isMasked (clientMode);
		if(length <= WebSocketFrame::kMaxPayloadLength7Bit)
			frame.basicHeader.playloadLength = uint8(length);
		else if(length <= NumericLimits::kMaxUnsignedInt16)
			frame.basicHeader.playloadLength = WebSocketFrame::kPlayloadLength16Bit;
		else
			frame.basicHeader.playloadLength = WebSocketFrame::kPlayloadLength64Bit;

		// basic header
		Streamer s (stream, WebSocketFrame::kByteOrder);
		if(s->write (frame.basicHeaderData, 2) != 2)
			return false;

		// payload length
		if(frame.basicHeader.playloadLength == WebSocketFrame::kPlayloadLength16Bit)
		{
			uint16 length16 = uint16(length);
			if(!s.write (length16))
				return false;
		}
		else if(frame.basicHeader.playloadLength == WebSocketFrame::kPlayloadLength64Bit)
		{
			uint64 length64 = uint64(length);
			if(!s.write (length64))
				return false;
		}

		// masking
		if(clientMode)
		{
			uint32 maskingKey = ::rand (); // note that this isn't a proper cryptographic RNG
			if(!s.write (maskingKey))
				return false;
			
			// mask data inplace
			uint8* maskingBuffer = reinterpret_cast<uint8*> (&maskingKey);
			auto dataBuffer = static_cast<uint8*> (data);
			for(int i = 0; i < length; i++)
				dataBuffer[i] = dataBuffer[i] ^ maskingBuffer[i % 4];
		}

		// payload
		if(s->write (data, length) != length)
			return false;

		return true;
	}

protected:
	IStream& stream;
	bool clientMode;
};

//************************************************************************************************
// WebSocketMessage
//************************************************************************************************

class WebSocketMessage: public Object
{
public:
	DECLARE_CLASS (WebSocketMessage, Object)

	static constexpr int kSmallPayloadSize = WebSocketFrame::kMaxPayloadLength7Bit;

	WebSocketMessage (): text (false) {}

	PROPERTY_BOOL (text, Text)
	PROPERTY_SHARED_AUTO (IMemoryStream, largePayload, LargePayload)

	void setSmallPayload (const void* data, int length)
	{
		ASSERT (length <= kSmallPayloadSize)
		smallPayloadLength = ccl_min (length, kSmallPayloadSize);
		::memcpy (smallPayload, data, smallPayloadLength);
		setLargePayload (nullptr);
	}

	void setPayload (const void* data, int length)
	{
		if(length <= kSmallPayloadSize)
			setSmallPayload (data, length);
		else
		{
			largePayload = NEW MemoryStream;
			largePayload->write (data, length);
			largePayload->rewind ();
			smallPayloadLength = 0;
		}
	}

	int getPayloadLength () const
	{
		return largePayload ? largePayload->getBytesWritten () : smallPayloadLength;
	}

	const void* getPayloadData () const
	{
		return largePayload ? largePayload->getMemoryAddress () : smallPayload;
	}

protected:
	uint8 smallPayload[kSmallPayloadSize] = {};
	int smallPayloadLength = 0;
};

DEFINE_CLASS_HIDDEN (WebSocketMessage, Object)

//************************************************************************************************
// WebSocketClient
//************************************************************************************************

class WebSocketClient: public Object,
					   public Threading::IPeriodicItem
{
public:
	WebSocketClient (IObserver* owner);
	~WebSocketClient ();

	static constexpr uint64 kMaxPayloadLength = 8 * 1024 * 1024; // 8 MB limit
	static constexpr int kReadWriteTimeout = 5 * 1000; // Don't block longer than this on read/write operations

	tresult connect (UrlRef url, VariantRef protocols, IProgressNotify* progress);
	void signalConnected (tresult result);
	tresult process ();
	void signalReceived (WebSocketMessage* message);
	void signalError ();
	void queueMessage (WebSocketMessage* message);
	uint32 getBufferedAmount () const;
	void flushAll ();
	void disconnect ();

	DECLARE_STRINGID_MEMBER (kConnectResult)
	DECLARE_STRINGID_MEMBER (kProcessingError)
	DECLARE_STRINGID_MEMBER (kMessageReceived)

	// IPeriodicItem
	int64 CCL_API getExecutionTime () const override;
	void CCL_API execute (int64 now) override;

	CLASS_INTERFACE (IPeriodicItem, Object)

protected:
	IObserver* owner;
	AutoPtr<IStream> stream;
	int64 nextExecutionTime;
	Threading::CriticalSection sendQueueLock;
	ObjectList sendQueue;
	int bufferedAmount;
	AutoPtr<WebSocketMessage> pendingMessage;

	WebSocketMessage* retrieveNextMessage ();
};

//************************************************************************************************
// WebSocketConnectWork
//************************************************************************************************

class WebSocketConnectWork: public Object,
							public Threading::AbstractWorkItem
{
public:
	WebSocketConnectWork (WebSocketClient& client, UrlRef url, VariantRef protocols, IProgressNotify* progress);

	// IWorkItem
	void CCL_API work () override;

	CLASS_INTERFACE (IWorkItem, Object)

protected:
	WebSocketClient& client;
	Url url;
	Variant protocols;
	SharedPtr<IProgressNotify> progress;
};

} // naemspace Web
} // namespace CCL

using namespace CCL;
using namespace Web;

//************************************************************************************************
// WebSocket::CancelHelper
//************************************************************************************************

tbool CCL_API WebSocket::CancelHelper::isCanceled ()
{
	if(exiting)
	{
		CCL_PRINTLN ("WebSocket canceled on exit")
		return true;
	}
	return canceled;
}

//************************************************************************************************
// WebSocket
//************************************************************************************************

DEFINE_CLASS (WebSocket, Object)
DEFINE_CLASS_NAMESPACE (WebSocket, "Network")
DEFINE_CLASS_UID (WebSocket, 0x3e8ea54b, 0xe756, 0x4eb6, 0xba, 0x89, 0x6a, 0x57, 0x3b, 0xc0, 0xc8, 0xb4)

//////////////////////////////////////////////////////////////////////////////////////////////////

bool WebSocket::exiting = false;
void WebSocket::cancelOnExit () { exiting = true; }

//////////////////////////////////////////////////////////////////////////////////////////////////

WebSocket::WebSocket ()
: readyState (kClosed),
  client (nullptr)
{
	client = NEW WebSocketClient (this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

WebSocket::~WebSocket ()
{
	cancelSignals ();

	ASSERT (readyState == kClosed)
	safe_release (client);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WebSocket::setState (ReadyState newState)
{
	ASSERT (System::IsInMainThread ())
	if(readyState != newState)
	{
		readyState = newState; 
		signal (Message (kOnReadyStateChange));
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

WebSocket::ReadyState CCL_API WebSocket::getReadyState () const
{
	return readyState;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

uint32 CCL_API WebSocket::getBufferedAmount () const
{
	return client->getBufferedAmount ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringRef CCL_API WebSocket::getExtensions () const
{
	CCL_NOT_IMPL ("Implement me!\n")
	return String::kEmpty;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringRef CCL_API WebSocket::getProtocol () const
{
	CCL_NOT_IMPL ("Implement me!\n")
	return String::kEmpty;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

UrlRef CCL_API WebSocket::getUrl () const
{
	return url;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API WebSocket::open (UrlRef url, VariantRef protocols)
{
	if(readyState != kClosed)
		return kResultUnexpected;

	System::GetThreadPool ().scheduleWork (NEW WebSocketConnectWork (*client, url, protocols, &cancelHelper));
	
	this->url.assign (url);
	setState (kConnecting);
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API WebSocket::close (int code, StringRef reason)
{
	if(readyState != kClosed)
	{
		if(readyState == kConnecting)
		{
			cancelHelper.setCanceled (true);
			System::GetThreadPool ().cancelWork (client, true);
		}
		else if(readyState == kOpen)
		{
			cancelHelper.setCanceled (true);
			System::GetThreadPool ().removePeriodic (client);
		}

		setState (kClosing);

		// TODO: properly send close message to server...

		client->flushAll ();
		client->disconnect ();
		this->url = Url::kEmpty;
		cancelHelper.setCanceled (false);

		setState (kClosed);
		signal (Message (kOnClose));
	}
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API WebSocket::send (VariantRef data)
{
	if(readyState != kOpen)
		return kResultUnexpected;
	
	if(data.isString ())
	{
		MutableCString textUtf8 (data.asString (), Text::kUTF8);
		int textLength = textUtf8.length ();
		ASSERT (textLength <= WebSocketClient::kMaxPayloadLength)
		if(textLength > WebSocketClient::kMaxPayloadLength)
			return kResultOutOfMemory;

		auto message = NEW WebSocketMessage;
		message->setText (true);
		message->setPayload (textUtf8.str (), textLength);
		
		client->queueMessage (message);
		return kResultOk;
	}
	else
	{
		UnknownPtr<IMemoryStream> payload (data.asUnknown ());
		if(!payload)
			return kResultInvalidArgument;

		auto binaryLength = payload->getBytesWritten ();
		ASSERT (binaryLength <= WebSocketClient::kMaxPayloadLength)
		if(binaryLength > WebSocketClient::kMaxPayloadLength)
			return kResultOutOfMemory;

		auto message = NEW WebSocketMessage;
		message->setLargePayload (payload);

		client->queueMessage (message);
		return kResultOk;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API WebSocket::notify (ISubject* subject, MessageRef msg)
{
	if(msg == WebSocketClient::kMessageReceived)
	{
		auto message = unknown_cast<WebSocketMessage> (msg[0]);
		ASSERT (message)
		if(message->isText ())
		{
			String text;
			text.appendCString (Text::kUTF8, 
								static_cast<CStringPtr> (message->getPayloadData ()),
								message->getPayloadLength ());

			signal (Message (kOnMessage, text));
		}
		else
		{
			AutoPtr<IMemoryStream> data;
			if(auto lp = message->getLargePayload ())
				data.share (lp);
			else
			{
				data = NEW MemoryStream;
				data->write (message->getPayloadData (), message->getPayloadLength ());
				data->rewind ();
			}
			signal (Message (kOnMessage, data));
		}
	}
	else if(msg == WebSocketClient::kConnectResult)
	{
		tresult result = msg[0].asResult ();
		if(result == kResultOk)
		{
			setState (kOpen);
			signal (Message (kOnOpen));
			System::GetThreadPool ().addPeriodic (client);
		}
		else
		{
			setState (kClosed);
			signal (Message (kOnError));
		}
	}
	else if(msg == WebSocketClient::kProcessingError)
	{
		close ();
		signal (Message (kOnError));
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_PROPERTY_NAMES (WebSocket)
	DEFINE_PROPERTY_NAME ("readyState")
	DEFINE_PROPERTY_NAME ("bufferedAmount")
END_PROPERTY_NAMES (WebSocket)

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API WebSocket::getProperty (Variant& var, MemberID propertyId) const
{
	if(propertyId == "readyState")
	{
		var = readyState;
		return true;
	}
	else if(propertyId == "bufferedAmount")
	{
		var = int64(getBufferedAmount ());
		return true;
	}
	return SuperClass::getProperty (var, propertyId);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_METHOD_NAMES (WebSocket)
	DEFINE_METHOD_ARGS ("open", "url, protocols")
	DEFINE_METHOD_ARGS ("close", "code, reason")
	DEFINE_METHOD_ARGS ("send", "data")
END_METHOD_NAMES (WebSocket)

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API WebSocket::invokeMethod (Variant& returnValue, MessageRef msg)
{
	if(msg == "open")
	{
		AutoPtr<IUrl> url;
		if(msg[0].isObject ())
			url = UnknownPtr<IUrl> (msg[0].asUnknown ()).detach ();
		if(url == nullptr)
			url = NEW Url (msg[0].asString ());

		Variant protocols;
		if(msg.getArgCount () > 1)
			protocols = msg[1];
		
		returnValue = open (*url, protocols);
		return true;
	}
	else if(msg == "close")
	{
		int code = msg.getArgCount () > 0 ? msg[0].asInt () : 0;
		String reason;
		if(msg.getArgCount () > 1)
			reason = msg[1].asString ();

		returnValue = close (code, reason);
		return true;
	}
	else if(msg == "send")
	{
		returnValue = send (msg[0]);
		return true;
	}
	return SuperClass::invokeMethod (returnValue, msg);
}

//************************************************************************************************
// WebSocketConnectWork
//************************************************************************************************

WebSocketConnectWork::WebSocketConnectWork (WebSocketClient& client, UrlRef url, VariantRef protocols, IProgressNotify* progress)
: AbstractWorkItem (&client), // use client as work id for cancelation
  client (client),
  url (url),
  protocols (protocols),
  progress (progress)
{
	this->protocols.share ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API WebSocketConnectWork::work ()
{
	tresult result = client.connect (url, protocols, progress);
	if(progress && progress->isCanceled ())
		return;

	client.signalConnected (result);
}

//************************************************************************************************
// WebSocketClient
//************************************************************************************************

DEFINE_STRINGID_MEMBER_ (WebSocketClient, kConnectResult, "connectResult")
DEFINE_STRINGID_MEMBER_ (WebSocketClient, kProcessingError, "processingError")
DEFINE_STRINGID_MEMBER_ (WebSocketClient, kMessageReceived, "messageReceived")

//////////////////////////////////////////////////////////////////////////////////////////////////

WebSocketClient::WebSocketClient (IObserver* owner)
: owner (owner),
  nextExecutionTime (0),
  bufferedAmount (0)
{
	sendQueue.objectCleanup (true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

WebSocketClient::~WebSocketClient ()
{
	stream.release ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult WebSocketClient::connect (UrlRef url, VariantRef protocols, IProgressNotify* progress)
{
	ASSERT (stream == nullptr)
	
	MutableCString protocol (url.getProtocol ());
	bool useSSL = protocol.compare (Meta::kWSS, false) == Text::kEqual;
	String hostname (url.getHostName ());

	AutoPtr<HTTP::Connection> connection = HTTP::Connection::resolve (hostname, useSSL);
	if(!connection)
		return kResultFailed;

	String path = UrlUtils::toResourcePath (url);
	MutableCString encodedPath = UrlUtils::toEncodedPath (path);

	MemoryStream outData;
	HTTP::Content outContent (outData);
	HTTP::Transaction t (*connection, HTTP::kGET, encodedPath, outContent);
	t.setProgress (progress);
	t.setUserAgent (HTTP::Client::getUserAgent ()); // not required by spec
	t.setHeader ("Upgrade", "websocket");
	t.setHeader ("Connection", "upgrade");

	MutableCString challengeKey;
	challengeKey.appendFormat ("%" FORMAT_INT64 "d", UnixTime::getTime ());
	Security::Crypto::Material material (Security::Crypto::Block (challengeKey.str (), challengeKey.length ()));
	t.setHeader ("Sec-WebSocket-Key", material.toCBase64 ());
	t.setHeader ("Sec-WebSocket-Version", "13");
	if(protocols.isString ())
		t.setHeader ("Sec-WebSocket-Protocol", MutableCString (protocols.asString (), Text::kUTF8));

	int status = 0;
	t.perform (status);	
	if(status != HTTP::kSwitchingProtocols)
		return kResultFailed;
	
	// TODO: implement HTTP redirects...

	auto& serverHeaders = t.getResponseHeaders ().getEntries ();
	if(serverHeaders.lookupValue ("Connection") != "Upgrade")
		return kResultFailed;
	if(serverHeaders.lookupValue ("Upgrade") != "websocket")
		return kResultFailed;
	auto& responseKey = serverHeaders.lookupValue ("Sec-WebSocket-Accept");
	if(responseKey.isEmpty ())
		return kResultFailed;

	// TODO: check if response is base64-encoded SHA-1 of challengeKey + "258EAFA5-E914-47DA-95CA-C5AB0DC85B11"...

	stream = connection->detach ();
	ASSERT (stream)
	if(UnknownPtr<Net::INetworkStream> netStream = stream)
	{
		netStream->setCancelCallback (progress);
		netStream->setTimeout (kReadWriteTimeout);
	}
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WebSocketClient::signalConnected (tresult result)
{
	(NEW Message (kConnectResult, result))->post (owner);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult WebSocketClient::process ()
{
	ASSERT (stream)
	bool anythingHappened = false;

	// Send pending messages
	while(1)
	{
		AutoPtr<WebSocketMessage> msg = retrieveNextMessage ();
		if(!msg)
			break;

		anythingHappened = true;

		void* mutableData = const_cast<void*> (msg->getPayloadData ());
		int length = msg->getPayloadLength ();
		bool text = msg->isText ();
		CCL_PRINTF ("WebSocket send frame: data = %p length = %d (%s)\n", mutableData, length, text ? "text" : "binary")

		WebSocketWriter writer (*stream, true);
		uint8 opcode = text ? WebSocketFrame::kText : WebSocketFrame::kBinary;
		if(!writer.writeFrame (opcode, mutableData, length)) // call _does_ block
			return kResultFailed;
	}
	
	// Read frames from server
	WebSocketReader reader (*stream);
	uint8 firstByte = 0;
	if(reader.canRead (firstByte)) // does not block
	{
		anythingHappened = true;

		if(!reader.readHeader (firstByte)) // call _does_ block
			return kResultFailed;

		uint64 length64 = reader.getPayloadLength ();
		ASSERT (length64 <= kMaxPayloadLength)
		if(length64 > kMaxPayloadLength)
			return kResultOutOfMemory;

		int length = int(length64);
		uint8 opcode = reader.getOpcode ();
		CCL_PRINTF ("WebSocket frame received: opcode = %d payload length = %d\n", int(opcode), length)

		if(WebSocketFrame::isControlFrame (opcode))
		{
			// control frame must not be fragmented
			ASSERT (length <= WebSocketFrame::kMaxPayloadLength7Bit)
			if(length > WebSocketFrame::kMaxPayloadLength7Bit)
				return kResultInvalidArgument;

			uint8 controlData[WebSocketFrame::kMaxPayloadLength7Bit] = {};
			if(!stream->read (controlData, length))
				return kResultFailed;

			if(opcode == WebSocketFrame::kClose)
			{
				// TODO: send back close message and stop further processing...
			}
			else if(opcode == WebSocketFrame::kPing)
			{
				// send back pong frame
				WebSocketWriter writer (*stream, true);
				if(!writer.writeFrame (WebSocketFrame::kPong, controlData, length)) // call _does_ block
					return kResultFailed;
			}
		}
		else // data frame (text or binary)
		{			
			if(!pendingMessage)
			{
				pendingMessage = NEW WebSocketMessage;
				if(opcode == WebSocketFrame::kText)
					pendingMessage->setText (true);
			}

			if(!pendingMessage->getLargePayload ())
				pendingMessage->setLargePayload (AutoPtr<MemoryStream> (NEW MemoryStream));

			auto ms = pendingMessage->getLargePayload ();
			uint32 offset = ms->getBytesWritten ();
			uint32 totalPayloadSize = offset + length;
			if(!ms->allocateMemoryForStream (totalPayloadSize))
				return kResultOutOfMemory;

			// read payload
			uint8* dst = static_cast<uint8*> (ms->getMemoryAddress ()) + offset;
			if(stream->read (dst, length) != length) // call _does_ block
				return kResultFailed;
			ms->setBytesWritten (totalPayloadSize);

			if(reader.isFinal ())
			{
				signalReceived (pendingMessage);
				pendingMessage.release ();
			}
		}
	}

	return anythingHappened ? kResultTrue : kResultFalse;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WebSocketClient::signalError ()
{
	(NEW Message (kProcessingError))->post (owner);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WebSocketClient::signalReceived (WebSocketMessage* message)
{
	(NEW Message (kMessageReceived, ccl_as_unknown (message)))->post (owner);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WebSocketClient::queueMessage (WebSocketMessage* message)
{
	Threading::ScopedLock scopedLock (sendQueueLock);
	sendQueue.add (message);
	bufferedAmount += message->getPayloadLength ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

WebSocketMessage* WebSocketClient::retrieveNextMessage ()
{
	Threading::ScopedLock scopedLock (sendQueueLock);
	auto message = static_cast<WebSocketMessage*> (sendQueue.removeFirst ());
	if(message)
		bufferedAmount -= message->getPayloadLength ();
	return message;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

uint32 WebSocketClient::getBufferedAmount () const
{
	return bufferedAmount;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WebSocketClient::flushAll ()
{
	Threading::ScopedLock scopedLock (sendQueueLock);
	sendQueue.removeAll ();

	pendingMessage.release ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WebSocketClient::disconnect ()
{
	stream.release ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int64 CCL_API WebSocketClient::getExecutionTime () const
{
	return nextExecutionTime;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API WebSocketClient::execute (int64 now)
{
	tresult result = process ();
	if(!(result == kResultTrue || result == kResultFalse))
		signalError ();

	nextExecutionTime = now + 1000; // 1 second	
}
