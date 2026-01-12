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
// Filename    : ccl/public/gui/graphics/markuptags.h
// Description : Markup Tags
//
//************************************************************************************************

#ifndef _ccl_markuptags_h
#define _ccl_markuptags_h

#include "ccl/public/text/cclstring.h"

namespace CCL {

//************************************************************************************************
// MarkupTags
//************************************************************************************************

namespace MarkupTags
{
	const String kBold ("b");
	const String kItalic ("i");
	const String kUnderline ("u");
	const String kSuperscript ("sup");
	const String kSubscript ("sub");
	const String kStyleColor ("style color");
	const String kColor ("color");
	const String kStyleSize ("style size");
	const String kSize ("size");
}

//************************************************************************************************
// MarkupBuilder
//************************************************************************************************

class MarkupBuilder
{
public:
	MarkupBuilder& append (StringRef content)
	{
		markup << content;
		return *this;
	}

	MarkupBuilder& appendTag (StringRef tag, StringRef paramValue, StringRef content) 
	{
		markup << "[" << tag;
		if(!paramValue.isEmpty ())
			markup << "=" << paramValue;
		markup << "]";
		markup << content;
		markup << "[/" << tag << "]";
		return *this; 
	}

	MarkupBuilder& bold (StringRef content)
	{
		return appendTag (MarkupTags::kBold, String::kEmpty, content);
	}

	MarkupBuilder& italic (StringRef content)
	{
		return appendTag (MarkupTags::kItalic, String::kEmpty, content);
	}

	MarkupBuilder& underline (StringRef content)
	{
		return appendTag (MarkupTags::kUnderline, String::kEmpty, content);
	}

	MarkupBuilder& superscript (StringRef content)
	{
		return appendTag (MarkupTags::kSuperscript, String::kEmpty, content);
	}

	MarkupBuilder& subscript (StringRef content)
	{
		return appendTag (MarkupTags::kSubscript, String::kEmpty, content);
	}

	String& asString () { return markup; }
	StringRef asString () const { return markup; }
	operator StringRef () const { return markup; }

protected:
	String markup;
};

} // namespace CCL

#endif // _ccl_markuptags_h
