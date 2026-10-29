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
#include "ccl/network/web/webabstraction.h"

#include "ccl/base/message.h"

#include "ccl/public/systemservices.h"

namespace CCL {
namespace Web {

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

} // namespace Web
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
DEFINE_CLASS_UID (WebSocket, 0x3e8ea54b, 0xe756, 0x4eb6, 0xba, 0x89, 0x6a, 0x57, 0x3b, 0xc0, 0xc8, 0xb4)
DEFINE_CLASS_CATEGORY (WebSocket, PLUG_CATEGORY_NETWORK_)

//////////////////////////////////////////////////////////////////////////////////////////////////

bool WebSocket::exiting = false;
void WebSocket::cancelOnExit () { exiting = true; }

//////////////////////////////////////////////////////////////////////////////////////////////////

WebSocket::WebSocket ()
: readyState (kClosed),
  client (nullptr),
  nextExecutionTime (0)
{
	client = WebEngine::instance ().createWebSocketClient (this);
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
			System::GetThreadPool ().removePeriodic (this);
		}

		setState (kClosing);

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

		auto* message = NEW WebSocketMessage;
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

		auto* message = NEW WebSocketMessage;
		message->setLargePayload (payload);

		client->queueMessage (message);
		return kResultOk;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int64 CCL_API WebSocket::getExecutionTime () const
{
	return nextExecutionTime;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API WebSocket::execute (int64 now)
{
	tresult result = client->process ();
	if(!(result == kResultTrue || result == kResultFalse))
		client->signalError ();

	nextExecutionTime = now + 1000; // 1 second	
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
			System::GetThreadPool ().addPeriodic (this);
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

