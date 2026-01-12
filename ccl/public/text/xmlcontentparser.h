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
// Filename    : ccl/public/text/xmlcontentparser.h
// Description : XML Content Parser
//
//************************************************************************************************

#ifndef _ccl_xmlcontentparser_h
#define _ccl_xmlcontentparser_h

#include "ccl/public/base/unknown.h"
#include "ccl/public/text/ixmlparser.h"

namespace CCL {

//************************************************************************************************
// XmlContentParser
/** Basic XML content parser implementation.
	\ingroup ccl_text */
//************************************************************************************************

class XmlContentParser: public Unknown,
						public IXmlContentHandler
{
public:
	XmlContentParser (bool parseNamespaces = false);
	~XmlContentParser ();

	bool parse (IStream& stream);
	bool parse (UrlRef path);

	StringRef getErrorMessage () const;
	void setSilentOnErrors (tbool state);

	// IXmlContentHandler
	tresult CCL_API startElement (StringRef name, const IStringDictionary& attributes) override;
	tresult CCL_API endElement (StringRef name) override;
	tresult CCL_API characterData (const uchar* data, int length, tbool isCDATA) override;
	tresult CCL_API processingInstruction (StringRef target, StringRef data) override;
	tresult CCL_API startNamespace (StringRef prefix, StringRef uri) override;
	tresult CCL_API endNamespace (StringRef prefix) override;

	CLASS_INTERFACE (IXmlContentHandler, Unknown)

protected:
	IXmlParser* xmlParser;
};

//************************************************************************************************
// XmlLambdaParser
/**	\ingroup ccl_text */
//************************************************************************************************

template <typename StartLambda>
class XmlLambdaParser: public XmlContentParser
{
public:
	XmlLambdaParser (const StartLambda& startLambda)
	: startLambda (startLambda)
	{}

	// XmlContentParser
	tresult CCL_API startElement (StringRef name, const IStringDictionary& attributes) override
	{
		return startLambda (name, attributes);
	}

protected:
	StartLambda startLambda;
};

//************************************************************************************************
// XmlLambdaVisitor
/** Create parser with a lamda as startElement function
	\ingroup ccl_text */
//************************************************************************************************

template<typename StartLambda>
XmlLambdaParser<StartLambda> XmlLambdaVisitor (const StartLambda& start) { return XmlLambdaParser<StartLambda> (start); }

} // namespace CCL

#endif // _ccl_xmlcontentparser_h
