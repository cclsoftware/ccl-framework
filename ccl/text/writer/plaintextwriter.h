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
// Filename    : ccl/text/writer/plaintextwriter.h
// Description : Plain Text Writer
//
//************************************************************************************************

#ifndef _ccl_plaintextwriter_h
#define _ccl_plaintextwriter_h

#include "ccl/text/writer/textwriter.h"
#include "ccl/text/writer/textbuilder.h"

namespace CCL {

//************************************************************************************************
// PlainTextWriter
//************************************************************************************************

class PlainTextWriter: public TextWriter,
					   public IPlainTextWriter
{
public:
	PlainTextWriter ();

	// IPlainTextWriter
	ITextBuilder* CCL_API createPlainTextBuilder () override;
	void CCL_API setDocumentLineFormat (TextLineFormat lineFormat) override;
	tresult CCL_API beginDocument (IStream& stream, TextEncoding encoding = Text::kUnknownEncoding) override;
	tresult CCL_API endDocument () override;
	tresult CCL_API writeLine (StringRef text) override;

	CLASS_INTERFACE (IPlainTextWriter, TextWriter)

protected:
};

//************************************************************************************************
// PlainTextBuilder
//************************************************************************************************

class PlainTextBuilder: public TextBuilder
{
public:
	PlainTextBuilder (TextLineFormat lineFormat);

	// TextBuilder
	tresult CCL_API printChunk (String& result, const Text::Chunk& chunk) override;

private:
	int listLevel;

	static const String kListBulletString;
};

} // namespace CCL

#endif // _ccl_plaintextwriter_h
