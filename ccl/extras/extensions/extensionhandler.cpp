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
// Filename    : ccl/extras/extensions/extensionhandler.cpp
// Description : Extension Handler
//
//************************************************************************************************

#define DEBUG_LOG 1

#include "ccl/extras/extensions/extensionhandler.h"
#include "ccl/extras/extensions/extensiondescription.h"

#include "ccl/app/presets/presetfile.h"
#include "ccl/app/presets/presetfileprimitives.h"
#include "ccl/public/app/ipresetmetainfo.h"
#include "ccl/app/documents/documenttemplates.h"
#include "ccl/app/utilities/pluginclass.h"

#include "ccl/base/message.h"
#include "ccl/base/storage/file.h"
#include "ccl/base/collections/stringdictionary.h"

#include "ccl/public/storage/ifileresource.h"
#include "ccl/public/system/ilocalemanager.h"
#include "ccl/public/system/inativefilesystem.h"
#include "ccl/public/system/ipackagemetainfo.h"
#include "ccl/public/plugins/icoderesource.h"
#include "ccl/public/plugins/stubobject.h"
#include "ccl/public/gui/framework/ihelpmanager.h"
#include "ccl/public/gui/framework/ithememanager.h"
#include "ccl/public/systemservices.h"
#include "ccl/public/plugservices.h"
#include "ccl/public/guiservices.h"

namespace CCL {
namespace Install {

//************************************************************************************************
// ExtensionHandlerStub
//************************************************************************************************

class ExtensionHandlerStub: public StubObject,
							public IExtensionHandler
{
public:
	DECLARE_STUB_METHODS (IExtensionHandler, ExtensionHandlerStub)

	// IExtensionHandler
	int CCL_API startupExtension (IExtensionDescription& description) override
	{
		Variant returnValue;
		invokeMethod (returnValue, Message ("startupExtension", &description));
		return returnValue.asInt ();
	}
};	

} // namespace Install
} // namespace CCL

using namespace CCL;
using namespace Install;

