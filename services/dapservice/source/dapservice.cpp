//************************************************************************************************
//
// DAP Service
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
// Filename    : dapservice.cpp
// Description : DAP Service
//
//************************************************************************************************

#include "dapservice.h"

#include "ccl/public/base/variant.h"
#include "ccl/public/base/memorystream.h"
#include "ccl/public/storage/filetype.h"
#include "ccl/public/netservices.h"
#include "ccl/public/network/web/iwebservice.h"
#include "ccl/public/network/web/iwebrequest.h"
#include "ccl/public/systemservices.h"
#include "ccl/public/system/isignalhandler.h"

#include "ccl/base/object.h"
#include "ccl/base/message.h"
#include "ccl/base/storage/jsonarchive.h"
#include "ccl/base/storage/attributes.h"

using namespace CCL;

//************************************************************************************************
// String Constants
//************************************************************************************************

namespace DAP
{
	DEFINE_STRINGID_ (kIdBody, "body")
	DEFINE_STRINGID_ (kIdType, "type")
	DEFINE_STRINGID_ (kIdCommand, "command")
	DEFINE_STRINGID_ (kIdEvent, "event")
	DEFINE_STRINGID_ (kIdThreadId, "threadId")
	DEFINE_STRINGID_ (kIdArguments, "arguments")
	DEFINE_STRINGID_ (kIdSeq, "seq")
	DEFINE_STRINGID_ (kIdExitCode, "exitCode")
	DEFINE_STRINGID_ (kIdRequestSeq, "request_seq")
	DEFINE_STRINGID_ (kIdSuccess, "success")
	DEFINE_STRINGID_ (kIdSupportsConfigurationDoneRequest, "supportsConfigurationDoneRequest")
	DEFINE_STRINGID_ (kIdId, "id")
	DEFINE_STRINGID_ (kIdName, "name")
	DEFINE_STRINGID_ (kIdThreads, "threads")

	static const String kRequest ("request");
	static const String kResponse ("response");
	static const String kEvent ("event");
	static const String kInitialize ("initialize");
	static const String kDisconnect ("disconnect");
	static const String kAttach ("attach");
	static const String kEnableNetworking ("enableNetworking");
	static const String kConfigurationDone ("configurationDone");
	static const String kThreads ("threads");
	static const String kInitialized ("initialized");
	static const String kExited ("exited");
}

//************************************************************************************************
// DAPMessage
//************************************************************************************************

class DAPMessage: public Object,
                  public IDebugMessage
{
public:
	DECLARE_CLASS_ABSTRACT (DAPMessage, Object)

	DEFINE_ENUM (MessageType)
	{
		kTypeUnknown,
		kTypeRequest,
		kTypeResponse,
		kTypeEvent
	};

	DEFINE_ENUM (MessageCommand)
	{
		kCommandUnknown,
		kCommandInitialize,
		kCommandDisconnect,
		kCommandAttach,
		kCommandEnableNetworking,
		kCommandConfigurationDone,
		kCommandThreads,

		kEventInitialized,
		kEventExited
	};

	DAPMessage (MessageType type = kTypeUnknown, MessageCommand command = kCommandUnknown, int threadId = kBroadcastThreadId);

	static StringRef messageTypeToString (MessageType type);
	static MessageType parseMessageType (StringRef string);
	static StringRef messageCommandToString (MessageCommand command);
	static MessageCommand parseMessageCommand (StringRef string);

	MessageType getMessageType () const { return type; }
	MessageCommand getMessageCommand () const { return command; }
	int getSequenceNumber () const { return sequenceNumber; }

	const Attributes& getAttributes () const { return attributes; }
	Attributes& getAttributes () { return attributes; }
	void setAttributes (const Attributes& attributes);

	// IDebugMessage
	int CCL_API getThreadId () const override { return threadId; }
	void CCL_API getRawData (String& data) const override;
	void CCL_API setRawData (StringRef data) override;

	CLASS_INTERFACE (IDebugMessage, Object)

protected:
	MessageType type;
	MessageCommand command;
	int threadId;
	Attributes attributes;
	int sequenceNumber;

	void updateProperties ();
};

//************************************************************************************************
// DAPMessage
//************************************************************************************************

