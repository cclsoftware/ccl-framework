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
// Filename    : ccl/system/localization/localemanager.cpp
// Description : Locale Manager
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "ccl/system/localization/localemanager.h"
#include "ccl/system/localization/languagepack.h"

#include "ccl/base/message.h"
#include "ccl/base/signalsource.h"
#include "ccl/base/storage/url.h"

#include "ccl/public/text/language.h"
#include "ccl/public/storage/iattributelist.h"
#include "ccl/public/text/itranslationtable.h"
#include "ccl/public/system/inativefilesystem.h"
#include "ccl/public/system/ifileutilities.h"
#include "ccl/public/system/ipackagefile.h"
#include "ccl/public/system/ipackagehandler.h"
#include "ccl/public/system/isysteminfo.h"
#include "ccl/public/systemservices.h"

#if CCL_STATIC_LINKAGE
#define USE_SHARED_TRANSLATION_TABLE 1 // we share a single translation table if CCL is linked statically
#endif

namespace CCL {

//************************************************************************************************
// LocaleManager::TranslationTable
//************************************************************************************************

class LocaleManager::TranslationTable: public Object
{
public:
	DECLARE_CLASS (TranslationTable, Object)
	DECLARE_METHOD_NAMES (TranslationTable)

	TranslationTable (StringID id = nullptr);

	PROPERTY_MUTABLE_CSTRING (id, ID)
	PROPERTY_SHARED_AUTO (ITranslationTable, table, Table)
	PROPERTY_BOOL (main, Main)

	// IObject
	tbool CCL_API invokeMethod (CCL::Variant& returnValue, MessageRef msg) override;
};

} // namespace CCL

using namespace CCL;

//////////////////////////////////////////////////////////////////////////////////////////////////
// System Services API
//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_EXPORT ILocaleManager& CCL_API System::CCL_ISOLATED (GetLocaleManager) ()
{
	return LocaleManager::instance ();
}

//************************************************************************************************
// LocaleManager
//************************************************************************************************

DEFINE_CLASS (LocaleManager, Object)
DEFINE_CLASS_NAMESPACE (LocaleManager, NAMESPACE_CCL)

//////////////////////////////////////////////////////////////////////////////////////////////////

