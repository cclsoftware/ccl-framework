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
// Filename    : ccl/extras/web/openai.h
// Description : OpenAI API Client
//
//************************************************************************************************

#ifndef _ccl_openai_h
#define _ccl_openai_h

#include "ccl/base/storage/attributes.h"
#include "ccl/base/collections/stringlist.h"

namespace CCL {
interface IStream;
interface IAsyncOperation;

namespace Web {
interface IXMLHttpRequest;

//************************************************************************************************
// OpenAI Namespace
//************************************************************************************************

namespace OpenAI
{
	//////////////////////////////////////////////////////////////////////////////////////////////
	// Classes
	//////////////////////////////////////////////////////////////////////////////////////////////

	class Client;
	class ResponseService;
	class ResponseCreateParams;
	class ResponseFormatJsonSchema;
	class StructuredResponse;
	class ResponseStream;

	//////////////////////////////////////////////////////////////////////////////////////////////
	// Constants
	//////////////////////////////////////////////////////////////////////////////////////////////

	const CString kModel = "model";
		const String kGpt_4_1_nano = "gpt-4.1-nano";
		const String kGpt_5_nano = "gpt-5-nano";

	const CString kTemperature = "temperature";
	const CString kTopP = "top_p";
	
	const CString kEffort = "effort";
		const String kLow = "low";
	
	const CString kReasoning = "reasoning";
	const CString kVerbosity = "verbosity";
	const CString kPreviousResponseId = "previous_response_id";
	const CString kText = "text";
	const CString kEnum = "enum";
	const CString kStore = "store";
	const CString kStream = "stream";
	const CString kInput = "input";

	const CString kRole = "role";
		const String kSystem = "system";
		const String kUser = "user";
		const String kDeveloper = "developer";
		const String kAssistant = "assistant";

	const CString kContent = "content";
	const CString kId = "id";
	const CString kOutput = "output";	
	const CString kProperties = "properties";
	const CString kName = "name";
	const CString kFormat = "format";
	
	const CString kType = "type";
		const String kMessage = "message";
		const String kOutputText = "output_text";
		const String kRefusal = "refusal";

	const CString kJsonSchemaType = "json_schema";
	const CString kRequired = "required";
	const CString kAdditionalProperties = "additionalProperties";
	const CString kStringType = "string";
	const CString kObjectType = "object";
	const CString kClassifierResponse = "classifier_response";
	const CString kSchema = "schema";
	const CString kFilters = "filters";
	const CString kTools = "tools";
	const CString kFileSearchTool = "file_search";
	const CString kWebSearchTool = "web_search";
	const CString kVectoreStoreIds = "vector_store_ids";
	const CString kAllowedDomains = "allowed_domains";

	const CString kError = "error";
	const CString kResponse = "response";
	const CString kDelta = "delta";
}

//************************************************************************************************
// OpenAI::Client
/**
	Lightweight Open AI API client class, inspired by the official Java SDK,
	primarily supporting a subset of the Responses API.
*/
//************************************************************************************************

class OpenAI::Client: public Object
{
public:
	DECLARE_CLASS_ABSTRACT (Client, Object)

	Client ();
	~Client ();
	
	PROPERTY_STRING (apiUrl, ApiUrl)
	PROPERTY_STRING (apiKey, ApiKey)
	
	ResponseService& getResponseService ();
	
	virtual void getResponseEndpoint (IUrl& url, bool stream) const;
	virtual void getAdditionalHeaders (IXMLHttpRequest& request) const;

protected:
	ResponseService* responseService;
};

//************************************************************************************************
// OpenAI::ResponseService
//************************************************************************************************

class OpenAI::ResponseService: public Object
{
public:
	DECLARE_CLASS_ABSTRACT (ResponseService, Object)

	ResponseService (Client& client);

	/**
		 Sends a request to the OpenAI Responses API endpoint and awaits the server reply.
	*/
	IAsyncOperation* create (const ResponseCreateParams& params);

	/**
		Sends a request to the OpenAI Responses API endpoint and returns a stream
		that yields response events as they are produced by the server.
	*/
	ResponseStream* stream (const ResponseCreateParams& params);

protected:
	Client& client;
};

//************************************************************************************************
// OpenAI::ResponseCreateParams
//************************************************************************************************

class OpenAI::ResponseCreateParams: public Object
{
public:
	DECLARE_CLASS (ResponseCreateParams, Object)

	ResponseCreateParams ();

	void setModel (StringRef model);
	void setTemperature (double temperature);
	void setTopP (double topP);
	void setReasoningEffort (StringRef effort);
	void setTextVerbosity (StringRef verbosity);
	void setPreviousResponseId (StringRef previousResposeId);
	void addInput (StringRef role, StringRef content);
	void setTextJsonSchema (StringID name, const ResponseFormatJsonSchema& schema);
	void addFileSearchTool (const StringList& vectorStores);
	void addWebSearchTool (const StringList& searchLinks);
	void setStore (bool value);
	void setStream (bool value);

	Attributes& getParams ();
	const Attributes& getParams () const;

private:
	Attributes params;
	
	Attributes& getTextAttributes ();
};

//************************************************************************************************
// OpenAI::ResponseFormatJsonSchema
//************************************************************************************************

class OpenAI::ResponseFormatJsonSchema: public Object
{
public:
	DECLARE_CLASS (ResponseFormatJsonSchema, Object)

	ResponseFormatJsonSchema ();

	void setStringAttributes (StringID name);
	void addStringEnum (StringID name, const StringList& enumValues);
	void setRequiredFields (const StringList& fields);
	void setAdditionalProperties (bool state);
	void setObjectAttributes (StringID name, const Attributes& properties, const StringList& requiredValues, bool hasAdditionalProperties);

	Attributes& getSchema ();
	const Attributes& getSchema () const;

private:
	Attributes schema;
	
	Attributes& getPropertiesAttributes ();
};

//************************************************************************************************
// OpenAI::StructuredResponse
//************************************************************************************************

class OpenAI::StructuredResponse: public Object
{
public:
	DECLARE_CLASS_ABSTRACT (ResponseFormatJsonSchema, Object)

	StructuredResponse (IStream& stream);
		
	String getOutputText () const;
	bool hasOutputRefusal () const;
	bool parseOutputTextJson (Attributes& result) const;

	String getResponseId () const;
	String getErrorText () const;
	
	const Attributes& getResponse () const;

protected:
	Attributes response;
};

//************************************************************************************************
// OpenAI::ResponseStream
//************************************************************************************************

class OpenAI::ResponseStream: public Object
{
public:
	DECLARE_CLASS_ABSTRACT (ResponseStream, Object)

	ResponseStream (IXMLHttpRequest* request, IStream* innerStream);
	~ResponseStream ();

	/**	Signaled whenever a new text delta is received.
		args[0]: delta text (String) */
	DECLARE_STRINGID_MEMBER (kResponseOutputTextDelta)

	/** Signaled when the streaming response finishes successfully. */
	DECLARE_STRINGID_MEMBER (kResponseCompleted)

	/** Signaled when a first successful event was received. */
	DECLARE_STRINGID_MEMBER (kResponseCreated)
	
	/** Signaled when an error is reported during streaming
		args[0]: error message (String) */
	DECLARE_STRINGID_MEMBER (kResponseError)

	IXMLHttpRequest* getRequest () const;
	
	// Object
	void CCL_API notify (ISubject* subject, MessageRef msg) override;

protected:	
	IXMLHttpRequest* request;
	IStream* innerStream;
	String accumulatedText;
	String responseId;
};

} // namespace Web
} // namespace CCL

#endif // _ccl_openai_h
