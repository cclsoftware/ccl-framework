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
// Filename    : ccl/text/writer/htmlwriter.h
// Description : HTML Writer
//
//************************************************************************************************

#ifndef _ccl_htmlwriter_h
#define _ccl_htmlwriter_h

#include "ccl/text/writer/textwriter.h"
#include "ccl/text/writer/textbuilder.h"

#include "ccl/public/text/ihtmlwriter.h"

namespace CCL {

//************************************************************************************************
// HtmlWriter
//************************************************************************************************

class HtmlWriter: public SgmlWriter,
				  public IHtmlWriter
{
public:
	HtmlWriter ();

	typedef SgmlWriter SuperClass;

	// IHtmlWriter
	tresult CCL_API beginDocument (IStream& stream, TextEncoding encoding = Text::kUnknownEncoding) override;
	DEFINE_TEXTWRITER_METHODS (SuperClass)
	DEFINE_MARKUPWRITER_METHODS (SuperClass)
	DEFINE_SGMLWRITER_METHODS (SuperClass)
	ITextBuilder* CCL_API createHtmlBuilder () override;
	tresult CCL_API pushMetaElement (StringRef name, StringRef content, tbool isHttpEquiv) override;
	tresult CCL_API pushStyleElement (StringRef cssContent) override;
	tresult CCL_API writeHead (StringRef title) override;

	CLASS_INTERFACE (IHtmlWriter, SuperClass)

protected:
	struct MetaElement
	{
		String name;
		String content;
		bool httpEquiv;

		MetaElement (StringRef name = nullptr, 
					 StringRef content = nullptr,
					 bool httpEquiv = false)
		: name (name),
		  content (content),
		  httpEquiv (httpEquiv) 
		{}
	};

	Vector<MetaElement> metaElements;
	String styleElement;
};

//************************************************************************************************
// HtmlBuilder
//************************************************************************************************

class HtmlBuilder: public TextBuilder
{
public:
	HtmlBuilder (TextLineFormat lineFormat, MarkupEncoder* encoder);

	// TextBuilder
	tresult CCL_API printChunk (String& result, const Text::Chunk& chunk) override;
};

} // namespace CCL

#endif // _ccl_htmlwriter_h