LocaleManager::LocaleManager ()
: activeLanguagePack (nullptr)
{
	tables.objectCleanup (true);
	languagePacks.objectCleanup (true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void LocaleManager::initialize ()
{
	loadLocales ();

	restoreUserLanguage ();

	ASSERT (!language.isEmpty ())
}

//////////////////////////////////////////////////////////////////////////////////////////////////

LocaleManager::~LocaleManager ()
{
#if !USE_SHARED_TRANSLATION_TABLE
	ASSERT (tables.isEmpty () == true)
#endif

	if(activeLanguagePack)
		activeLanguagePack->close ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void LocaleManager::loadLocales ()
{
	ResourceUrl path (CCLSTR ("localeinfo.xml"));
	bool result = locales.loadFromFile (path);
	SOFT_ASSERT (result == true, "Locale info not loaded")
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API LocaleManager::setLanguage (StringID language)
{
	setNativeUserLanguage (language);
	setNativeLanguagePack (String::kEmpty); // reset language pack

	SignalSource (Signals::kLocales).signal (Message (Signals::kApplicationLanguageChanged, String (language)));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringID CCL_API LocaleManager::getLanguage () const
{
	return language;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const ILocaleInfo& CCL_API LocaleManager::getCurrentLocale () const
{
	if(activeLanguagePack)
		return activeLanguagePack->getLocaleInfo ();

	const ILocaleInfo* localeInfo = getLocale (language);
	if(localeInfo)
		return *localeInfo;

	return defaultLocale;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const ILocaleInfo* CCL_API LocaleManager::getLocale (StringID language) const
{
	// Note: This method behaves ambiguous if language packs and built-in locales
	// have the same language code.

	ArrayForEach (languagePacks, LanguagePack, languagePack)
		if(languagePack->getLanguage () == language)
			return &languagePack->getLocaleInfo ();
	EndFor
	
	ListForEachObject (locales.getLocales (), LocaleInfo, info)
		if(info->getLanguage () == language)
			return info;
	EndFor

	if(language == LanguageCode::English)
		return &defaultLocale;

	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

LocaleManager::TranslationTable* LocaleManager::addTableEntry (StringID tableID)
{
	TranslationTable* table = nullptr;

#if USE_SHARED_TRANSLATION_TABLE
	if(tables.isEmpty ())
		tables.add (NEW TranslationTable);
	table = (TranslationTable*)tables.getFirst ();
#else
	tables.add (table = NEW TranslationTable (tableID));
#endif

	return table;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult LocaleManager::loadStringTable (ITranslationTable*& table, UrlRef packagePath, bool subFolder, StringID tableID, IAttributeList* variables)
{
	TranslationTable* entry = addTableEntry (tableID);
	table = entry->getTable ();
	if(variables)
	{
		ForEachAttribute (*variables, name, value)
			table->addVariable (name, value.asString ());
		EndFor
	}

	bool loaded = false;
	if(System::GetFileSystem ().fileExists (packagePath))
	{
		AutoPtr<IPackageFile> packageFile = System::GetPackageHandler ().openPackage (packagePath);
		if(packageFile)
		{
			IFileSystem* fileSystem = packageFile->getFileSystem ();
			ASSERT (fileSystem != nullptr)
			loaded = true;
			
			Url localeFolder (nullptr, Url::kFolder);
			if(subFolder)
			{
				String name (getLanguage ());
				//if(name.contains (CCLSTR ("-"))) // "en-US" -> "en"
				//	name.truncate (name.index (CCLSTR ("-")));
				localeFolder.setPath (name, Url::kFolder);
			}
			
			ForEachFile (fileSystem->newIterator (localeFolder, IFileIterator::kFiles), path)
			CCL_PRINT ("#\tLoading strings from file: ")
			CCL_PRINTLN (path->getPath ())
			
			AutoPtr<IStream> stream = fileSystem->openStream (*path);
			ASSERT (stream != nullptr)
			if(stream)
			{
				AutoPtr<IStream> seekableStream = System::GetFileUtilities ().createSeekableStream (*stream);
				ASSERT (seekableStream != nullptr)
				tresult result = table->loadStrings (*seekableStream);
				ASSERT (result == kResultOk)
			}
			EndFor
		}
	}
	
	return loaded || language == LanguageCode::English ? kResultOk : kResultFalse;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API LocaleManager::loadStrings (ITranslationTable*& table, UrlRef packagePath, StringID tableID, IAttributeList* variables)
{
	CCL_PRINT ("# Load String Package: ")
	CCL_PRINT (tableID)
	CCL_PRINT (" - ")
	CCL_PRINTLN (UrlFullString (packagePath))

	// 1) try to redirect to active language pack
	if(activeLanguagePack)
	{
		Url redirectPath;
		if(activeLanguagePack->getTableLocation (redirectPath, tableID))
		{
			if(loadStringTable (table, redirectPath, false, tableID, variables) == kResultOk)
				return kResultOk;
		
			unloadStrings (table); // load failed, cleanup table
		}
	}

	// 2) load from given location
	return loadStringTable (table, packagePath, true, tableID, variables);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API LocaleManager::loadModuleStrings (ITranslationTable*& table, ModuleRef module, StringID tableID, IAttributeList* variables)
{
	ResourceUrl path (module, CCLSTR ("translations.package"), Url::kFile);		
	tresult result = loadStrings (table, path, tableID, variables);

	// try to load from translations folder
	// (points to built-in resources, not to the development resource folder)
	if(result != kResultOk)
	{
		unloadStrings (table);

		ResourceUrl path2 (module, CCLSTR ("translations"), Url::kFolder);
		result = loadStrings (table, path2, tableID, variables);
	}

	#if !USE_SHARED_TRANSLATION_TABLE
	// remember which table corresponds to the main application
	if(module == System::GetMainModuleRef ())
		if(TranslationTable* entry = getEntryForTable (table))
			entry->setMain (true);
	#endif

	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

LocaleManager::TranslationTable* LocaleManager::getTableEntry (StringID tableID) const
{
#if USE_SHARED_TRANSLATION_TABLE
	return (TranslationTable*)tables.getFirst ();
#else
	if(tableID == kMainTableID)
	{
		ListForEachObject (tables, TranslationTable, entry)
			if(entry->isMain ())
				return entry;
		EndFor
	}
	else
	{
		ListForEachObject (tables, TranslationTable, entry)
			if(entry->getID () == tableID)
				return entry;
		EndFor
	}
	return nullptr;
#endif
}

//////////////////////////////////////////////////////////////////////////////////////////////////

LocaleManager::TranslationTable* LocaleManager::getEntryForTable (ITranslationTable* table) const
{
#if USE_SHARED_TRANSLATION_TABLE
	return (TranslationTable*)tables.getFirst ();
#else
	ListForEachObject (tables, TranslationTable, entry)
		if(entry->getTable () == table)
			return entry;
	EndFor
	return nullptr;
#endif
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ITranslationTable* CCL_API LocaleManager::getStrings (StringID tableID) const
{
	TranslationTable* entry = getTableEntry (tableID);
	return entry ? entry->getTable () : nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API LocaleManager::unloadStrings (ITranslationTable* table)	
{
#if !USE_SHARED_TRANSLATION_TABLE
	TranslationTable* foundEntry = getEntryForTable (table);
	ASSERT (foundEntry != nullptr)
	if(!foundEntry)
		return kResultFalse;

	tables.remove (foundEntry);
	foundEntry->release ();
#endif
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API LocaleManager::scanLanguagePacks (UrlRef url)
{
	int count = LanguagePackHandler::find (languagePacks, url);
	if(count > 0)
		languagePacks.sort (); // sort by title
	return count;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IUnknownIterator* CCL_API LocaleManager::createLanguagePackIterator () const
{
	return languagePacks.newIterator ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const ILanguagePack* CCL_API LocaleManager::getActiveLanguagePack () const
{
	return activeLanguagePack;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API LocaleManager::setActiveLanguagePack (const ILanguagePack* _languagePack)
{
	String pathString;
	if(LanguagePack* languagePack = unknown_cast<LanguagePack> (_languagePack))
	{
		Url path = languagePack->getPath ();
		pathString = UrlDisplayString (path);
		if(path.getProtocol () == ResourceUrl::Protocol)
			path.getUrl (pathString);
		
		if(path.isFolder ()) // mark folders with "/" at the end
			if(!pathString.endsWith (Url::strPathChar))
				pathString.append (Url::strPathChar);
	}

	setNativeLanguagePack (pathString);
	setNativeUserLanguage (CString::kEmpty); // reset user language

	MutableCString language (_languagePack ? _languagePack->getLanguage () : LanguageCode::English);
	SignalSource (Signals::kLocales).signal (Message (Signals::kApplicationLanguageChanged, String (language), const_cast<ILanguagePack*> (_languagePack)));
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void LocaleManager::restoreUserLanguage ()
{
	#if (0 && DEBUG)
	Debugger::printf ("System language is '%s'\n", getSystemLanguage ().str ());
	#endif

	// 1) try to restore last selected language pack
	String pathString;
	if(getNativeLanguagePack (pathString))
	{
		Url path;
		if(pathString.contains (CCLSTR ("://")))
			path.setUrl (pathString);
		else
			path.fromDisplayString (pathString, Url::kDetect); // detect folder with "/" at the end

		if(restoreLanguagePack (path))
			return;
	}

	// 2) try to find language pack based on saved/system language
	MutableCString savedLanguage;
	getNativeUserLanguage (savedLanguage);
	if(savedLanguage.isEmpty ())
		savedLanguage = getSystemLanguage ();

	StringDictionary dict;
	StorableObject::loadFromFile (dict, Url ("resource:///languagepacks.xml"));
	String fileName = dict.lookupValue (String (savedLanguage));
	if(!fileName.isEmpty ())
	{
		Url path;
		getLanguagesFolder (path);
		path.descend (fileName);

		if(restoreLanguagePack (path))
		{
			String displayString = UrlDisplayString (path);
			if(path.getProtocol () == ResourceUrl::Protocol)
				path.getUrl (displayString);

			setNativeLanguagePack (displayString);
			setNativeUserLanguage (CString::kEmpty); // reset user language
			return;
		}
	}			

	// 3) default to built-in English language
	language = LanguageCode::English;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool LocaleManager::restoreLanguagePack (UrlRef path)
{
	ASSERT (activeLanguagePack == nullptr)
	ASSERT (languagePacks.isEmpty ())

	AutoPtr<LanguagePack> languagePack = NEW LanguagePack (path);
	if(languagePack->open ())
	{
		languagePacks.add (return_shared<LanguagePack> (languagePack));

		activeLanguagePack = languagePack;
		language = languagePack->getLanguage ();
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const FileType& CCL_API LocaleManager::getLanguagePackFileType () const
{
	return LanguagePack::getFileType ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API LocaleManager::getLanguagesFolder (IUrl& url) const
{
	System::GetSystem ().getLocation (url, CCL::System::kAppSupportFolder);
	url.descend (CCLSTR ("Languages"), Url::kFolder);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IUnknownIterator* CCL_API LocaleManager::createGeographicRegionIterator () const
{
	AutoPtr<GeographicRegionList> list = NEW GeographicRegionList;
	collectGeographicRegions (*list);
	return NEW HoldingIterator (list, list->newIterator ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringID CCL_API LocaleManager::getSystemLanguage () const
{
	return LanguageCode::English;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringID CCL_API LocaleManager::getInputLanguage () const
{
	return LanguageCode::EnglishUS;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool LocaleManager::getNativeUserLanguage (MutableCString& language) const
{
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void LocaleManager::setNativeUserLanguage (StringID language)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void LocaleManager::setNativeLanguagePack (StringRef pathString)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool LocaleManager::getNativeLanguagePack (String& pathString) const
{
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringID CCL_API LocaleManager::getSystemRegion () const
{
	return CountryCode::kUS;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void LocaleManager::collectGeographicRegions (GeographicRegionList& list) const
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

uchar CCL_API LocaleManager::getCharacterOnKey (uchar characterUS, tbool withCapsLock) const
{
	return characterUS;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringID CCL_API LocaleManager::getMeasureSystem () const
{
	if(getSystemRegion () == CountryCode::kUS)
		return MeasureID::kMeasureUS;
	return MeasureID::kMeasureSI;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_METHOD_NAMES (LocaleManager)
	DEFINE_METHOD_ARGR ("getStrings", "tableId", "LocaleManager.TranslationTable")
END_METHOD_NAMES (LocaleManager)

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API LocaleManager::invokeMethod (Variant& returnValue, MessageRef msg)
{
	if(msg == "getStrings")
	{
		MutableCString tableID (msg[0]);
		if(TranslationTable* entry = getTableEntry (tableID))
			returnValue.takeShared (ccl_as_unknown (entry));
		return true;
	}
	else
		return SuperClass::invokeMethod (returnValue, msg);
}

//************************************************************************************************
// LocaleManager::TranslationTable
//************************************************************************************************

DEFINE_CLASS (LocaleManager::TranslationTable, Object)
DEFINE_CLASS_NAMESPACE (LocaleManager::TranslationTable, NAMESPACE_CCL)

//////////////////////////////////////////////////////////////////////////////////////////////////

LocaleManager::TranslationTable::TranslationTable (StringID id)
: id (id),
  table (System::CreateTranslationTable ()),
  main (false)
{
	ASSERT (table != nullptr)
}

//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_METHOD_NAMES (LocaleManager::TranslationTable)
	DEFINE_METHOD_NAME ("getString")
END_METHOD_NAMES (LocaleManager::TranslationTable)

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API LocaleManager::TranslationTable::invokeMethod (CCL::Variant& returnValue, MessageRef msg)
{
	if(msg == "getString")
	{
		CCL::String translated;
		switch(msg.getArgCount ())
		{
		case 1 : 
			table->getString (translated, nullptr, MutableCString (msg[0].asString ()));
			break;
		case 2 : 
			table->getString (translated, MutableCString (msg[0].asString ()), MutableCString (msg[1].asString ()));
			break;
		}

		returnValue = translated;
		returnValue.share ();
		return true;
	}
	else
		return SuperClass::invokeMethod (returnValue, msg);
}