//////////////////////////////////////////////////////////////////////////////////////////////////
// Stub registration
//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_KERNEL_INIT_LEVEL (ExtensionHandlerStub, kFirstRun)
{
	REGISTER_STUB_CLASS (IExtensionHandler, ExtensionHandlerStub)
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// Extension Handler Folder Names
//////////////////////////////////////////////////////////////////////////////////////////////////

static const String kNativePluginFolderName ("plugins");
static const String kCorePluginFolderName ("coreplugins");
static const String kScriptPluginFolderName ("scripts");
static const String kLanguageFolderName ("languages");
static const String kHelpFolderName ("help");
static const String kTutorialsFolderName ("tutorials");
static const String kSkinFolderName ("skin");
static const String kSkinFileName ("default.skin");

//************************************************************************************************
// ExtensionHandler
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (ExtensionHandler, Object)

//************************************************************************************************
// IExtensionProductHandler
//************************************************************************************************

DEFINE_IID_ (IExtensionProductHandler, 0xf112d2dc, 0x7269, 0x42b8, 0x83, 0x2c, 0xa2, 0x5a, 0x42, 0x8d, 0x61, 0x9a)

//************************************************************************************************
// ExtensionNativePluginHandler
//************************************************************************************************

int ExtensionNativePluginHandler::startupExtension (ExtensionDescription& description)
{
	Url nativePluginFolder (description.getPath ());
	nativePluginFolder.descend (kNativePluginFolderName, Url::kFolder);
	nativePluginFolder.descend (EXTENSION_PLATFORM_FOLDER, Url::kFolder);

	if(System::GetFileSystem ().fileExists (nativePluginFolder))
		return System::GetPlugInManager ().scanFolder (nativePluginFolder, CodeResourceType::kNative);
	return 0;
}

//************************************************************************************************
// ExtensionCorePluginHandler
//************************************************************************************************

int ExtensionCorePluginHandler::startupExtension (ExtensionDescription& description)
{
	Url corePluginFolder (description.getPath ());
	corePluginFolder.descend (kCorePluginFolderName, Url::kFolder);
	corePluginFolder.descend (EXTENSION_PLATFORM_FOLDER, Url::kFolder);

	if(System::GetFileSystem ().fileExists (corePluginFolder))
		return System::GetPlugInManager ().scanFolder (corePluginFolder, CodeResourceType::kCore);
	return 0;
}

//************************************************************************************************
// ExtensionScriptPluginHandler
//************************************************************************************************

int ExtensionScriptPluginHandler::startupExtension (ExtensionDescription& description)
{
	Url scriptPluginFolder (description.getPath ());
	scriptPluginFolder.descend (kScriptPluginFolderName, Url::kFolder);

	if(System::GetFileSystem ().fileExists (scriptPluginFolder))
		return System::GetPlugInManager ().scanFolder (scriptPluginFolder, CodeResourceType::kScript);
	return 0;
}

//************************************************************************************************
// ExtensionLanguageHandler
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (ExtensionLanguageHandler::TableEntry, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

ExtensionLanguageHandler::ExtensionLanguageHandler ()
{
	tables.objectCleanup (true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int ExtensionLanguageHandler::startupExtension (ExtensionDescription& description)
{
	Url languageFolder (description.getPath ());
	languageFolder.descend (kLanguageFolderName, Url::kFolder);

	int scannedLanguagePacks = 0;
	if(System::GetFileSystem ().fileExists (languageFolder))
		scannedLanguagePacks = System::GetLocaleManager ().scanLanguagePacks (languageFolder);

	ITranslationTable* table = nullptr;
	String tableLocation = description.getExtraInfo ().getString (Meta::kTranslationStringTable);
	if(!tableLocation.isEmpty ())
	{
		MutableCString customTableId (description.getExtraInfo ().getString (Meta::kTranslationTableID));
		MutableCString tableId = customTableId.isEmpty () ? description.getID () : customTableId;
		Url stringTablePath = description.getPath ();
		stringTablePath.descend (tableLocation, IUrl::kDetect);
		System::GetLocaleManager ().loadStrings (table, stringTablePath, tableId);
		if(table)
			tables.add (NEW TableEntry (String (tableId), table));
	}
	
	if(scannedLanguagePacks > 0 || table)
		return 1;

	return 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ExtensionLanguageHandler::onExtensionChanged (ExtensionDescription& description)
{
	// check if extension contains the active language pack
	const ILanguagePack* languagePack = System::GetLocaleManager ().getActiveLanguagePack ();
	UnknownPtr<IFileResource> file = const_cast<ILanguagePack*> (languagePack);
	bool active = file && Url (description.getPath ()).contains (file->getPath ());
	if(active)
	{
		if(description.isEnabled () && !description.isUninstallPending () && !description.isUpdatePending ())
			System::GetLocaleManager ().setActiveLanguagePack (languagePack); // keep it
		else
			System::GetLocaleManager ().setActiveLanguagePack (nullptr); // reset to English
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ExtensionLanguageHandler::shutdownExtension (ExtensionDescription& description)
{
	String tableId = description.getExtraInfo ().getString (Meta::kTranslationTableID);
	if(tableId.isEmpty ())
		tableId = description.getID ();

	auto result = tables.findIf<TableEntry> ([&] (const TableEntry& t) { return t.getTableID () == tableId; });
	if(result)
		if(System::GetLocaleManager ().unloadStrings (result->getTable ()) == kResultOk)
		{
			tables.remove (result);
			result->release ();
		}
}

//************************************************************************************************
// ExtensionHelpHandler
//************************************************************************************************

int ExtensionHelpHandler::startupExtension (ExtensionDescription& description)
{
	int result = 0;

	Url helpFolder (description.getPath ());
	helpFolder.descend (kHelpFolderName, Url::kFolder);
	if(System::GetFileSystem ().fileExists (helpFolder))
	{
		if(System::GetHelpManager ().addHelpCatalog (helpFolder, IHelpCatalog::kGlobal) == kResultOk)
			result++;
	}

	Url tutorialsFolder (description.getPath ());
	tutorialsFolder.descend (kTutorialsFolderName, Url::kFolder);
	if(System::GetFileSystem ().fileExists (tutorialsFolder))
	{
		if(System::GetHelpManager ().addTutorials (tutorialsFolder) == kResultOk)
			result++;
	}

	return result;
}

//************************************************************************************************
// ExtensionPresetHandler
//************************************************************************************************

int ExtensionPresetHandler::startupExtension (ExtensionDescription& description)
{
	#if DEBUG_LOG
	CCL_PRINTF ("Extension extraInfo (%s)\n", MutableCString (description.getTitle ()).str ())
	description.getExtraInfo ().dump ();
	#endif

	Url presetFolder (description.getPath ());
	presetFolder.descend (PresetPackageHandler::kPresetFolder, Url::kFolder);
	if(System::GetFileSystem ().fileExists (presetFolder))
	{
		PresetLocationHandler::instance ().addLocation (presetFolder);
		return 1;
	}
	return 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ExtensionPresetHandler::onExtensionInstalled (ExtensionDescription& description, bool silent)
{
	// look for default presets for the included plugins
	String vendor (description.getExtraInfo ().getString (Meta::kPresetHandlerVendorName));
	if(vendor.isEmpty ())
		vendor = RootComponent::instance ().getCompanyName ();
	if(vendor.isEmpty ())
		return;

	PresetPackageHandler& presetHandler = PresetPackageHandler::instance ();

	Url presetFolder (description.getPath ());
	presetFolder.descend (PresetPackageHandler::kPresetFolder, Url::kFolder);

	Url vendorFolder (presetFolder);
	vendorFolder.descend (vendor);

	if(CCL::File (vendorFolder).exists ())
	{
		// scan subFolders, interpret them as className (would be easier and more precise if we had the meta attributes of the included plugins)
		ForEachFile (CCL::File (vendorFolder).getFS ().newIterator (vendorFolder, IFileIterator::kFolders), p)
			String className;
			p->getName (className, false);

			Url defaultPresetPath (*p);
			PresetFilePrimitives::descendPresetName (defaultPresetPath, PresetFilePrimitives::kDefaultPresetFileName, presetHandler, false);

			CCL::File defaultPreset (defaultPresetPath);
			if(defaultPreset.exists ())
			{
				// found default preset: copy to user presets folder in the same vendor/className subFolders
				Url destFile;
				presetHandler.getWriteLocation (destFile, nullptr);
				destFile.descend (vendor);
				destFile.descend (className);
				PresetFilePrimitives::descendPresetName (destFile, PresetFilePrimitives::kDefaultPresetFileName, presetHandler, false);

				defaultPreset.copyTo (destFile);
			}
		EndFor
	}
}

//************************************************************************************************
// ExtensionTemplateHandler
//************************************************************************************************

int ExtensionTemplateHandler::startupExtension (ExtensionDescription& description)
{
	Url templateFolder (description.getPath ());
	templateFolder.descend (DocumentTemplateList::kTemplatesFolder, Url::kFolder);
	if(System::GetFileSystem ().fileExists (templateFolder))
	{
		templateFolder.getParameters ().appendEntry (CCLSTR (UrlParameter::kPackageID), description.getID ());
		DocumentTemplateList::addAdditionalLocation (templateFolder);
		return 1;
	}
	return 0;
}

//************************************************************************************************
// ExtensionSkinHandler
//************************************************************************************************

ExtensionSkinHandler::~ExtensionSkinHandler ()
{
	ASSERT (themeList.isEmpty ())
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ExtensionSkinHandler::shutdown ()
{
	ListForEach (themeList, ITheme*, theme)
		System::GetThemeManager ().unloadTheme (theme);
	EndFor
	themeList.removeAll ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int ExtensionSkinHandler::startupExtension (ExtensionDescription& description)
{
	Url path;
	if(findDefaultSkin (path, description.getPath ()))
	{
		MutableCString packageId (description.getID ());
		MutableCString customTableId (description.getExtraInfo ().getString (Meta::kTranslationTableID));
		MutableCString tableId = customTableId.isEmpty () ? packageId : customTableId;

		ITranslationTable* stringTable = System::GetLocaleManager ().getStrings (tableId);

		ITheme* theme = nullptr;
		System::GetThemeManager ().loadTheme (theme, path, packageId, stringTable);
		ASSERT (theme != nullptr)
		themeList.append (theme);
		return 1;
	}
	return 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ExtensionSkinHandler::findDefaultSkin (Url& result, UrlRef path) const
{
	// try folder
	Url skinFolder (path);
	skinFolder.descend (kSkinFolderName, Url::kFolder);
	if(System::GetFileSystem ().fileExists (skinFolder))
	{
		result = skinFolder;
		return true;
	}

	// try file
	Url skinFile (path);
	skinFile.descend (kSkinFileName, Url::kFile);
	if(System::GetFileSystem ().fileExists (skinFile))
	{
		result = skinFile;
		return true;
	}

	return false;
}

//************************************************************************************************
// ExtensionSnapshotHandler
//************************************************************************************************

int ExtensionSnapshotHandler::startupExtension (ExtensionDescription& description)
{
	Url snapshotFolder (description.getPath ());
	snapshotFolder.descend (PlugInSnapshots::kFolderName, Url::kFolder);
	if(System::GetFileSystem ().fileExists (snapshotFolder))
	{
		PlugInSnapshots::instance ().addLocation (snapshotFolder);
		return 1;
	}
	return 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ExtensionSnapshotHandler::shutdownExtension (ExtensionDescription& description)
{
	Url snapshotFolder (description.getPath ());
	snapshotFolder.descend (PlugInSnapshots::kFolderName, Url::kFolder);
	PlugInSnapshots::instance ().removeLocation (snapshotFolder);
}

//************************************************************************************************
// ExternalExtensionHandler
//************************************************************************************************

ExternalExtensionHandler::~ExternalExtensionHandler ()
{
	ASSERT (handlers.isEmpty ())
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ExternalExtensionHandler::startup ()
{
	ForEachPlugInClass (PLUG_CATEGORY_EXTENSIONHANDLER, description)
		IExtensionHandler* handler = ccl_new<IExtensionHandler> (description.getClassID ());
		ASSERT (handler)
		if(handler)
		{
			handlers.append (handler);
			if(UnknownPtr<IComponent> component = handler)
				component->initialize (nullptr);
		}
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ExternalExtensionHandler::shutdown ()
{
	ListForEach (handlers, IExtensionHandler*, handler)
		if(UnknownPtr<IComponent> component = handler)
			component->terminate ();

		ccl_release (handler);
	EndFor
	handlers.removeAll ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int ExternalExtensionHandler::startupExtension (ExtensionDescription& description)
{
	int useCount = 0;
	ListForEach (handlers, IExtensionHandler*, handler)
		useCount += handler->startupExtension (description);
	EndFor
	return useCount;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ExternalExtensionHandler::shutdownExtension (ExtensionDescription& description)
{
	// TODO:
	#if 0
	ListForEach (handlers, IExtensionHandler*, handler)
		handler->shutdownExtension (description);
	EndFor
	#endif
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API ExternalExtensionHandler::checkCompatibility (IExtensionDescription& description)
{
	ListForEach (handlers, IExtensionHandler*, handler)
		UnknownPtr<IExtensionCompatibilityHandler> compatibilityHandler (handler);
		if(compatibilityHandler)
		{
			tresult result = compatibilityHandler->checkCompatibility (description);
			if(result != kResultOk)
				return result;
		}
	EndFor
	return kResultOk;
}
