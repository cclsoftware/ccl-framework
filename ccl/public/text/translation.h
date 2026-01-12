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
// Filename    : ccl/public/text/translation.h
// Description : String Translation
//
//************************************************************************************************

#ifndef _ccl_translation_h
#define _ccl_translation_h

#include "ccl/public/text/cclstring.h"
#include "ccl/public/base/cclmacros.h"

#undef BEGIN_XSTRINGS
#undef END_XSTRINGS
#undef XSTRING
#undef XSTR
#undef TRANSLATE

namespace CCL {

interface ITranslationTable;

/** \defgroup translation Translation 
    \ingroup ccl_text 
@{ */

//////////////////////////////////////////////////////////////////////////////////////////////////
// Translation macros
//////////////////////////////////////////////////////////////////////////////////////////////////

/** \addtogroup translation 

	Examples:

	@code

	BEGIN_XSTRINGS ("MyScope")
		XSTRING (ThisIsATest, "This is a test")
	END_XSTRINGS

	String translatedFromConstant = XSTR (ThisIsATest);
	
	String stringToBeTranslated;
	String translatedOnTheFly = TRANSLATE (stringToBeTranslated);
	
	const char* key = XSTR_REF(ThisIsATest).getKey ();

	@endcode
*/

/** Begin scope of translated string literals. */
#define BEGIN_XSTRINGS(name) \
namespace XStrings { \
static const CCL::LocalString::BeginScope UNIQUE_IDENT (beginScope) (name);

/** End scope of translated string literals. */
#define END_XSTRINGS \
static const CCL::LocalString::EndScope UNIQUE_IDENT (endScope); }

/** Define string translation. */
#define XSTRING(var, key) \
static const CCL::LocalString str##var (key);

/** Use translated string as Unicode string. */
#define XSTR(var) \
XStrings::str##var.getText ()

/** Obtain translated string reference. */
#define XSTR_REF(var) \
XStrings::str##var

/** Translate string "on the fly". */
#define TRANSLATE(string) \
CCL::LocalString::translate (nullptr, string)

/** Translate string "on the fly" with scope. */
#define TRANSLATE2(scope, string) \
CCL::LocalString::translate (scope, string)

//************************************************************************************************
// LocalString
/** Helper class to define translated strings statically.  */
//************************************************************************************************

class LocalString
{
public:
	LocalString (const char* key);

	static bool hasTable ();
	static ITranslationTable* getTable ();
	static void setTable (ITranslationTable* table);
	static void tableDestroyed ();
	
	static String translate (StringID scope, StringRef keyString);	
	static String translate (StringID scope, StringID keyString);	

	struct EnglishCorrection
	{
		const char* scope;
		const char* key;
		const char* englishText;
	};

	static void addCorrections (const EnglishCorrection corrections[], int count);

	const char* getKey () const;
	StringRef getText (const ITranslationTable* altTable = nullptr) const;
	operator StringRef () const;

	struct BeginScope
	{
		BeginScope (const char* name);
	};

	struct EndScope
	{
		EndScope ();
	};

protected:
	static ITranslationTable* theTable;
	static const char* currentScope;

	const char* scope;
	const char* key;
	mutable String text;
};

/** @} */

} // namespace CCL

#endif // _ccl_translation_h
