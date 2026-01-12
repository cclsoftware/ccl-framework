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
// Filename    : ccl/public/text/itranslationtable.h
// Description : Translation Table Interface
//
//************************************************************************************************

#ifndef _ccl_itranslationtable_h
#define _ccl_itranslationtable_h

#include "ccl/public/base/iunknown.h"

namespace CCL {

interface IStream;
interface ITranslationTableHook;

//************************************************************************************************
// ITranslationTable
/** String translation table.
	\ingroup translation */
//************************************************************************************************

interface ITranslationTable: IUnknown
{
	/**	Add variable for translation string processing, specified by key and value. 
		Variables must be defined *before* translations are loaded! */
	virtual tresult CCL_API addVariable (StringID name, StringRef text) = 0;

	/** Add a single translation. Key and scope name must be ASCII! */
	virtual tresult CCL_API addString (StringID scope, StringID key, StringRef text) = 0;

	/** Load string translations from GNU MO (Machine Object) stream. */
	virtual tresult CCL_API loadStrings (IStream& stream, ITranslationTableHook* hook = nullptr) = 0;

	/** Get translated string for key in specified scope. Key and scope name must be ASCII! */
	virtual tresult CCL_API getString (String& result, StringID scope, StringID key) const = 0;

	/** Add a single translation. Non-ASCII characters in key are replaced with XML entities. */
	virtual tresult CCL_API addStringWithUnicodeKey (StringID scope, StringRef unicodeKey, StringRef text) = 0;

	/** Get translated string for Unicode key. Non-ASCII characters in key are replaced with XML entities. */
	virtual tresult CCL_API getStringWithUnicodeKey (String& result, StringID scope, StringRef unicodeKey) const = 0;

	/** Save string translations to GNU PO (Portable Object) or POT (Portable Object Template). */
	virtual tresult CCL_API saveStrings (IStream& stream, tbool isTemplate) const = 0;

	DECLARE_IID (ITranslationTable)
};

DEFINE_IID (ITranslationTable, 0x3599536d, 0xcbe9, 0x4f6f, 0xa8, 0x20, 0x6e, 0xf2, 0x96, 0x68, 0x50, 0xc)

//************************************************************************************************
// ITranslationTableHook
/** String translation table hook. 
	\ingroup ccl_text */
//************************************************************************************************

interface ITranslationTableHook: IUnknown
{
	/** Translation has been added to table. */
	virtual void CCL_API translationAdded (StringID scope, StringID key, StringRef text) = 0;

	DECLARE_IID (ITranslationTableHook)
};

DEFINE_IID (ITranslationTableHook, 0x6e604130, 0x685f, 0x4184, 0xbc, 0x9e, 0xed, 0x91, 0x33, 0x22, 0x67, 0x4c)

} // namespace CCL

#endif // _ccl_itranslationtable_h
