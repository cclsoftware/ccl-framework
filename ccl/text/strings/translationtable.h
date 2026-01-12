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
// Filename    : ccl/text/strings/translationtable.h
// Description : Translation Table
//
//************************************************************************************************

#ifndef _ccl_translationtable_h
#define _ccl_translationtable_h

#include "ccl/text/strings/stringtable.h"

#include "ccl/public/text/cstring.h"
#include "ccl/public/text/itranslationtable.h"
#include "ccl/public/collections/vector.h"

namespace CCL {

//************************************************************************************************
// TranslatedText
//************************************************************************************************

struct TranslatedText
{
	MutableCString scope;
	String text;

	TranslatedText (StringID scope = nullptr, StringRef text = nullptr)
	: scope (scope),
	  text (text)
	{}
};

//************************************************************************************************
// TranslationEntry
//************************************************************************************************

struct TranslationEntry: StringEntry
{
	Vector<TranslatedText> translations;
	
	TranslationEntry (const char* cString = nullptr, OwnerHint hint = kNoCopy)
	: StringEntry (cString, hint)
	{}

	void addText (const TranslatedText& text);
	StringRef getText (StringID scope) const;
};

//************************************************************************************************
// TranslationTable
//************************************************************************************************

class TranslationTable: public Unknown,
						public ITranslationTable
{
public:
	// ITranslationTable
	tresult CCL_API addVariable (StringID name, StringRef text) override;
	tresult CCL_API addString (StringID scope, StringID key, StringRef text) override;
	tresult CCL_API addStringWithUnicodeKey (StringID scope, StringRef unicodeKey, StringRef text) override;
	tresult CCL_API loadStrings (IStream& stream, ITranslationTableHook* hook = nullptr) override;
	tresult CCL_API getString (String& result, StringID scope, StringID key) const override;
	tresult CCL_API getStringWithUnicodeKey (String& result, StringID scope, StringRef unicodeKey) const override;
	tresult CCL_API saveStrings (IStream& stream, tbool isTemplate) const override;

	CLASS_INTERFACE (ITranslationTable, Unknown)

protected:
	StringTable strings;
	StringTable variables;

	void resolveVariables (String& result, StringRef text) const;
	String getVariable (StringID name) const;
};

} // namespace CCL

#endif // _ccl_translationtable_h
