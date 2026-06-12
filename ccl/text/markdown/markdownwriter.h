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
// Filename    : ccl/text/markdown/markdownwriter.h
// Description : Markdown Writer
//
//************************************************************************************************

#ifndef _ccl_markdownwriter_h
#define _ccl_markdownwriter_h

#include "ccl/text/writer/textwriter.h"
#include "ccl/text/writer/textbuilder.h"

#include "ccl/public/text/imarkdownwriter.h"

namespace CCL {

//************************************************************************************************
// MarkdownWriter
//************************************************************************************************

class MarkdownWriter: public MarkupWriter,
					  public IMarkdownWriter
{
public:
	MarkdownWriter ();

	// IMarkdownWriter
	tresult CCL_API beginDocument (IStream& stream, TextEncoding encoding = Text::kUnknownEncoding) override;
	DEFINE_TEXTWRITER_METHODS (SuperClass)
	ITextBuilder* CCL_API createTextBuilder () override;
	DEFINE_MARKUPWRITER_METHODS (SuperClass)

	CLASS_INTERFACE (IMarkdownWriter, MarkupWriter)

	using SuperClass = MarkupWriter;
};

//************************************************************************************************
// MarkdownBuilder
//************************************************************************************************

class MarkdownBuilder: public TextBuilder
{
public:
	MarkdownBuilder (TextLineFormat lineFormat, MarkupEncoder* encoder);

	// TextBuilder
	tresult CCL_API printFragment (String& result, const TextFragment& fragment) override;

private:
	Vector<int> listCounters;
};

} // namespace CCL

#endif // _ccl_markdownwriter_h
