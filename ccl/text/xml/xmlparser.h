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
// Filename    : ccl/text/xml/xmlparser.h
// Description : XML Parser
//
//************************************************************************************************

#ifndef _ccltext_xmlparser_h
#define _ccltext_xmlparser_h

#include "ccl/public/base/unknown.h"
#include "ccl/public/collections/vector.h"
#include "ccl/public/text/ixmlparser.h"
#include "ccl/public/text/cclstring.h"

namespace CCL {

//************************************************************************************************
// XmlParser
//************************************************************************************************

class XmlParser: public Unknown,
				 public IXmlParser
{
public:
	XmlParser (bool parseNamespaces);
	~XmlParser ();

	bool isAborted () const;
	IXmlContentHandler* getHandler () const;
	Vector<String>& getOpenElements () { return openElements; }
	PROPERTY_BOOL (receivingCDATA, ReceivingCDATA)

	// IXmlParser
	void CCL_API setHandler (IXmlContentHandler* handler) override;
	tresult CCL_API parse (IStream& stream) override;
	void CCL_API abort () override;
	int CCL_API getCurrentLineNumber () const override;
	int64 CCL_API getCurrentByteIndex () const override;
	StringRef CCL_API getErrorMessage () const override;
	void CCL_API setErrorMessage (StringRef errorMessage) override;
	void CCL_API setSilentOnErrors (tbool state) override;

	CLASS_INTERFACE (IXmlParser, Unknown)

protected:
	void* parser;
	IXmlContentHandler* handler;
	Vector<String> openElements;
	String errorMessage;
	bool aborted;
	bool silent;
};

} // namespace CCL

#endif // _ccltext_xmlparser_h