/*static*/StringRef DAPMessage::messageTypeToString (MessageType type)
{
	if(type == kTypeRequest)
		return DAP::kRequest;
	else if(type == kTypeResponse)
		return DAP::kResponse;
	else if(type == kTypeEvent)
		return DAP::kEvent;

	return String::kEmpty;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

/*static*/DAPMessage::MessageCommand DAPMessage::parseMessageType (StringRef string)
{
	if(string == DAP::kRequest)
		return kTypeRequest;
	else if(string == DAP::kResponse)
		return kTypeResponse;
	else if(string == DAP::kEvent)
		return kTypeEvent;

	return kTypeUnknown;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

/*static*/StringRef DAPMessage::messageCommandToString (MessageCommand command)
{
	// commands
	if(command == kCommandInitialize)
		return DAP::kInitialize;
	else if(command == kCommandDisconnect)
		return DAP::kDisconnect;
	else if(command == kCommandAttach)
		return DAP::kAttach;
	else if(command == kCommandEnableNetworking)
		return DAP::kEnableNetworking;
	else if(command == kCommandConfigurationDone)
		return DAP::kConfigurationDone;
	else if(command == kCommandThreads)
		return DAP::kThreads;

	// events
	else if(command == kEventInitialized)
		return DAP::kInitialized;
	else if(command == kEventExited)
		return DAP::kExited;

	return String::kEmpty;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

/*static*/DAPMessage::MessageCommand DAPMessage::parseMessageCommand (StringRef string)
{
	// commands
	if(string == DAP::kInitialize)
		return kCommandInitialize;
	else if(string == DAP::kDisconnect)
		return kCommandDisconnect;
	else if(string == DAP::kAttach)
		return kCommandAttach;
	else if(string == DAP::kEnableNetworking)
		return kCommandEnableNetworking;
	else if(string == DAP::kConfigurationDone)
		return kCommandConfigurationDone;
	else if(string == DAP::kThreads)
		return kCommandThreads;

	// events
	else if(string == DAP::kInitialized)
		return kEventInitialized;
	else if(string == DAP::kExited)
		return kEventExited;

	return kCommandUnknown;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_CLASS_ABSTRACT (DAPMessage, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

DAPMessage::DAPMessage (MessageType type, MessageCommand command, int threadId)
: type (type),
  command (command),
  threadId (threadId),
  sequenceNumber (IDebugMessage::kBroadcastThreadId)
{
	attributes.set (DAP::kIdBody, NEW Attributes (), Attributes::kOwns);
	attributes.set (DAP::kIdType, messageTypeToString (type));
	if(type == kTypeRequest || type == kTypeResponse)
		attributes.set (DAP::kIdCommand, messageCommandToString (command));
	else if(type == kTypeEvent)
		attributes.set (DAP::kIdEvent, messageCommandToString (command));

	if(threadId >= 0)
	{
		Attributes* arguments = NEW Attributes ();
		arguments->set (DAP::kIdThreadId, threadId);
		attributes.set (DAP::kIdArguments, arguments, Attributes::kOwns);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DAPMessage::setAttributes (const Attributes& attributes)
{
	this->attributes.copyFrom (attributes);
	updateProperties ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API DAPMessage::getRawData (String& data) const
{
	data = JsonUtils::toString (attributes);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API DAPMessage::setRawData (StringRef data)
{
	if(!JsonUtils::parseString (attributes, data))
	{
		ASSERT (false)
	}

	updateProperties ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DAPMessage::updateProperties ()
{
	Variant value;
	type = kTypeUnknown;
	if(attributes.getAttribute (value, DAP::kIdType))
		type = parseMessageType (value.asString ());

	command = kCommandUnknown;
	if(attributes.getAttribute (value, DAP::kIdCommand))
		command = parseMessageCommand (value.asString ());

	threadId = IDebugMessage::kBroadcastThreadId;
	if(attributes.getAttribute (value, DAP::kIdArguments))
	{
		Attributes* arguments = unknown_cast<Attributes> (value.asUnknown ());
		if(arguments)
		{
			if(arguments->getAttribute (value, DAP::kIdThreadId))
				threadId = value.asInt ();
		}
	}

	sequenceNumber = -1;
	if(attributes.getAttribute (value, DAP::kIdSeq))
		sequenceNumber = value.asInt ();
}

//************************************************************************************************
// DAPService
//************************************************************************************************

DEFINE_STRINGID_MEMBER_ (DAPService, kProtocolIdentifier, "dap")

//////////////////////////////////////////////////////////////////////////////////////////////////

DAPService::DAPService ()
: debuggableManager (nullptr),
  receiveThread (nullptr),
  receiveSocket (nullptr),
  sendSocket (nullptr),
  sequenceNumber (0),
  connected (false)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

DAPService::~DAPService ()
{
	System::GetSignalHandler ().cancelMessages (this);

	ASSERT (debuggableManager == nullptr)
	ASSERT (receiveSocket == nullptr)
	ASSERT (receiveThread == nullptr)
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API DAPService::startup (StringRef arg, IDebuggableManager* manager)
{
	ASSERT (receiveSocket == nullptr)
	if(receiveSocket)
		return false; // started already

	int port = 0;
	if(!arg.getIntValue (port))
		return false;

	CCL_WARN ("Starting DAP Service at port %d...\n", port)

	debuggableManager = manager;

	address.setIP (127, 0, 0, 1, port);

	receiveSocket = System::GetNetwork ().createSocket (Net::kInternet, Net::kStream, Net::kTCP);
	receiveSocket->bind (address);
	receiveSocket->listen (1);

	receiveThread = System::CreateNativeThread ({receiveThreadFunction, "Debug Server Receive Thread", this});
	receiveThread->setPriority (Threading::kPriorityBelowNormal);
	receiveThread->start ();

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API DAPService::shutdown ()
{
	CCL_WARN ("Shutting down DAP Service...\n", 0);

	if(receiveSocket)
		receiveSocket->disconnect ();

	disconnect ();
	while(sendSocket)
		System::ThreadSleep (10); // wait until sendSocket is released by receiveThread

	if(receiveThread)
	{
		receiveThread->terminate ();
		safe_release (receiveThread);
	}

	if(receiveSocket)
		safe_release (receiveSocket);

	debuggableManager = nullptr;
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API DAPService::sendMessage (const IDebugMessage& iMessage)
{
	if(DAPMessage* message = unknown_cast<DAPMessage> (&iMessage))
	{
		message->getAttributes ().set (DAP::kIdSeq, sequenceNumber);

		String data;
		message->getRawData (data);
		
		String header (Web::Meta::kContentLength);
		header.append (": ");
		header.appendIntValue (data.length ());
		header.append ("\r\n\r\n");
		data.prepend (header);

		if(sendMessage (data))
		{
			sequenceNumber++;
			return true;
		}
	}

	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IDebugMessage* CCL_API DAPService::createMessage (StringRef rawData)
{
	DAPMessage* m = NEW DAPMessage ();
	m->setRawData (rawData);
	return m;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool DAPService::sendMessage (StringRef response) const
{
	if(sendSocket == nullptr)
		return false;

	MutableCString cstring (response, Text::kUTF8);
	int nSent = sendSocket->send (cstring.str (), cstring.length ());
	ASSERT (nSent == cstring.length ())

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DAPService::disconnect ()
{
	DAPMessage message (DAPMessage::kTypeEvent, DAPMessage::kEventExited);
	Attributes* arguments = NEW Attributes ();
	message.getAttributes ().set (DAP::kIdBody, arguments, Attributes::kOwns);
	arguments->set (DAP::kIdExitCode, 1);

	sendMessage (message);

	if(debuggableManager)
		debuggableManager->onDisconnected ();

	connected = false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

/*static*/int CCL_API DAPService::receiveThreadFunction (void* arg)
{
	DAPService* me = static_cast<DAPService*>(arg);
	while(me->receiveSocket)
	{
		if(me->sendSocket = me->receiveSocket->accept ()) // blocking
		{
			const int kBufferSize = 64;
			char buffer[kBufferSize] = { };
			me->connected = true;

			while(me->connected)
			{
				if(me->sendSocket->isReadable ())
				{
					MutableCString message;
					while(me->connected && me->sendSocket->isReadable ())
					{
						int bytesRead = me->sendSocket->receive (buffer, kBufferSize); // blocking
						message.append (buffer, bytesRead);
					}

					ASSERT (!message.isEmpty ())

					if(me->connected)
					{
						String request;
						request.appendCString (Text::kUTF8, message);
						(NEW Message ("handleMessage", request))->post (me);
					}
				}
				System::ThreadSleep (50);
			}

			if(me->sendSocket)
			{
				me->sendSocket->disconnect ();
				safe_release (me->sendSocket);
			}
			CCL_PRINTLN ("Disconnected.");
		}
	}

	CCL_WARN ("Receive thread terminated.", 0);
	return 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API DAPService::notify (ISubject* subject, MessageRef msg)
{
	if(msg == "handleMessage")
	{
		handleMessage (msg.getArg (0).asString ());
		return;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool DAPService::handleMessage (StringRef s)
{
	bool success = true;

	int jsonStart = -1;
	int bracketCount = 0;
	int messageLength = s.length ();
	for(int i = 0; i < messageLength; i++)
	{
		if(s[i] == '{')
		{
			if(jsonStart == -1)
				jsonStart = i;

			bracketCount++;
		}
		else if(s[i] == '}')
		{
			bracketCount--;
			if(bracketCount < 0)
			{
				ASSERT (false)
				bracketCount = 0;
			}
			if(bracketCount == 0 && jsonStart >= 0)
			{
				success &= handleJSONMessage (s.subString (jsonStart, i - jsonStart + 1));
				jsonStart = -1;
			}
		}
	}

	return success;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool DAPService::handleJSONMessage (StringRef request)
{
	if(debuggableManager == nullptr)
		return false;

	AutoPtr<DAPMessage> m = unknown_cast<DAPMessage> (createMessage (request));
	if(m->getMessageType () == DAPMessage::kTypeUnknown)
	{
		ASSERT (false)
		return false;
	}

	if(m->getThreadId () == IDebugMessage::kBroadcastThreadId)
	{
		// Handle global requests like initialize
		if(m->getMessageType () == DAPMessage::kTypeRequest)
		{
			if(m->getMessageCommand () == DAPMessage::kCommandInitialize
				|| m->getMessageCommand () == DAPMessage::kCommandDisconnect
				|| m->getMessageCommand () == DAPMessage::kCommandAttach
				|| m->getMessageCommand () == DAPMessage::kCommandEnableNetworking
				|| m->getMessageCommand () == DAPMessage::kCommandConfigurationDone
				|| m->getMessageCommand () == DAPMessage::kCommandThreads)
			{
				DAPMessage message (DAPMessage::kTypeResponse, m->getMessageCommand ());
				message.getAttributes ().set (DAP::kIdRequestSeq, m->getSequenceNumber ());
				message.getAttributes ().set (DAP::kIdSuccess, true);

				Attributes* arguments = NEW Attributes ();
				message.getAttributes ().set (DAP::kIdBody, arguments, Attributes::kOwns);

				if(m->getMessageCommand () == DAPMessage::kCommandInitialize)
					arguments->set (DAP::kIdSupportsConfigurationDoneRequest, true);
				else if(m->getMessageCommand () == DAPMessage::kCommandDisconnect)
				{
					debuggableManager->receiveMessage (*m);
					sequenceNumber = 0;
					disconnect ();
				}
				else if(m->getMessageCommand () == DAPMessage::kCommandThreads)
				{
					const IContainer& debuggables = debuggableManager->getDebuggables ();
					AutoPtr<IUnknownIterator> iter = debuggables.createIterator ();
					while(!iter->done ())
					{
						UnknownPtr<IDebuggable> dbg = iter->nextUnknown ();
						if(dbg)
						{
							Attributes* threadInfo = NEW Attributes ();
							threadInfo->set (DAP::kIdId, dbg->getThreadId ());
							threadInfo->set (DAP::kIdName, dbg->getName ());
							arguments->queue (DAP::kIdThreads, threadInfo, Attributes::kOwns);
						}
					}
				}

				sendMessage (message);

				if(m->getMessageCommand () == DAPMessage::kCommandInitialize)
					sendMessage (DAPMessage (DAPMessage::kTypeEvent, DAPMessage::kEventInitialized));

				return true;
			}
		}
	}

	debuggableManager->receiveMessage (*m);
	return true;
}
