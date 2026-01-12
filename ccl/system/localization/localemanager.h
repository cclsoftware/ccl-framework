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
// Filename    : ccl/system/localization/localemanager.h
// Description : Locale Manager
//
//************************************************************************************************

#ifndef _ccl_localemanager_h
#define _ccl_localemanager_h

#include "ccl/base/singleton.h"
#include "ccl/base/collections/objectarray.h"

#include "ccl/system/localization/localeinfo.h"

#include "ccl/public/system/ilocalemanager.h"

namespace CCL {

class LanguagePack;

//************************************************************************************************
// LocaleManager
//************************************************************************************************

class LocaleManager: public Object,
					 public ILocaleManager,
					 public ExternalSingleton<LocaleManager>
{
public:
	DECLARE_CLASS (LocaleManager, Object)
	DECLARE_METHOD_NAMES (LocaleManager)

	LocaleManager ();
	~LocaleManager ();

	void initialize ();

	// ILocaleManager
	void CCL_API setLanguage (StringID language) override;
	StringID CCL_API getLanguage () const override;
	const ILocaleInfo& CCL_API getCurrentLocale () const override;
	const ILocaleInfo* CCL_API getLocale (StringID language) const override;
	tresult CCL_API loadStrings (ITranslationTable*& table, UrlRef path, StringID tableID, IAttributeList* variables = nullptr) override;
	tresult CCL_API loadModuleStrings (ITranslationTable*& table, ModuleRef module, StringID tableID, IAttributeList* variables = nullptr) override;
	ITranslationTable* CCL_API getStrings (StringID tableID) const override;
	tresult CCL_API unloadStrings (ITranslationTable* table) override;
	int CCL_API scanLanguagePacks (UrlRef url) override;
	IUnknownIterator* CCL_API createLanguagePackIterator () const override;
	const ILanguagePack* CCL_API getActiveLanguagePack () const override;
	tresult CCL_API setActiveLanguagePack (const ILanguagePack* languagePack) override;
	const FileType& CCL_API getLanguagePackFileType () const override;
	void CCL_API getLanguagesFolder (IUrl& url) const override;
	StringID CCL_API getSystemLanguage () const override;
	StringID CCL_API getInputLanguage () const override;
	IUnknownIterator* CCL_API createGeographicRegionIterator () const override;
	StringID CCL_API getSystemRegion () const override;
	uchar CCL_API getCharacterOnKey (uchar characterUS, tbool withCapsLock = false) const override;
	StringID CCL_API getMeasureSystem () const override;
	
	CLASS_INTERFACE (ILocaleManager, Object)

	class TranslationTable;

protected:
	MutableCString language;
	ObjectList tables;
	LocaleInfoList locales;
	LocaleInfoBase defaultLocale;
	ObjectArray languagePacks;
	LanguagePack* activeLanguagePack;

	void loadLocales ();
	void restoreUserLanguage ();
	bool restoreLanguagePack (UrlRef path);
	TranslationTable* addTableEntry (StringID tableID);
	tresult loadStringTable (ITranslationTable*& table, UrlRef packagePath, bool subFolder, StringID tableID, IAttributeList* variables);
	TranslationTable* getTableEntry (StringID tableID) const;
	TranslationTable* getEntryForTable (ITranslationTable* table) const;

	// IObject
	tbool CCL_API invokeMethod (Variant& returnValue, MessageRef msg) override;

	// to be implemented by platform subclass:
	virtual bool getNativeUserLanguage (MutableCString& language) const;
	virtual void setNativeUserLanguage (StringID language);
	virtual void setNativeLanguagePack (StringRef pathString);
	virtual bool getNativeLanguagePack (String& pathString) const;
	virtual void collectGeographicRegions (GeographicRegionList& list) const;
};

} // namespace CCL

#endif // _ccl_localemanager_h
