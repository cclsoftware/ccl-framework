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
// Filename    : ccl/network/web/internal/wsclient.cpp
// Description : Internal WebSocket Client
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "ccl/network/web/internal/wsclient.h"
#include "ccl/network/web/internal/httpclient.h"

#include "ccl/base/storage/url.h"
#include "ccl/base/security/cryptomaterial.h"

#include "ccl/public/base/streamer.h"
#include "ccl/public/network/web/iwebsocket.h"

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

	// bit layout of basicHeaderData[0]: FIN(7) RSV1(6) RSV2(5) RSV3(4) OPCODE(3..0)
	// bit layout of basicHeaderData[1]: MASK(7) PAYLOADLENGTH(6..0)
	// note: deliberately not using C++ bitfields here, their physical bit order within
	// a byte is implementation-defined and does not match this wire format on common
	// little-endian compilers (bitfields there are packed starting at the low-order bit)

	enum Flags
	{
		kFinal = 1<<7,
		kRSV1 = 1<<6,
		kRSV2 = 1<<5,
		kRSV3 = 1<<4
	};

	static constexpr uint8 kMaskBit = 1<<7;
	static constexpr uint8 kOpcodeMask = 0x0F;
	static constexpr uint8 kPayloadLengthMask = 0x7F;

	uint8 basicHeaderData[2] = {};

	bool isFinal () const { return get_flag<uint8> (basicHeaderData[0], kFinal); }
	void isFinal (bool state) { set_flag<uint8> (basicHeaderData[0], kFinal, state); }

	bool isMasked () const { return get_flag<uint8> (basicHeaderData[1], kMaskBit); }
	void isMasked (bool state) { set_flag<uint8> (basicHeaderData[1], kMaskBit, state); }

	uint8 getOpcode () const { return basicHeaderData[0] & kOpcodeMask; }
	void setOpcode (uint8 opcode) { basicHeaderData[0] = (basicHeaderData[0] & ~kOpcodeMask) | (opcode & kOpcodeMask); }

	uint8 getPayloadLength7 () const { return basicHeaderData[1] & kPayloadLengthMask; }
	void setPayloadLength7 (uint8 length) { basicHeaderData[1] = (basicHeaderData[1] & ~kPayloadLengthMask) | (length & kPayloadLengthMask); }

	static constexpr uint8 kMaxPayloadLength7Bit = WebSocketMessage::kSmallPayloadSize;
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

		if(frame.getPayloadLength7 () == WebSocketFrame::kPlayloadLength16Bit)
		{
			if(!s.read (frame.optionalHeader.playloadLength16))
				return false;
		}
		else if(frame.getPayloadLength7 () == WebSocketFrame::kPlayloadLength64Bit)
		{
			if(!s.read (frame.optionalHeader.payloadLength64))
				return false;
		}

		if(frame.isMasked ()) // client to server only
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
		return frame.getOpcode ();
	}

	uint64 getPayloadLength () const
	{
		switch(frame.getPayloadLength7 ())
		{
		case WebSocketFrame::kPlayloadLength16Bit : return frame.optionalHeader.playloadLength16;
		case WebSocketFrame::kPlayloadLength64Bit : return frame.optionalHeader.payloadLength64;
		default : return frame.getPayloadLength7 ();
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
		frame.setOpcode (opcode);
		frame.isMasked (clientMode);
		if(length <= WebSocketFrame::kMaxPayloadLength7Bit)
			frame.setPayloadLength7 (uint8(length));
		else if(length <= NumericLimits::kMaxUnsignedInt16)
			frame.setPayloadLength7 (WebSocketFrame::kPlayloadLength16Bit);
		else
			frame.setPayloadLength7 (WebSocketFrame::kPlayloadLength64Bit);

		// basic header
		Streamer s (stream, WebSocketFrame::kByteOrder);
		if(s->write (frame.basicHeaderData, 2) != 2)
			return false;

		// payload length
		if(frame.getPayloadLength7 () == WebSocketFrame::kPlayloadLength16Bit)
		{
			uint16 length16 = uint16(length);
			if(!s.write (length16))
				return false;
		}
		else if(frame.getPayloadLength7 () == WebSocketFrame::kPlayloadLength64Bit)
		{
			uint64 length64 = uint64(length);
			if(!s.write (length64))
				return false;
		}

		// masking
		if(clientMode)
		{
			uint32 maskingKey = ::rand (); // note that this isn't a proper cryptographic RNG
			uint8* maskingBuffer = reinterpret_cast<uint8*> (&maskingKey);
			if(s->write (maskingBuffer, 4) != 4) // raw write, must match byte order used for masking below
				return false;

			// mask data inplace
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

} // namespace Web
} // namespace CCL

using namespace CCL;
using namespace Web;

//************************************************************************************************
// InternalWSClient
//************************************************************************************************

InternalWSClient::InternalWSClient (IObserver* owner)
: WebSocketClient (owner),
  bufferedAmount (0)
{
	sendQueue.objectCleanup (true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

InternalWSClient::~InternalWSClient ()
{
	stream.release ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult InternalWSClient::connect (UrlRef url, VariantRef protocols, IProgressNotify* progress)
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

tresult InternalWSClient::process ()
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

			IMemoryStream* ms = pendingMessage->getLargePayload ();
			uint32 offset = ms->getBytesWritten ();
			uint32 totalPayloadSize = offset + length;
			if(!ms->allocateMemoryForStream (totalPayloadSize))
				return kResultOutOfMemory;

			// read payload
			uint8* dst = static_cast<uint8*> (ms->getMemoryAddress ()) + offset;
			int numRead = stream->read (dst, length); // call _does_ block
			if(numRead != length)
			{
				CCL_PRINTLN ("WebSocket read payload failed!")
				return kResultFailed;
			}
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

void InternalWSClient::queueMessage (WebSocketMessage* message)
{
	Threading::ScopedLock scopedLock (sendQueueLock);
	sendQueue.add (message);
	bufferedAmount += message->getPayloadLength ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

WebSocketMessage* InternalWSClient::retrieveNextMessage ()
{
	Threading::ScopedLock scopedLock (sendQueueLock);
	auto message = static_cast<WebSocketMessage*> (sendQueue.removeFirst ());
	if(message)
		bufferedAmount -= message->getPayloadLength ();
	return message;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

uint32 InternalWSClient::getBufferedAmount () const
{
	return bufferedAmount;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void InternalWSClient::flushAll ()
{
	Threading::ScopedLock scopedLock (sendQueueLock);
	sendQueue.removeAll ();

	pendingMessage.release ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void InternalWSClient::disconnect ()
{
	// TODO: properly send close message to server...

	stream.release ();
}
