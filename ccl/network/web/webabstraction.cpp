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
// Filename    : ccl/network/web/webabstraction.cpp
// Description : Web Implementation Abstraction
//
//************************************************************************************************

#include "ccl/network/web/webabstraction.h"

#include "ccl/base/message.h"

#include "ccl/public/base/memorystream.h"

using namespace CCL;
using namespace Web;

//************************************************************************************************
// WebEngine
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (WebEngine, Object)

//************************************************************************************************
// WebSocketMessage
//************************************************************************************************

DEFINE_CLASS_HIDDEN (WebSocketMessage, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

WebSocketMessage::WebSocketMessage ()
: text (false)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WebSocketMessage::setSmallPayload (const void* data, int length)
{
	ASSERT (length <= kSmallPayloadSize)
	smallPayloadLength = ccl_min (length, kSmallPayloadSize);
	::memcpy (smallPayload, data, smallPayloadLength);
	setLargePayload (nullptr);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WebSocketMessage::setPayload (const void* data, int length)
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

//////////////////////////////////////////////////////////////////////////////////////////////////

int WebSocketMessage::getPayloadLength () const
{
	return largePayload ? largePayload->getBytesWritten () : smallPayloadLength;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const void* WebSocketMessage::getPayloadData () const
{
	return largePayload ? largePayload->getMemoryAddress () : smallPayload;
}

//************************************************************************************************
// WebSocketClient
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (WebSocketClient, Object)
DEFINE_STRINGID_MEMBER_ (WebSocketClient, kConnectResult, "connectResult")
DEFINE_STRINGID_MEMBER_ (WebSocketClient, kProcessingError, "processingError")
DEFINE_STRINGID_MEMBER_ (WebSocketClient, kMessageReceived, "messageReceived")

//////////////////////////////////////////////////////////////////////////////////////////////////

WebSocketClient::WebSocketClient (IObserver* owner)
: owner (owner)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WebSocketClient::signalConnected (tresult result)
{
	(NEW Message (kConnectResult, result))->post (owner);
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
