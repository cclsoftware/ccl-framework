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
// Filename    : ccl/public/text/itextwriter.h
// Description : Text Writer Interface
//
//************************************************************************************************

#ifndef _ccl_itextwriter_h
#define _ccl_itextwriter_h

#include "ccl/public/textservices.h"

namespace CCL {

class MutableCString;
interface ITextBuilder;

//************************************************************************************************
// ITextWriter
/**	\ingroup ccl_text */
//************************************************************************************************

interface ITextWriter: IUnknown
{
	/** Set line ending style. Needs to be called before beginDocument(). */
	virtual void CCL_API setDocumentLineFormat (TextLineFormat lineFormat) = 0;

	/** Begin new document. Supported encodings: "UTF-16" (default), "UTF-8", "ISO-8859-1", "US-ASCII". */
	virtual tresult CCL_API beginDocument (IStream& stream, TextEncoding encoding = Text::kUnknownEncoding) = 0;

	/** Finish writing to stream (see beginDocument) */
	virtual tresult CCL_API endDocument () = 0;

	/** Write line of text. */
	virtual tresult CCL_API writeLine (StringRef text) = 0;

	DECLARE_IID (ITextWriter)
};

DEFINE_IID (ITextWriter, 0xd7e06d6b, 0x9c01, 0x426b, 0xb8, 0x7d, 0x28, 0x75, 0xb4, 0xfd, 0xcc, 0xc1)

//************************************************************************************************
// IPlainTextWriter
/**	\ingroup ccl_text */
//************************************************************************************************

interface IPlainTextWriter: ITextWriter
{
	virtual ITextBuilder* CCL_API createPlainTextBuilder () = 0;

	DECLARE_IID (IPlainTextWriter)
};

DEFINE_IID (IPlainTextWriter, 0x921D1169, 0xE0AB, 0x413B, 0x9F, 0x45, 0x16, 0xA3, 0x66, 0xCC, 0x23, 0x15)

//************************************************************************************************
// IMarkupWriter
/**	\ingroup ccl_text */
//************************************************************************************************

interface IMarkupWriter: ITextWriter
{
	/** Write markup code directly. */
	virtual tresult CCL_API writeMarkup (StringRef markup, tbool appendNewline = false) = 0;

	/** Encode Unicode to markup entites. */
	virtual tresult CCL_API encode (String& result, StringRef text) = 0;

	/** Encode Unicode to markup entities for ASCII representatiton. */
	virtual tresult CCL_API encode (MutableCString& result, StringRef text) = 0;

	/** Decode markup entities to Unicode. */
	virtual tresult CCL_API decode (String& result, StringRef text) = 0;

	DECLARE_IID (IMarkupWriter)
};

DEFINE_IID (IMarkupWriter, 0x9ae4e410, 0x2ff0, 0x4c31, 0xb5, 0x12, 0x65, 0x81, 0x3c, 0x7e, 0x52, 0xd4)

//************************************************************************************************
// ISgmlWriter
/**	\ingroup ccl_text */
//************************************************************************************************

interface ISgmlWriter: IMarkupWriter
{	
	/** Write DOCTYPE declaration. */
	virtual tresult CCL_API writeDocType (StringRef name, StringRef pubid, StringRef sysid, StringRef subset) = 0;
	
	/** Set whether to indent elements (default is on). */
	virtual void CCL_API setShouldIndent (tbool state) = 0;

	/** Start element. Attributes are optional. */
	virtual tresult CCL_API startElement (StringRef name, const IStringDictionary* attributes = nullptr) = 0;

	/** End element. */
	virtual tresult CCL_API endElement (StringRef name) = 0;

	/** Write simple element with text and no attributes. */
	virtual tresult CCL_API writeElement (StringRef name, StringRef value) = 0;

	/** Write simple element with optional attributes and text. */
	virtual tresult CCL_API writeElement (StringRef name, const IStringDictionary* attributes = nullptr, StringRef value = nullptr) = 0;

	/** Write value inside an element. */
	virtual tresult CCL_API writeValue (StringRef value) = 0;

	/** Write comment. */
	virtual tresult CCL_API writeComment (StringRef text) = 0;

	/** Get current depth of nested elements. */
	virtual int CCL_API getCurrentDepth () const = 0;

	DECLARE_IID (ISgmlWriter)
};

DEFINE_IID (ISgmlWriter, 0xf3640ac2, 0x2323, 0x42a8, 0xa8, 0x34, 0xeb, 0xb0, 0x36, 0xfb, 0xbf, 0x38)

} // namespace CCL

#endif // _ccl_itextwriter_h
