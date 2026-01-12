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
// Filename    : ccl/public/text/ixmlparser.h
// Description : XML Parser Interface
//
//************************************************************************************************

#ifndef _ccl_ixmlparser_h
#define _ccl_ixmlparser_h

#include "ccl/public/textservices.h"

namespace CCL {

interface IXmlContentHandler;

//************************************************************************************************
// IXmlParser
/**	Xml Parser - created via System::CreateXmlParser();
    \ingroup ccl_text */
//************************************************************************************************

interface IXmlParser: IUnknown
{
	/** Init content handler. */
	virtual void CCL_API setHandler (IXmlContentHandler* handler) = 0;

	/** Parse XML data stream. */
	virtual tresult CCL_API parse (IStream& stream) =  0;

	/** Abort parsing, can be called from content handler. */
	virtual void CCL_API abort () = 0;

	/** Returns current line number, can be called from content handler. */
	virtual int CCL_API getCurrentLineNumber () const = 0;

	/** Returns the current byte index, can be called from content handler. */
	virtual int64 CCL_API getCurrentByteIndex () const = 0;

	/** Returns last error message or empty string. */
	virtual StringRef CCL_API getErrorMessage () const = 0;

	/** Set error message from content handler. */
	virtual void CCL_API setErrorMessage (StringRef errorMessage) = 0;

	/** Suppress error reporting / break in debug build (e.g. when failure is likely). */
	virtual void CCL_API setSilentOnErrors (tbool state) = 0;

	DECLARE_IID (IXmlParser)
};

DEFINE_IID (IXmlParser, 0xe66ff5b1, 0x99c8, 0x49c2, 0x89, 0x14, 0x6e, 0x79, 0x1e, 0xa5, 0xae, 0x3c)

//************************************************************************************************
// IXmlContentHandler
/** \ingroup ccl_text */
//************************************************************************************************

interface IXmlContentHandler: IUnknown
{
	/** Notification of the beginning of an element. A corresponding endElement call will follow. */
	virtual tresult CCL_API startElement (StringRef name, const IStringDictionary& attributes) = 0;

	/** Notification of the end of an element. */
	virtual tresult CCL_API endElement (StringRef name) = 0;

	/** Notification of character data. Can be called several times with small chunks of a single entity. */
	virtual tresult CCL_API characterData (const uchar* data, int length, tbool isCDATA) = 0;

	/** Notification of processing instruction (<?target data?>). */
	virtual tresult CCL_API processingInstruction (StringRef target, StringRef data) = 0;

	/** Called once for each namespace declaration (if namespace processing is enabled). */
	virtual tresult CCL_API startNamespace (StringRef prefix, StringRef uri) = 0;

	/** Called once for each namespace declaration (if namespace processing is enabled). */
	virtual tresult CCL_API endNamespace (StringRef prefix) = 0;

	DECLARE_IID (IXmlContentHandler)
};

DEFINE_IID (IXmlContentHandler, 0x9982803e, 0x592b, 0x480b, 0x9a, 0xc4, 0x4f, 0x4, 0xc9, 0x8, 0xf4, 0x6e)

} // namespace CCL

#endif // _ccl_ixmlparser_h
