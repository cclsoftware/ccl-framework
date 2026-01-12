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
// Filename    : ccl/public/text/xmlcontentparser.cpp
// Description : XML Content Parser
//
//************************************************************************************************

#include "ccl/public/text/xmlcontentparser.h"
#include "ccl/public/text/cclstring.h"

#include "ccl/public/system/inativefilesystem.h"
#include "ccl/public/systemservices.h"

using namespace CCL;

//************************************************************************************************
// XmlContentParser
//************************************************************************************************

XmlContentParser::XmlContentParser (bool parseNamespaces)
: xmlParser (System::CreateXmlParser (parseNamespaces))
{
	ASSERT (xmlParser != nullptr)
	if(xmlParser)
		xmlParser->setHandler (this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

XmlContentParser::~XmlContentParser ()
{
	if(xmlParser)
	{
		xmlParser->setHandler (nullptr);
		xmlParser->release ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool XmlContentParser::parse (IStream& stream)
{
	ASSERT (xmlParser != nullptr)
	if(!xmlParser)
		return false;

	bool result = xmlParser->parse (stream) == kResultOk;
	#if DEBUG
	if(!result)
	{
		CCL_PRINT (">>> XmlContentParser failed: ")
		CCL_PRINTLN (xmlParser->getErrorMessage ())
	}
	#endif
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool XmlContentParser::parse (UrlRef path)
{
	AutoPtr<IStream> stream = System::GetFileSystem ().openStream (path);
	if(stream == nullptr)
		return false;

	return parse (*stream);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringRef XmlContentParser::getErrorMessage () const
{
	ASSERT (xmlParser != nullptr)
	return xmlParser ? xmlParser->getErrorMessage () : String::kEmpty;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void XmlContentParser::setSilentOnErrors (tbool state)
{
	xmlParser->setSilentOnErrors (state);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API XmlContentParser::startElement (StringRef name, const IStringDictionary& attributes)
{
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API XmlContentParser::endElement (StringRef name)
{
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API XmlContentParser::characterData (const uchar* data, int length, tbool isCDATA)
{
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API XmlContentParser::processingInstruction (StringRef target, StringRef data)
{
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API XmlContentParser::startNamespace (StringRef prefix, StringRef uri)
{
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API XmlContentParser::endNamespace (StringRef prefix)
{
	return kResultOk;
}
