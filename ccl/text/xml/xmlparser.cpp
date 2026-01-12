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
// Filename    : ccl/text/xml/xmlparser.cpp
// Description : XML Parser
//
//************************************************************************************************

#define DEBUG_LOG 0//DEBUG
#define USE_ENTITIES 1

#include "ccl/text/xml/xmlparser.h"
#include "ccl/text/xml/xmlentities.h"
#include "ccl/text/xml/xmlstringdict.h"

#include "ccl/public/base/istream.h"
#include "ccl/public/base/variant.h"

#include "expat.h"

#define myParser ((XML_Parser)parser)

using namespace CCL;

static const String xmlErrorStringFormat = CCLSTR ("An XML reading error occurred in line %(1), column %(2): %(3)");

//************************************************************************************************
// Expat handlers
//************************************************************************************************

static void XmlStartElementHandler (void* userData, const XML_Char* name, const XML_Char** atts)
{
	#if DEBUG_LOG
	CCL_PRINTLN ((const uchar*)name)
	#endif

	XmlParser* parser = (XmlParser*)userData;
	IXmlContentHandler* handler = parser->getHandler ();
	if(parser->isAborted () || !handler)
		return;

	XmlStringDictionary attributes;
	if(atts) for(int i = 0; atts[i] != nullptr; i += 2)
	{
		String key ((const uchar*)atts[i]);
		String value ((const uchar*)atts[i+1]);

		#if DEBUG_LOG
		String output;
		output << "    " << key << "=" << value;
		CCL_PRINTLN (output)
		#endif

		attributes.appendEntry (key, value);
	}

	// Keep track of opened elements to avoid creating an additional
	// String from XML_Char* data in XmlEndElementHandler()
	Vector<String>& openElements = parser->getOpenElements ();
	openElements.add ((const uchar*) name);

	if(handler->startElement (openElements.last (), attributes) != kResultOk)
		parser->abort ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

static void XmlEndElementHandler (void* userData, const XML_Char* name)
{
	#if DEBUG_LOG
	CCL_PRINTLN((uchar*)name)
	#endif

	XmlParser* parser = (XmlParser*)userData;
	IXmlContentHandler* handler = parser->getHandler ();
	if(parser->isAborted () || !handler)
		return;

	// Retrieve the last opened element as stored in XmlStartElementHandler()
	Vector<String>& openElements = parser->getOpenElements ();
	ASSERT (openElements.last () == (const uchar*) name)

	if(handler->endElement (openElements.last ()) != kResultOk)
		parser->abort ();

	openElements.removeLast ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

static void XmlCharacterDataHandler (void* userData, const XML_Char* data, int len)
{
	XmlParser* parser = (XmlParser*)userData;
	IXmlContentHandler* handler = parser->getHandler ();
	if(parser->isAborted () || !handler)
		return;

	// data is not null terminated!
	if(handler->characterData ((const uchar*)data, len, parser->isReceivingCDATA ()) != kResultOk)
		parser->abort ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

static void XmlCDATABeginHandler (void* userData)
{
	XmlParser* parser = (XmlParser*)userData;
	parser->setReceivingCDATA (true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

static void XmlCDATAEndHandler (void* userData)
{
	XmlParser* parser = (XmlParser*)userData;
	parser->setReceivingCDATA (false);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

static void XmlProcessingInstructionHandler (void* userData, const XML_Char* target, const XML_Char* data)
{
	XmlParser* parser = (XmlParser*)userData;
	IXmlContentHandler* handler = parser->getHandler ();
	if(parser->isAborted () || !handler)
		return;

	String targetString ((const uchar*)target);
	String dataString ((const uchar*)data);

	if(handler->processingInstruction (targetString, dataString) != kResultOk)
		parser->abort ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

#if USE_ENTITIES
static int XmlExternalEntityHandler (XML_Parser parser,
									 const XML_Char* context, const XML_Char* base,
									 const XML_Char* systemId, const XML_Char* publicId)
{
	XML_Status status = XML_STATUS_ERROR;
	XML_Parser entityParser = XML_ExternalEntityParserCreate (parser, context, nullptr);
	if(entityParser)
	{
		static MutableCString builtInDTD = XmlEntities::makeBuiltInDTD ();
		static int length = builtInDTD.length ();

		status = XML_Parse (entityParser, builtInDTD, length, XML_TRUE);
		#if DEBUG
		if(status != XML_STATUS_OK)
		{
			XML_Error error = ::XML_GetErrorCode (entityParser);
		}
		#endif
		::XML_ParserFree (entityParser);
	}
	return status;
}
#endif

//////////////////////////////////////////////////////////////////////////////////////////////////

static void XmlStartNamespaceDeclHandler (void* userData, const XML_Char* prefix, const XML_Char* uri)
{
	XmlParser* parser = (XmlParser*)userData;
	IXmlContentHandler* handler = parser->getHandler ();
	if(parser->isAborted () || !handler)
		return;

	String prefixString ((uchar*)prefix);
	String uriString ((uchar*)uri);

	if(handler->startNamespace (prefixString, uriString) != kResultOk)
		parser->abort ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

static void XmlEndNamespaceDeclHandler (void* userData, const XML_Char* prefix)
{
	XmlParser* parser = (XmlParser*)userData;
	IXmlContentHandler* handler = parser->getHandler ();
	if(parser->isAborted () || !handler)
		return;

	String prefixString ((uchar*)prefix);
	if(handler->endNamespace (prefixString) != kResultOk)
		parser->abort ();
}

//************************************************************************************************
// XmlParser
//************************************************************************************************

XmlParser::XmlParser (bool parseNamespaces)
: handler (nullptr),
  aborted (false),
  silent (false),
  receivingCDATA (false)
{
	if(parseNamespaces)
		parser = ::XML_ParserCreateNS (nullptr, ':');
	else
		parser = ::XML_ParserCreate (nullptr);
	ASSERT (parser != nullptr)

	::XML_SetUserData (myParser, this);
	::XML_SetStartElementHandler (myParser, XmlStartElementHandler);
	::XML_SetEndElementHandler (myParser, XmlEndElementHandler);
	::XML_SetCharacterDataHandler(myParser, XmlCharacterDataHandler);
	::XML_SetCdataSectionHandler (myParser, XmlCDATABeginHandler, XmlCDATAEndHandler);
	::XML_SetProcessingInstructionHandler (myParser, XmlProcessingInstructionHandler);

	#if USE_ENTITIES
	::XML_UseForeignDTD (myParser, true);
	::XML_SetExternalEntityRefHandler (myParser, XmlExternalEntityHandler);
	::XML_SetParamEntityParsing (myParser, XML_PARAM_ENTITY_PARSING_UNLESS_STANDALONE);
	#endif

	if(parseNamespaces)
		::XML_SetNamespaceDeclHandler (myParser, XmlStartNamespaceDeclHandler, XmlEndNamespaceDeclHandler);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

XmlParser::~XmlParser ()
{
	if(parser)
		::XML_ParserFree (myParser);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool XmlParser::isAborted () const
{
	return aborted;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IXmlContentHandler* XmlParser::getHandler () const
{
	return handler;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API XmlParser::setHandler (IXmlContentHandler* _handler)
{
	handler = _handler;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API XmlParser::parse (IStream& stream)
{
	ASSERT (handler != nullptr)

	errorMessage.empty ();
	aborted = false;
	XML_Status status = XML_STATUS_OK;

	#define XML_BUFFER_SIZE 8192
	char buffer[XML_BUFFER_SIZE];

	while(!isAborted ())
	{
		int numRead = stream.read (buffer, XML_BUFFER_SIZE);
		if(numRead <= 0)
			break;

		status = ::XML_Parse (myParser, buffer, numRead, 0);
		if(status != XML_STATUS_OK)
			break;
	}
	#undef XML_BUFFER_SIZE

	if(isAborted ()) // aborted by handler
		return kResultFalse;

	if(status == XML_STATUS_OK)
		status = ::XML_Parse (myParser, nullptr, 0, 1);

	if(status != XML_STATUS_OK && errorMessage.isEmpty ())
	{
		XML_Error code = ::XML_GetErrorCode (myParser);
		int line = (int)::XML_GetCurrentLineNumber (myParser);
		int column = (int)::XML_GetCurrentColumnNumber (myParser);
		String info (::XML_ErrorString (code));
		Variant args[3];
		args[0] = line;
		args[1] = column;
		args[2] = info;
		errorMessage.appendFormat (xmlErrorStringFormat, args, 3);

		if(!silent)
		{
			// Note: CCL_WARN is not available in ccltext module!
			#if DEBUG
			Debugger::print (errorMessage);
			if(code == XML_ERROR_NO_ELEMENTS && line <= 1 && column == 0)
				Debugger::println ("");
			else
				Debugger::debugBreak ("\n");
			#endif
		}
	}
	return status == XML_STATUS_OK ? kResultOk : kResultFalse;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API XmlParser::abort ()
{
	aborted = true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API XmlParser::getCurrentLineNumber () const
{
	return (int)::XML_GetCurrentLineNumber (myParser);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int64 CCL_API XmlParser::getCurrentByteIndex () const
{
	return XML_GetCurrentByteIndex (myParser);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringRef CCL_API XmlParser::getErrorMessage () const
{
	return errorMessage;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API XmlParser::setErrorMessage (StringRef _errorMessage)
{
	errorMessage = _errorMessage;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API XmlParser::setSilentOnErrors (tbool state)
{
	silent = state != 0;
}
