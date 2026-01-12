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
// Filename    : ccl/public/system/ilocalemanager.h
// Description : Locale Manager Interface
//
//************************************************************************************************

#ifndef _ccl_ilocalemanager_h
#define _ccl_ilocalemanager_h

#include "ccl/public/base/iunknown.h"

namespace CCL {

class FileType;
interface ILocaleInfo;
interface IAttributeList;
interface ITranslationTable;
interface ILanguagePack;
interface IUnknownIterator;

//////////////////////////////////////////////////////////////////////////////////////////////////
// Locale Signals
//////////////////////////////////////////////////////////////////////////////////////////////////

namespace Signals
{
	/** Signals related to Locales. */
	DEFINE_STRINGID (kLocales, "CCL.Locales")

		/** [OUT] Input language (keyboard layout) changed. */
		DEFINE_STRINGID (kInputLanguageChanged, "InputLanguageChanged")

		/** [OUT] Application language changed (valid after next restart). args[0]: language code; args[1]: ILanguagePack (can be null). */
		DEFINE_STRINGID (kApplicationLanguageChanged, "ApplicationLanguageChanged")
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// Country Codes
//////////////////////////////////////////////////////////////////////////////////////////////////

namespace CountryCode
{
	DEFINE_STRINGID (kUS, "US")
	DEFINE_STRINGID (kGermany, "DE")
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// Measure IDs
//////////////////////////////////////////////////////////////////////////////////////////////////

namespace MeasureID
{
	DEFINE_STRINGID (kMeasureSI, "SI")  ///< metric measure system (Systeme International)
	DEFINE_STRINGID (kMeasureUS, "US")  ///< United States measure system
}

//************************************************************************************************
// ILocaleManager
/**	\ingroup ccl_system */
//************************************************************************************************

interface ILocaleManager: IUnknown
{
	//////////////////////////////////////////////////////////////////////////////////////////////
	// Language
	//////////////////////////////////////////////////////////////////////////////////////////////

	/** Set current application language (e.g. "en"). */
	virtual void CCL_API setLanguage (StringID language) = 0;

	/** Get current application language (e.g. "en"). */
	virtual StringID CCL_API getLanguage () const = 0;

	/** Get information for current locale. */
	virtual const ILocaleInfo& CCL_API getCurrentLocale () const = 0;

	/** Get locale information by language identifier. */
	virtual const ILocaleInfo* CCL_API getLocale (StringID language) const = 0;

	//////////////////////////////////////////////////////////////////////////////////////////////
	// String Translation
	//////////////////////////////////////////////////////////////////////////////////////////////

	DECLARE_STRINGID_MEMBER (kMainTableID) ///< symbolic identifier for main application string table
	
	/** Load translation table. */
	virtual tresult CCL_API loadStrings (ITranslationTable*& table, UrlRef path, StringID tableID, IAttributeList* variables = nullptr) = 0;

	/** Load translation table of given module. */
	virtual tresult CCL_API loadModuleStrings (ITranslationTable*& table, ModuleRef module, StringID tableID, IAttributeList* variables = nullptr) = 0;

	/** Get translation table. */
	virtual ITranslationTable* CCL_API getStrings (StringID tableID) const = 0;

	/** Unload translation table. */
	virtual tresult CCL_API unloadStrings (ITranslationTable* table) = 0;		

	//////////////////////////////////////////////////////////////////////////////////////////////
	// Language Packs
	//////////////////////////////////////////////////////////////////////////////////////////////

	/** Scan for language packs at given location. */
	virtual int CCL_API scanLanguagePacks (UrlRef url) = 0;

	/** Create iterator for registered language packs (ILanguagePack). */
	virtual IUnknownIterator* CCL_API createLanguagePackIterator () const = 0;

	/** Get currently active language pack (can be null). */
	virtual const ILanguagePack* CCL_API getActiveLanguagePack () const = 0;

	/** Set active language pack (can be null to reset, requires application restart). */
	virtual tresult CCL_API setActiveLanguagePack (const ILanguagePack* languagePack) = 0;

	/** Get language pack file type. */
	virtual const FileType& CCL_API getLanguagePackFileType () const = 0;
	
	/** Get application languages folder. */
	virtual void CCL_API getLanguagesFolder (IUrl& url) const = 0;

	//////////////////////////////////////////////////////////////////////////////////////////////
	// System Information
	//////////////////////////////////////////////////////////////////////////////////////////////

	/** Get system UI language. */
	virtual StringID CCL_API getSystemLanguage () const = 0;

	/** Get input language (keyboard layout) of calling thread. */
	virtual StringID CCL_API getInputLanguage () const = 0;
		
	/** Create iterator for geographic regions (IGeographicRegion) provided by the system. */
	virtual IUnknownIterator* CCL_API createGeographicRegionIterator () const = 0;

	/** Get system region ISO 3166-1 alpha-2 code. @see namespace CountryCode. */
	virtual StringID CCL_API getSystemRegion () const = 0;

	/**	Get character on key for current input language at location specified by a character on the ANSI (US-English) keyboard layout.
		Only defined for characters and digits. */
	virtual uchar CCL_API getCharacterOnKey (uchar characterUS, tbool withCapsLock = false) const = 0;
	
	/** Get measure system. @see namespace MeasureID. */
	virtual StringID CCL_API getMeasureSystem () const = 0;
	
	DECLARE_IID (ILocaleManager)
};

DEFINE_IID (ILocaleManager, 0x81c824af, 0xffc1, 0x4149, 0xab, 0xba, 0x52, 0xcd, 0x5d, 0xe2, 0xdb, 0xb5)
DEFINE_STRINGID_MEMBER (ILocaleManager, kMainTableID, "~main")

//************************************************************************************************
// ILanguagePack
/**	\ingroup ccl_system */
//************************************************************************************************

interface ILanguagePack: IUnknown
{
	/** Get title. */
	virtual StringRef CCL_API getTitle () const = 0;

	/** Get language code. */
	virtual StringID CCL_API getLanguage () const = 0;

	/** Get location of named resource inside language pack. */
	virtual tbool CCL_API getResourceLocation (IUrl& path, StringRef resourceName) const = 0;

	/** Get revision number. */
	virtual int CCL_API getRevision () const = 0;

	DECLARE_IID (ILanguagePack)
};

DEFINE_IID (ILanguagePack, 0xd46924a7, 0x4d74, 0x42c1, 0xa0, 0x1a, 0x28, 0x5a, 0xa8, 0x3e, 0x4e, 0xed)

} // namespace CCL

#endif // _ccl_ilocalemanager_h
