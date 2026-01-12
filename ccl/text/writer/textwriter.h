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
// Filename    : ccl/text/writer/textwriter.h
// Description : Text Writer
//
//************************************************************************************************

#ifndef _ccl_textwriter_h
#define _ccl_textwriter_h

#include "ccl/public/base/unknown.h"

#include "ccl/public/text/cclstring.h"
#include "ccl/public/text/itextwriter.h"
#include "ccl/public/text/istringdict.h"

#include "ccl/text/transform/textstreamer.h"

namespace CCL {

class MarkupEncoder;

//////////////////////////////////////////////////////////////////////////////////////////////////
// Text Writer Macros
//////////////////////////////////////////////////////////////////////////////////////////////////

#define DEFINE_TEXTWRITER_METHODS(Parent) \
void CCL_API setDocumentLineFormat (TextLineFormat lineFormat) override \
{ Parent::setDocumentLineFormat (lineFormat); } \
tresult CCL_API endDocument () override \
{ return Parent::endDocument (); } \
tresult CCL_API writeLine (StringRef text) override \
{ return Parent::writeLine (text); } \

//////////////////////////////////////////////////////////////////////////////////////////////////
// Markup Writer Macros
//////////////////////////////////////////////////////////////////////////////////////////////////

#define DEFINE_MARKUPWRITER_METHODS(Parent) \
tresult CCL_API writeMarkup (StringRef markup, tbool appendNewline = false) override \
{ return Parent::writeMarkup (markup, appendNewline); } \
tresult CCL_API encode (String& result, StringRef text) override \
{ return Parent::encode (result, text); } \
tresult CCL_API encode (MutableCString& result, StringRef text) override \
{ return Parent::encode (result, text); } \
tresult CCL_API decode (String& result, StringRef text) override \
{ return Parent::decode (result, text); }

//////////////////////////////////////////////////////////////////////////////////////////////////
// SGML Writer Macros
//////////////////////////////////////////////////////////////////////////////////////////////////

#define DEFINE_SGMLWRITER_METHODS(Parent) \
tresult CCL_API writeDocType (StringRef name, StringRef pubid, StringRef sysid, StringRef subset) override \
{ return Parent::writeDocType (name, pubid, sysid, subset); } \
void CCL_API setShouldIndent (tbool state) override \
{ return Parent::setShouldIndent (state); } \
tresult CCL_API startElement (StringRef name, const IStringDictionary* attributes = nullptr) override \
{ return Parent::startElement (name, attributes); } \
tresult CCL_API endElement (StringRef name) override \
{ return Parent::endElement (name); } \
tresult CCL_API writeElement (StringRef name, StringRef value) override \
{ return Parent::writeElement (name, value); } \
tresult CCL_API writeElement (StringRef name, const IStringDictionary* attributes = nullptr, StringRef value = nullptr) override \
{ return Parent::writeElement (name, attributes, value); } \
tresult CCL_API writeValue (StringRef value) override \
{ return Parent::writeValue (value); } \
tresult CCL_API writeComment (StringRef text) override \
{ return Parent::writeComment (text); } \
int CCL_API getCurrentDepth () const override \
{ return Parent::getCurrentDepth (); }

//************************************************************************************************
// TextWriter
//************************************************************************************************

class TextWriter: public Unknown,
				  public ITextWriter
{
public:
	TextWriter ();
	~TextWriter ();

	PROPERTY_VARIABLE (TextLineFormat, lineFormat, LineFormat)
	PROPERTY_BOOL (indentDisabled, IndentDisabled)
	
	// ITextWriter
	void CCL_API setDocumentLineFormat (TextLineFormat lineFormat) override;
	tresult CCL_API beginDocument (IStream& stream, TextEncoding encoding = Text::kUnknownEncoding) override;
	tresult CCL_API endDocument () override;
	tresult CCL_API writeLine (StringRef text) override;

	CLASS_INTERFACE (ITextWriter, Unknown)

protected:
	TextStreamer* streamer;

	static const String strIndent;
	static const String strSpace;

	void incIndent () { indent++; }
	void decIndent () { indent--; }
	int currentIndent () const { return indent; }
	String getIndent () const { return indentDisabled ? String () : String (strIndent, indent); }

private:
	int indent;
};

//************************************************************************************************
// MarkupWriter
//************************************************************************************************

class MarkupWriter: public TextWriter,
					public IMarkupWriter
{
public:
	MarkupWriter (MarkupEncoder* encoder);
	~MarkupWriter ();

	// IMarkupWriter
	void CCL_API setDocumentLineFormat (TextLineFormat lineFormat) override;
	tresult CCL_API beginDocument (IStream& stream, TextEncoding encoding = Text::kUnknownEncoding) override;
	tresult CCL_API endDocument () override;
	tresult CCL_API writeLine (StringRef text) override;
	tresult CCL_API writeMarkup (StringRef markup, tbool appendNewline = false) override;
	tresult CCL_API encode (String& result, StringRef text) override;
	tresult CCL_API encode (MutableCString& result, StringRef text) override;
	tresult CCL_API decode (String& result, StringRef text) override;

	CLASS_INTERFACE (IMarkupWriter, TextWriter)

protected:
	MarkupEncoder* encoder;

	String encodeEntities (StringRef text);
};

//************************************************************************************************
// SgmlWriter
//************************************************************************************************

class SgmlWriter: public MarkupWriter,
				  public ISgmlWriter
{
public:
	SgmlWriter (MarkupEncoder* encoder);

	enum Constants
	{
		kMaxLineLength = 100
	};

	// ISgmlWriter
	tresult CCL_API beginDocument (IStream& stream, TextEncoding encoding = Text::kUnknownEncoding) override;
	DEFINE_TEXTWRITER_METHODS (MarkupWriter)
	DEFINE_MARKUPWRITER_METHODS (MarkupWriter)
	tresult CCL_API writeDocType (StringRef name, StringRef pubid, StringRef sysid, StringRef subset) override;
	void CCL_API setShouldIndent (tbool state) override;
	tresult CCL_API startElement (StringRef name, const IStringDictionary* attributes = nullptr) override;
	tresult CCL_API endElement (StringRef name) override;
	tresult CCL_API writeElement (StringRef name, StringRef value) override;
	tresult CCL_API writeElement (StringRef name, const IStringDictionary* attributes = nullptr, StringRef value = nullptr) override;
	tresult CCL_API writeValue (StringRef value) override;
	tresult CCL_API writeComment (StringRef text) override;
	int CCL_API getCurrentDepth () const override;

	CLASS_INTERFACE (ISgmlWriter, MarkupWriter)

protected:
	bool writeAttributesString (const IStringDictionary* attributes, int offset);
};

} // namespace CCL

#endif // _ccl_textwriter_h
