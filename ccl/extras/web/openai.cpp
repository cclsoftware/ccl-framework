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
// Filename    : ccl/extras/web/openai.cpp
// Description : OpenAI API Client
//
//************************************************************************************************

#include "ccl/extras/web/openai.h"
#include "ccl/extras/web/webxhroperation.h"
#include "ccl/extras/web/webresponsestream.h"

#include "ccl/base/storage/jsonarchive.h"
#include "ccl/base/storage/url.h"
#include "ccl/base/message.h"

#include "ccl/public/base/istream.h"

#include "ccl/public/network/web/httpstatus.h"
#include "ccl/public/network/web/iwebrequest.h"
#include "ccl/public/network/web/ixmlhttprequest.h"
#include "ccl/public/network/web/iwebcredentials.h"

#include "ccl/public/plugservices.h"

namespace CCL {
namespace Web {

namespace OpenAI
{
	static const String kAPIUrl ("https://api.openai.com/v1");
}

} // namespace Web
} // namespace CCL

using namespace CCL;
using namespace Web;
using namespace OpenAI;

//************************************************************************************************
// OpenAI::Client
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (Client, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

Client::Client ()
: responseService (nullptr)
{
	apiUrl = kAPIUrl;
	responseService = NEW ResponseService (*this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Client::~Client ()
{
	responseService->release ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ResponseService& Client::getResponseService ()
{
	return *responseService;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Client::getResponseEndpoint (IUrl& url, bool stream) const
{
	url.setUrl (getApiUrl ());
	url.descend ("responses");
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Client::getAdditionalHeaders (IXMLHttpRequest& request) const
{}

//************************************************************************************************
// OpenAI::ResponseService
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (ResponseService, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

ResponseService::ResponseService (Client& client)
: client (client)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

IAsyncOperation* ResponseService::create (const ResponseCreateParams& params)
{
	Url url;
	client.getResponseEndpoint (url, false);
	
	auto* httpPostRequest = ccl_new<IXMLHttpRequest> (ClassID::XMLHttpRequest);
	String authType = !client.getApiKey ().isEmpty () ? String (Meta::kBearer) : String::kEmpty;
	httpPostRequest->open (HTTP::kPOST, url, true, String::kEmpty, client.getApiKey (), authType);
	httpPostRequest->setRequestHeader (Meta::kContentType, JsonArchive::kMimeType);
	client.getAdditionalHeaders (*httpPostRequest);

	AutoPtr<IStream> jsonData = JsonUtils::serialize (params.getParams ());
	httpPostRequest->send (jsonData);
	
	return NEW AsyncXHROperation (httpPostRequest);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ResponseStream* ResponseService::stream (const ResponseCreateParams& params)
{
	Url url;
	client.getResponseEndpoint (url, true);
		
	AutoPtr<IXMLHttpRequest> httpPostRequest = ccl_new<IXMLHttpRequest> (ClassID::XMLHttpRequest);
	String authType = !client.getApiKey ().isEmpty () ? String (Meta::kBearer) : String::kEmpty;
	httpPostRequest->open (HTTP::kPOST, url, true, String::kEmpty, client.getApiKey (), authType);
	httpPostRequest->setRequestHeader (Meta::kContentType, JsonArchive::kMimeType);
	client.getAdditionalHeaders (*httpPostRequest);

	AutoPtr<WebResponseStream> responseStream = NEW WebMixedResponseStream; // response can be SSE or JSON
	httpPostRequest->setResponseStream (responseStream);
	AutoPtr<IStream> jsonData = JsonUtils::serialize (params.getParams ());
	httpPostRequest->send (jsonData);

	return NEW ResponseStream (httpPostRequest, responseStream);
}

//************************************************************************************************
// OpenAI::StructuredResponse
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (StructuredResponse, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

StructuredResponse::StructuredResponse (IStream& stream)
{
	JsonUtils::parse (response, stream);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

String StructuredResponse::getOutputText () const
{
	IterForEach (response.newQueueIterator (kOutput, ccl_typeid<Attributes> ()), Attributes, outputAttributes)
		if(outputAttributes)
		{
			String outputType = outputAttributes->getString (kType);
			if(outputType == kMessage)
			{
				IterForEach (outputAttributes->newQueueIterator (kContent, ccl_typeid<Attributes> ()), Attributes, contentAttributes)
					if(contentAttributes)
					{
						String contentType = contentAttributes->getString (kType);
						if(contentType == kOutputText)
							return contentAttributes->getVariant (kText);
					}
				EndFor
			}
		}
	EndFor
	
	return String::kEmpty;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool StructuredResponse::hasOutputRefusal () const
{
	IterForEach (response.newQueueIterator (kOutput, ccl_typeid<Attributes> ()), Attributes, outputAttributes)
		if(outputAttributes)
		{
			String outputType = outputAttributes->getString (kType);
			if(outputType == kMessage)
			{
				IterForEach (outputAttributes->newQueueIterator (kContent, ccl_typeid<Attributes> ()), Attributes, contentAttributes)
					if(contentAttributes)
					{
						String contentType = contentAttributes->getString (kType);
						if(contentType == kRefusal)
							return true;
					}
				EndFor
			}
		}
	EndFor
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool StructuredResponse::parseOutputTextJson (Attributes& result) const
{
	return JsonUtils::parseString (result, getOutputText ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

String StructuredResponse::getResponseId () const
{
	return response.getString (kId);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

String StructuredResponse::getErrorText () const
{
	Attributes* error = response.getAttributes (OpenAI::kError);
	if(error)
		return error->getString (MutableCString (OpenAI::kMessage));
	else
		return String::kEmpty;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const Attributes& StructuredResponse::getResponse () const
{
	return response;
}

//************************************************************************************************
// OpenAI::ResponseStream
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (ResponseStream, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_STRINGID_MEMBER_ (ResponseStream, kResponseOutputTextDelta, "response.output_text.delta")
DEFINE_STRINGID_MEMBER_ (ResponseStream, kResponseCompleted, "response.completed")
DEFINE_STRINGID_MEMBER_ (ResponseStream, kResponseCreated, "response.created")
DEFINE_STRINGID_MEMBER_ (ResponseStream, kResponseError, "response.error")

//////////////////////////////////////////////////////////////////////////////////////////////////

ResponseStream::ResponseStream (IXMLHttpRequest* request, IStream* innerStream)
: request (request),
  innerStream (innerStream)
{
	ASSERT (request != nullptr && innerStream != nullptr)
	
	request->retain ();
	innerStream->retain ();
	ISubject::addObserver (innerStream, this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ResponseStream::~ResponseStream ()
{
	ISubject::removeObserver (innerStream, this);
	innerStream->release ();
	request->release ();

	cancelSignals ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IXMLHttpRequest* ResponseStream::getRequest () const
{
	return request;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API ResponseStream::notify (ISubject* subject, MessageRef msg)
{
	if(msg == WebSseStream::kOnMessage)
	{
		MutableCString currentEvent = msg[0].asString ();
		String currentData = msg[1].asString ();
		
		if(currentEvent == kResponseOutputTextDelta)
		{
			Attributes result;
			JsonUtils::parseString (result, currentData);
			String delta = result.getString (kDelta);
						
			deferSignal (NEW Message (kResponseOutputTextDelta, delta), kSignalQueueAddAlways);
			accumulatedText.append (delta);
		}
		else if(currentEvent == kResponseCompleted)
		{
			deferSignal (NEW Message (kResponseCompleted, accumulatedText));
		}
		else if(currentEvent == kResponseCreated)
		{
			Attributes response;
			JsonUtils::parseString (response, currentData);

			if(Attributes* contentAttributes = response.getAttributes (kResponse))
			{
				String responseId = contentAttributes->getString (kId);
				deferSignal (NEW Message (kResponseCreated, responseId));
			}
		}
		else if(currentEvent == kResponseError)
		{
			Attributes errorInfo;
			JsonUtils::parseString (errorInfo, currentData);

			if(Attributes* error = errorInfo.getAttributes (kError))
			{
				String errorMessage = error->getString (MutableCString (kMessage));
				deferSignal (NEW Message (kResponseError, errorMessage));
			}
			else
				deferSignal (NEW Message (kResponseError));
		}
	}
	else if(msg == WebMixedResponseStream::kResponseCompleted)
	{
		bool eventStream = msg[0].asBool ();
		if(eventStream == false)
		{
			UnknownPtr<IMemoryStream> responseData (msg[1].asUnknown ());
			if(responseData)
			{
				Attributes responseAttributes;
				if(JsonUtils::parse (responseAttributes, *responseData))
				{
					if(Attributes* error = responseAttributes.getAttributes (kError))
					{
						String errorMessage = error->getString (MutableCString (kMessage));
						deferSignal (NEW Message (kResponseError, errorMessage));
					}
				}
			}
		}
	}
}

//************************************************************************************************
// OpenAI::ResponseCreateParams
//************************************************************************************************

DEFINE_CLASS_HIDDEN (ResponseCreateParams, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

ResponseCreateParams::ResponseCreateParams ()
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ResponseCreateParams::setModel (StringRef model)
{
	params.set (kModel, model);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ResponseCreateParams::setTemperature (double temperature)
{
	params.set (kTemperature, temperature);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ResponseCreateParams::setTopP (double topP)
{
	params.set (kTopP, topP);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ResponseCreateParams::setReasoningEffort (StringRef effort)
{
	Attributes reasoningEffort;
	reasoningEffort.set (kEffort, effort);
	params.set (kReasoning, reasoningEffort.asUnknown (), Attributes::kTemp);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ResponseCreateParams::setTextVerbosity (StringRef verbosity)
{
	getTextAttributes ().set (kVerbosity, verbosity);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ResponseCreateParams::setPreviousResponseId (StringRef previousResposeId)
{
	params.set (kPreviousResponseId, previousResposeId);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ResponseCreateParams::addInput (StringRef role, StringRef content)
{
	Attributes inputElement;
	inputElement.set (kRole, role);
	inputElement.set (kContent, content);
	
	params.queue (kInput, &inputElement, Attributes::kTemp);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ResponseCreateParams::setTextJsonSchema (StringID name, const ResponseFormatJsonSchema& schema)
{
	Attributes format;
	format.set (kName, name);
	format.set (kType, kJsonSchemaType);
	
	format.setAttribute (kSchema, const_cast<Attributes&> (schema.getSchema ()).asUnknown (), Attributes::kTemp);
	
	getTextAttributes ().setAttribute (kFormat, format.asUnknown (), Attributes::kTemp);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ResponseCreateParams::addFileSearchTool (const StringList& vectorStores)
{
	Attributes fileSearchTool;
	fileSearchTool.set (kType, kFileSearchTool);
	AttributeQueue vectorStoreIdsQueue;
	for(auto vectoreStore: vectorStores)
	{
		vectorStoreIdsQueue.addValue (*vectoreStore);
	}

	fileSearchTool.set (kVectoreStoreIds, vectorStoreIdsQueue.asUnknown (), Attributes::kTemp);
	
	params.queue (kTools, &fileSearchTool, Attributes::kTemp);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ResponseCreateParams::addWebSearchTool (const StringList& searchLinks)
{
	Attributes webSearchTool;
	webSearchTool.set (kType, kWebSearchTool);
	
	Attributes domains;
	AttributeQueue webSearchLinks;
	for(auto link: searchLinks)
	{
		webSearchLinks.addValue (*link);
	}

	domains.set (kAllowedDomains, webSearchLinks.asUnknown (), Attributes::kTemp);
	webSearchTool.setAttribute (kFilters, domains.asUnknown (), Attributes::kTemp);
	
	params.queue (kTools, &webSearchTool, Attributes::kTemp);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ResponseCreateParams::setStore (bool value)
{
	params.set (kStore, value, Variant::kBoolFormat);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ResponseCreateParams::setStream (bool value)
{
	params.set (kStream, value, Variant::kBoolFormat);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Attributes& ResponseCreateParams::getParams ()
{
	return params;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const Attributes& ResponseCreateParams::getParams () const
{
	return params;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Attributes& ResponseCreateParams::getTextAttributes ()
{
	Attributes* textAttributes = params.getAttributes (kText);
	if(!textAttributes)
		params.set (kText, textAttributes = NEW Attributes, Attributes::kOwns);
	
	return *textAttributes;
}

//************************************************************************************************
// OpenAI::ResponseFormatJsonSchema
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (ResponseFormatJsonSchema, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

ResponseFormatJsonSchema::ResponseFormatJsonSchema ()
{
	schema.set (kType, kObjectType);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ResponseFormatJsonSchema::setStringAttributes (StringID name)
{
	Attributes stringAttr;
	stringAttr.set (kType, kStringType);
	getPropertiesAttributes ().setAttribute (name, stringAttr.asUnknown (), Attributes::kTemp);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ResponseFormatJsonSchema::addStringEnum (StringID name, const StringList& enumValues)
{
	Attributes enumAttr;
	enumAttr.set (kType, kStringType);

	AttributeQueue enumQueue;
	for(auto enumValue: enumValues)
	{
		enumQueue.addValue (*enumValue);
	}

	enumAttr.setAttribute (kEnum, enumQueue.asUnknown(), Attributes::kTemp);
	getPropertiesAttributes ().setAttribute (name, enumAttr.asUnknown (), Attributes::kTemp);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ResponseFormatJsonSchema::setRequiredFields (const StringList& fields)
{
	AttributeQueue fieldsQueue;
	for(auto field: fields)
	{
		fieldsQueue.addValue (*field);
	}

	schema.setAttribute (kRequired, fieldsQueue.asUnknown(), Attributes::kTemp);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ResponseFormatJsonSchema::setAdditionalProperties (bool state)
{
	schema.setAttribute (kAdditionalProperties, Variant (state, Variant::kBoolFormat), Attributes::kTemp);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ResponseFormatJsonSchema::setObjectAttributes (StringID name, const Attributes& properties, const StringList& requiredValues, bool hasAdditionalProperties)
{
	Attributes objAttr;
	objAttr.set (kType, kObjectType);
	
	objAttr.setAttribute (kProperties, const_cast<Attributes&> (properties).asUnknown (), Attributes::kTemp);
	
	AttributeQueue requiredQueue;
	for(auto requiredValue: requiredValues)
	{
		requiredQueue.addValue (*requiredValue);
	}
	objAttr.setAttribute (kRequired, requiredQueue.asUnknown (), Attributes::kTemp);
	
	objAttr.setAttribute (kAdditionalProperties, Variant (hasAdditionalProperties, Variant::kBoolFormat), Attributes::kTemp);
	
	getPropertiesAttributes ().setAttribute (name, objAttr.asUnknown (), Attributes::kTemp);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Attributes& ResponseFormatJsonSchema::getSchema ()
{
	return schema;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const Attributes& ResponseFormatJsonSchema::getSchema () const
{
	return schema;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Attributes& ResponseFormatJsonSchema::getPropertiesAttributes ()
{
	Attributes* properties = schema.getAttributes (kProperties);
	if(!properties)
		schema.set (kProperties, properties = NEW Attributes, Attributes::kOwns);
	
	return *properties;
}

