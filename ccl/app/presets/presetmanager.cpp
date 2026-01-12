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
// Filename    : ccl/app/presets/presetmanager.cpp
// Description : Preset Manager
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "ccl/app/presets/presetmanager.h"
#include "ccl/app/presets/presetsystem.h"
#include "ccl/app/presets/presetfile.h"
#include "ccl/app/presets/presetfileprimitives.h"
#include "ccl/app/presets/presettrader.h"
#include "ccl/app/presets/presetdescriptor.h"
#include "ccl/app/presets/objectpreset.h"
#include "ccl/app/utilities/pluginclass.h"

#include "ccl/app/safety/appsafety.h"

#include "ccl/base/message.h"
#include "ccl/base/objectconverter.h"
#include "ccl/base/storage/file.h"
#include "ccl/base/storage/settings.h"
#include "ccl/base/storage/storage.h"
#include "ccl/base/storage/persistence/expression.h"
#include "ccl/base/boxedtypes.h"

#include "ccl/public/app/presetmetainfo.h"
#include "ccl/public/app/signals.h"
#include "ccl/public/gui/framework/ialert.h"
#include "ccl/public/gui/framework/iprogressdialog.h"
#include "ccl/public/system/isysteminfo.h"
#include "ccl/public/system/inativefilesystem.h"
#include "ccl/public/system/ithreadpool.h"
#include "ccl/public/system/isearcher.h"
#include "ccl/public/system/ilockable.h"
#include "ccl/public/collections/unknownlist.h"
#include "ccl/public/collections/variantvector.h"
#include "ccl/public/text/translation.h"
#include "ccl/public/plugservices.h"

namespace CCL {

//************************************************************************************************
// PresetConvertFilterBase
//************************************************************************************************

class PresetConvertFilterBase: public ConvertFilter
{
protected:
	IPresetManager& getManager () const;
};

//************************************************************************************************
// UrlToPresetFilter
//************************************************************************************************

class UrlToPresetFilter: public PresetConvertFilterBase
{
public:
	// IConvertFilter
	tbool CCL_API canConvert (IUnknown* object, UIDRef cid = kNullUID) const override;
	IUnknown* CCL_API convert (IUnknown* object, UIDRef cid = kNullUID) const override;
};

//************************************************************************************************
// FileDescriptorToPresetFilter
//************************************************************************************************

class FileDescriptorToPresetFilter: public PresetConvertFilterBase
{
public:
	// IConvertFilter
	tbool CCL_API canConvert (IUnknown* object, UIDRef cid = kNullUID) const override;
	IUnknown* CCL_API convert (IUnknown* object, UIDRef cid = kNullUID) const override;
};

//************************************************************************************************
// PresetConvertFilterBase
//************************************************************************************************

inline IPresetManager& PresetConvertFilterBase::getManager () const
{
	return System::GetPresetManager ();
}

//************************************************************************************************
// UrlToPresetFilter
//************************************************************************************************

tbool CCL_API UrlToPresetFilter::canConvert (IUnknown* object, UIDRef cid) const
{
	if(cid == ccl_iid<IPreset> ())
	{
		UnknownPtr<IUrl> url (object);
		if(url)
			return url->getProtocol () == CCLSTR ("class")
				|| getManager ().supportsFileType (url->getFileType ());
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IUnknown* CCL_API UrlToPresetFilter::convert (IUnknown* object, UIDRef cid) const
{
	ASSERT (cid == ccl_iid<IPreset> ())
	UnknownPtr<IUrl> url (object);
	if(url)
	{
		// plugin class url
		if(const IClassDescription* description = System::GetPlugInManager ().getClassDescription (*url))
			return ccl_as_unknown (NEW ObjectPreset (description));

		// preset file url
		return getManager ().openPreset (*url);
	}
	return nullptr;
}

//************************************************************************************************
// FileDescriptorToPresetFilter
//************************************************************************************************

tbool CCL_API FileDescriptorToPresetFilter::canConvert (IUnknown* object, UIDRef cid) const
{
	if(cid == ccl_iid<IPreset> ())
	{
		UnknownPtr<IFileDescriptor> descriptor (object);
		if(descriptor)
		{
			FileType fileType;
			descriptor->getFileType (fileType);
			return getManager ().supportsFileType (fileType);
		}
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IUnknown* CCL_API FileDescriptorToPresetFilter::convert (IUnknown* object, UIDRef cid) const
{
	ASSERT (cid == ccl_iid<IPreset> ())
	UnknownPtr<IFileDescriptor> descriptor (object);
	if(descriptor)
		return getManager ().openPreset (*descriptor);

	return nullptr;
}

//************************************************************************************************
// GetPresetsWork
//************************************************************************************************

class GetPresetsWork: public Object,
					  public Threading::AbstractWorkItem,
					  public AbstractProgressNotify
{
public:
	GetPresetsWork (IObserver* observer, IAttributeList* metaInfo);

	// IWorkItem
	void CCL_API cancel () override;
	void CCL_API work () override;

	// IProgressNotify
	tbool CCL_API isCanceled () override;

	CLASS_INTERFACE2 (IWorkItem, IProgressNotify, Object)

protected:
	IObserver* observer;
	SharedPtr<IAttributeList> metaInfo;
	bool canceled;
};

} // namespace CCL

using namespace CCL;
using namespace Persistence;

//************************************************************************************************
// PresetManager::FavoriteItem
//************************************************************************************************

class PresetManager::FavoriteItem: public Object
{
public:
	DECLARE_CLASS (FavoriteItem, Object)

	PROPERTY_OBJECT (Url, presetUrl, PresetUrl)
	PROPERTY_STRING (sortPath, SortPath)

	static void makePersistentUrl (Url& presetUrl, IAttributeList* metaInfo)
	{
		// store relative for native urls, keep full url for packages
		if(presetUrl.isNativePath ())
			PresetFilePrimitives::makeRelativePresetUrl (presetUrl, metaInfo);
	}

	// Object
	bool load (const Storage& storage) override
	{
		const Attributes& a = storage.getAttributes ();
		a.get (presetUrl, "url");
		a.get (sortPath, "sortPath");
		return true;
	}

	bool save (const Storage& storage) const override
	{
		Attributes& a = storage.getAttributes ();
		a.set ("url", presetUrl);
		a.set ("sortPath", sortPath);
		return true;
	}
};

DEFINE_CLASS_PERSISTENT (PresetManager::FavoriteItem, Object, "PresetFavorite")

//************************************************************************************************
// PresetManager::FavoritesList
// Inherits functionality for favorite folders, adds a flat list of favorite preset references
//************************************************************************************************

class PresetManager::FavoritesList: public SortFolderList
{
public:
	DECLARE_CLASS (FavoritesList, SortFolderList)

	FavoritesList ();

	FavoriteItem* getPresetItem (UrlRef url, bool create = false);
	void removePresetItem (FavoriteItem* item);

	const ObjectList& getFavoriteItems () const { return favoriteItems; }

	// SortFolderList
	bool load (const Storage& storage) override;
	bool save (const Storage& storage) const override;

private:
	ObjectList favoriteItems;
};

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_KERNEL_INIT_LEVEL (PresetManager, kSetupLevel)
{
	ObjectConverter::instance ().registerFilter (AutoPtr<ConvertFilter> (NEW UrlToPresetFilter));

	if(!System::IsInMainAppModule ()) // main module must register a real convert filter, this one only delegates to other converters in the main module
		ObjectConverter::instance ().registerFilter (AutoPtr<ConvertFilter> (NEW FileDescriptorToPresetFilter));
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// Strings
//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_XSTRINGS ("Presets")
	XSTRING (ScanningPresets, "Scanning Presets...")
	XSTRING (ScanningPlugIns, "Scanning Plug-Ins...")
	XSTRING (AskResetBlocklist, "Do you want to rescan blocked plug-ins next time you start $APPNAME?")
	XSTRING (PluginsNeedRestart, "Some plug-ins will be updated next time you start $APPNAME.")
	XSTRING (AskRemovePlugInSettings, "Do you want to remove all plug-in settings and perform a full rescan next time you start $APPNAME?")
END_XSTRINGS

//************************************************************************************************
// GetPresetsWork
//************************************************************************************************

GetPresetsWork::GetPresetsWork (IObserver* observer, IAttributeList* metaInfo)
: AbstractWorkItem (observer),
  observer (observer),
  metaInfo (metaInfo),
  canceled (false)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API GetPresetsWork::cancel ()
{
	canceled = true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API GetPresetsWork::isCanceled ()
{
	return canceled;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API GetPresetsWork::work ()
{
	AutoPtr<IUnknownList> presets = PresetManager::instance ().getPresets (metaInfo, this);
	if(presets && !canceled)
	{
		Message* m = NEW Message (Signals::kGetPresetsCompleted, static_cast<IUnknownList*> (presets));
		m->post (observer);
	}
}

//************************************************************************************************
// PresetManager::FavoritesList
//************************************************************************************************

DEFINE_CLASS_PERSISTENT (PresetManager::FavoritesList, SortFolderList, "PresetFavoritesList")

//////////////////////////////////////////////////////////////////////////////////////////////////

PresetManager::FavoritesList::FavoritesList ()
{
	favoriteItems.objectCleanup (true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

PresetManager::FavoriteItem* PresetManager::FavoritesList::getPresetItem (UrlRef url, bool create)
{
	for(auto item : iterate_as<FavoriteItem> (favoriteItems))
		if(item->getPresetUrl () == url)
			return item;

	if(create)
	{
		auto item = NEW FavoriteItem;
		item->setPresetUrl (url);
		favoriteItems.add (item);
		return item;
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PresetManager::FavoritesList::removePresetItem (FavoriteItem* item)
{
	if(favoriteItems.remove (item))
		item->release ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PresetManager::FavoritesList::load (const Storage& storage)
{
	storage.getAttributes ().unqueue (favoriteItems, "favorites", ccl_typeid<FavoriteItem> ());
	return loadFolders (storage, "class");
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PresetManager::FavoritesList::save (const Storage& storage) const
{
	storage.getAttributes ().queue ("favorites", favoriteItems);
	return saveFolders (storage, "class");
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// Commands
//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_COMMANDS (PresetManager)
	DEFINE_COMMAND_ ("Presets", "Re-Index Presets", PresetManager::onScanPresets, CommandFlags::kGlobal)
	DEFINE_COMMAND_ ("Presets", "Update Plug-In List", PresetManager::onScanPlugIns, CommandFlags::kGlobal)
	DEFINE_COMMAND_ ("Presets", "Reset Blocklist", PresetManager::onResetBlocklist, CommandFlags::kGlobal)
	DEFINE_COMMAND_ ("Presets", "Remove Plug-In Settings", PresetManager::onRemovePlugInSettings, CommandFlags::kGlobal)
END_COMMANDS (PresetManager)

//************************************************************************************************
// PresetManager
//************************************************************************************************

static Threading::IThreadPool& getPresetWorker ()
{
#if 1
	static AutoPtr<Threading::IThreadPool> theWorker;
	if(theWorker == nullptr)
		theWorker = System::CreateThreadPool ({1, Threading::kPriorityBelowNormal, "PresetWorker"});
	return *theWorker;
#else
	return System::GetThreadPool ();
#endif
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_CLASS (PresetManager, Component)
DEFINE_CLASS_NAMESPACE (PresetManager, NAMESPACE_CCL)
DEFINE_COMPONENT_SINGLETON (PresetManager)
IMPLEMENT_COMMANDS (PresetManager, Component)

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PresetManager::needFullRescan = false;
bool PresetManager::showProgressDialog = true;
const String PresetManager::kSettingsName ("PresetManager");

//////////////////////////////////////////////////////////////////////////////////////////////////

void PresetManager::forceFullScanOnStartup ()
{
	needFullRescan = true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PresetManager::suppressProgressDialog (bool suppress)
{
	showProgressDialog = !suppress;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringRef PresetManager::getScanningPresetsText ()
{
	return XSTR (ScanningPresets);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

PresetManager::PresetManager ()
: Component (CCLSTR ("PresetManager")),
  fileSystemSink (Signals::kFileSystem),
  presetsSignal (Signals::kPresetManager),
  presetStore (nullptr),
  folderSignalSuspended (false)
{
	infoCache.objectCleanup (true);
	presetFavorites.setListClass (ccl_typeid<FavoritesList> ());

	fileSystemSink.setObserver (this);
	fileSystemSink.enable (true);

	loadSettings ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

PresetManager::~PresetManager ()
{
	fileSystemSink.enable (false);
	cancelSignals ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API PresetManager::terminate ()
{
	saveSettings ();

	delete presetStore;
	presetStore = nullptr;

	return SuperClass::terminate ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PresetManager::getSettingsPath (IUrl& path)
{
	path.assign (XmlSettings (kSettingsName).getPath ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PresetManager::loadSettings ()
{
	XmlSettings settings (kSettingsName);
	settings.checkVersion (false);
	if(settings.restore ())
		presetFavorites.restore (settings.getAttributes ("favorites"));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PresetManager::saveSettings ()
{
	XmlSettings settings (kSettingsName);
	presetFavorites.store (settings.getAttributes ("favorites"));
	// note: sortFolders (additional empty folders) are not stored - consider this as a kind of cleanup

	settings.flush ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API PresetManager::presetExists (IAttributeList* metaInfo, StringRef name, const FileType* fileType)
{
	return presetStore && presetStore->presetExists (metaInfo, name, fileType);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API PresetManager::hasPresets (IAttributeList* metaInfo)
{
	if(!presetStore)
		return false;

	Threading::ScopedLock guard (lock);
	return presetStore->hasPresets (*metaInfo) ? true : false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API PresetManager::getPresetsInBackground (IObserver* observer, IAttributeList* metaInfo)
{
	ASSERT (observer && metaInfo)
	getPresetWorker ().scheduleWork (NEW GetPresetsWork (observer, metaInfo));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API  PresetManager::cancelGetPresets (IObserver* observer)
{
	ASSERT (observer)
	getPresetWorker ().cancelWork (observer, true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IUnknownList* CCL_API PresetManager::getPresets (IAttributeList* metaInfo, IProgressNotify* progress)
{
	if(!presetStore)
		return nullptr;

	UnknownPtr<Threading::ILockProvider> lockProvider (&System::GetPresetFileRegistry ());
	Threading::ILockable* lockable = lockProvider ? lockProvider->getLock () : nullptr;
	ASSERT (lockable)

	// scan preset locations from all handlers (might be called from a background thread!)
	Threading::AutoLock autoLock (lockable);

	return presetStore->getPresets (metaInfo, progress);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PresetManager::collectSubFolders (IMutableArray& subFolders, const IAttributeList* metaInfo)
{
	if(presetStore)
		presetStore->collectSubFolders (subFolders, metaInfo);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IPreset* CCL_API PresetManager::openPreset (UrlRef url)
{
	IPresetFileHandler* handler = System::GetPresetFileRegistry ().getHandlerForFile (url);
	return handler ? openPreset (*handler, url) : nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IPreset* CCL_API PresetManager::openPreset (const IFileDescriptor& descriptor)
{
	return ObjectConverter::toInterface<IPreset> (&const_cast<IFileDescriptor&> (descriptor));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API PresetManager::supportsFileType (const FileType& fileType)
{
	return System::GetPresetFileRegistry ().getHandlerForFileType (fileType) != nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IPreset* PresetManager::openPreset (IPresetFileHandler& handler, UrlRef url)
{
	// lookup in store
	AutoPtr<PresetDescriptor> descriptor;
	if(presetStore)
	{
		Url storeUrl (url);
		PresetUrl::removeSubPresetIndex (storeUrl);
		descriptor = presetStore->getPresetDescriptor (storeUrl);
	}

	IPreset* preset = handler.openPreset (url, descriptor);
	if(preset)
	{
		// add to store if necessary
		if(presetStore)
		{
			if(descriptor)
				descriptor->applySubFolder (*preset);
			else if(isInManagedFolder (url))
			{
				if(IAttributeList* metaInfo = preset->getMetaInfo ())
				{
					String subFolder (PresetFilePrimitives::determineRelativeSubFolder (handler, *metaInfo, url));
					PresetMetaAttributes (*metaInfo).setSubFolder (subFolder);
				}
				presetStore->addPreset (url, *preset);
			}
		}

		// reference subpreset of collection...
		UnknownPtr<IPresetCollection> collection (preset);
		if(collection)
		{
			// 1) try preset index
			int presetIndex = PresetUrl::getSubPresetIndex (url);
			if(presetIndex >= 0)
			{
				IPreset* subPreset = collection->openPreset (presetIndex);
				preset->release ();
				return subPreset;
			}
			// 2) try other parameters
			else if(url.getParameters ().countEntries () > 0)
			{
				IPreset* subPreset = collection->openPreset (url.getParameters ());
				preset->release ();
				return subPreset;
			}
		}
	}
	return preset;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IPreset* CCL_API PresetManager::openDefaultPreset (IPresetFileHandler& handler, IAttributeList* metaInfo)
{
	// try all read locations of handler (first is the write location)
	Url url;
	int i = 0;
	while(handler.getReadLocation (url, metaInfo, i++))
	{
		url.descend (PresetFilePrimitives::kDefaultPresetFileName);
		url.setFileType (handler.getFileType (), false); // allow dots in the preset name

		if(System::GetFileSystem ().fileExists (url))
			if(IPreset* preset = openPreset (handler, url))
				return preset;
	}

	if(!presetStore)
		return nullptr;

	// query database as fallback: this also finds a default preset in an external location not managed by the given handler
	Expression condition = presetStore->makeClassCondition (*metaInfo)
		&& Member ("title") == PresetFilePrimitives::kDefaultPresetFileName
		&& Member ("subFolder") == "";

	AutoPtr<Iterator> iter (presetStore->query (condition));
	for(auto descriptor : iterate_as<PresetDescriptor> (*iter))
		return openPreset (descriptor->getUrl ());

	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PresetManager::isInManagedFolder (UrlRef url)
{
	IPresetFileRegistry& registry (System::GetPresetFileRegistry ());
	for(int h = 0, numhandlers = registry.countHandlers (); h < numhandlers; h++)
	{
		IPresetFileHandler* handler = registry.getHandler (h);
		int i = 0;
		Url folder;
		while(handler->getReadLocation (folder, nullptr, i++))
			if(folder.contains (url))
				return true;
	}
	return false;
}


//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API PresetManager::onPresetCreated (UrlRef url, IPreset& preset)
{
	if(isInManagedFolder (url))
	{
		if(presetStore)
			presetStore->onPresetCreated (url, preset);

		presetsSignal.signal (Message (Signals::kPresetCreated, &preset));
	}

	if(url.isNativePath ())
	{
		IUrl* u = const_cast<IUrl*> (&url);
		SignalSource (Signals::kFileSystem).signal (Message (Signals::kFileCreated, u));
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API PresetManager::onPresetRemoved (UrlRef url, IPreset& preset)
{
	if(presetStore)
		presetStore->onPresetRemoved (url, preset);

	presetsSignal.signal (Message (Signals::kPresetRemoved, &preset));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API PresetManager::removePreset (IPreset& preset)
{
	ASSERT (!preset.isReadOnly ())

	Url url;
	if(preset.getUrl (url))
	{
		if(System::GetFileSystem ().fileExists (url))
		{
			if(!System::GetFileSystem ().removeFile (url, IFileSystem::kDeleteToTrashBin))
				return false;
		}
		onPresetRemoved (url, preset);
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PresetManager::movePresetInternal (IPreset& preset, UrlRef newUrl, UrlRef oldUrl, const String* newSubFolder)
{
	if(System::GetFileSystem ().moveFile (newUrl, oldUrl))
	{
		IAttributeList* oldMetaInfo = preset.getMetaInfo ();

		// note: new file is added to database in the following call
		AutoPtr<IPreset> newPreset (System::GetPresetManager ().openPreset (newUrl));
		if(newPreset)
		{
			IAttributeList* newMetaInfo = newPreset->getMetaInfo ();
			ASSERT (newMetaInfo && oldMetaInfo)
			if(newMetaInfo && oldMetaInfo)
			{
				// copy meta info from old preset but update to new title and subFolder
				newMetaInfo->copyFrom (*oldMetaInfo);
				PresetMetaAttributes (*newMetaInfo).setTitle (newPreset->getPresetName ());
				if(newSubFolder)
					PresetMetaAttributes (*newMetaInfo).setSubFolder (*newSubFolder);

				// update reference in favorite item
				if(FavoriteItem* item = getFavoriteItem (preset, false))
				{
					Url url (newUrl);
					FavoriteItem::makePersistentUrl (url, newMetaInfo);
					item->setPresetUrl (url);

					signalFavoritesChanged (*newMetaInfo);
				}
			}

			onPresetRemoved (oldUrl, preset);
			onPresetCreated (newUrl, *newPreset);
		}
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API PresetManager::renamePreset (IPreset& preset, StringRef newName, IUrl* _newUrl)
{
	ASSERT (!preset.isReadOnly ())

	Url oldUrl;
	if(preset.getUrl (oldUrl))
	{
		// folder presets might have an extension
		String ext;
		if(oldUrl.getType () == IUrl::kFolder)
			oldUrl.getExtension (ext);

		// build new url
		FileType ft (oldUrl.getFileType ());

		Url newUrl (oldUrl);
		newUrl.setName (LegalFileName (newName));
		newUrl.setFileType (ft, false);

		if(ext.isEmpty () == false && ext != ft.getExtension ())
			newUrl.setExtension (ext);

		if(System::GetFileSystem ().fileExists (newUrl))
		{
			bool canRename = false;
			if(!System::GetFileSystem ().isCaseSensitive ())
			{
				// allow changing only upper/lower case in a case insensitive file system (the existing new url is the same file)
				String oldFileName, newFileName;
				oldUrl.getName (oldFileName, false);
				newUrl.getName (newFileName, false);
				if(oldFileName.compare (newName, false) == 0 && oldFileName != newFileName)
					canRename = true;
			}
			if(!canRename)
				return false;
		}

		if(_newUrl)
			*_newUrl = newUrl;

		// rename the file
		return movePresetInternal (preset, newUrl, oldUrl);
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API PresetManager::movePreset (IPreset& preset, StringRef newSubfolder)
{
	Url oldUrl;
	preset.getUrl (oldUrl);

	IAttributeList* metaInfo = preset.getMetaInfo ();
	IPresetFileHandler* handler = System::GetPresetFileRegistry ().getHandlerForFile (oldUrl);
	if(metaInfo && handler)
	{
		String fileName;
		oldUrl.getName (fileName);

		String oldSubFolder (PresetFilePrimitives::determineRelativeSubFolder (*handler, *metaInfo, oldUrl));
		if(oldSubFolder == newSubfolder)
			return false;

		Url newUrl (oldUrl);
		newUrl.ascend ();

		if(preset.isReadOnly ())
		{
			// *copy* into default location instead
			PresetFilePrimitives::getWriteLocation (newUrl, oldUrl.getFileType (), metaInfo);

			newUrl.descend (newSubfolder, Url::kFolder);
			newUrl.descend (fileName);
			newUrl.makeUnique ();

			if(System::GetFileSystem ().copyFile (newUrl, oldUrl, INativeFileSystem::kDisableWriteProtection))
			{
				AutoPtr<IPreset> newPreset (System::GetPresetManager ().openPreset (newUrl));
				if(newPreset)
					onPresetCreated (newUrl, *newPreset);

				return true;
			}
		}
		else
		{
			// ascend to base folder, descend into new sub folder
			Url oldRelativeUrl;
			oldRelativeUrl.descend (oldSubFolder);
			while(oldRelativeUrl.ascend ())
				newUrl.ascend ();

			newUrl.descend (newSubfolder, Url::kFolder);
			newUrl.descend (fileName);
			newUrl.makeUnique ();

			return movePresetInternal (preset, newUrl, oldUrl, &newSubfolder);
		}
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

SortFolderList* PresetManager::getSortFolderList (const IAttributeList& metaInfo) const
{
	if(!presetStore)
		return nullptr;

	String classId (presetStore->getClassKey (metaInfo));
	ASSERT (!classId.isEmpty ());
	return sortFolders.getSortFolderList (classId);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IUnknownIterator* CCL_API PresetManager::getSortFolders (const IAttributeList& metaInfo) const
{
	if(SortFolderList* list = getSortFolderList (metaInfo))
		return list->newIterator ();
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API PresetManager::hasSortFolder (const IAttributeList& metaInfo, StringRef path) const
{
	// explicit folders
	if(SortFolderList* folderList = getSortFolderList (metaInfo))
		if(folderList->contains (path))
			return true;

	if(!presetStore)
		return false;

	// subfolders in preset database
	VariantVector subFolders;
	ccl_const_cast (presetStore)->collectSubFolders (subFolders, &metaInfo);
	for(auto& v : subFolders)
	{
		String subFolderString (v.asString ());
		if(subFolderString == path)
			return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API PresetManager::addSortFolder (const IAttributeList& metaInfo, StringRef path)
{
	if(SortFolderList* folderList = getSortFolderList (metaInfo))
	{
		folderList->addOnce (SortFolderList::makeLegalFolderPath (path));

		signalSubFoldersChanged (metaInfo, Signals::kPresetSubFolderAdded, path);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PresetManager::movePresetsInternal (const IAttributeList& metaInfo, StringRef sourceFolder, StringRef targetFolder, bool isRemove)
{
	if(!presetStore)
		return;

	// sub folders of sourceFolder start with "sourceFolder/"
	String subFolderPrefix (sourceFolder);
	if(!sourceFolder.endsWith (Url::strPathChar))
		subFolderPrefix << Url::strPathChar;

	String subFolderPattern (subFolderPrefix);
	subFolderPattern << "%";

	// get affected presets from store (in sourceFolder or subFolders)
	Expression condition = presetStore->makeClassCondition (metaInfo)
		&& (Member ("subFolder") == sourceFolder || Member ("subFolder").like (subFolderPattern));

	ObjectList descriptors;
	descriptors.objectCleanup (true);
	AutoPtr<Iterator> iter (presetStore->query (condition));
	for(auto descriptor : iterate_as<PresetDescriptor> (*iter))
		descriptors.add (return_shared (descriptor));

	ObjectArray folderUrls;
	folderUrls.objectCleanup (true);
	for(auto descriptor : iterate_as<PresetDescriptor> (descriptors))
	{
		AutoPtr<IPreset> preset (openPreset (descriptor->getUrl ()));
		if(preset)
		{
			String subFolder (descriptor->getSubFolder ());
			ASSERT (subFolder == sourceFolder || subFolder.startsWith (subFolderPrefix))
			if(subFolder == sourceFolder || subFolder.startsWith (subFolderPrefix))
			{
				String newSubFolder (targetFolder);
				Url subFolderUrl (subFolder);

				// collect source folders to be removed if finally empty (order deepest path first for correct empty-check below)
				Url parentFolder (descriptor->getUrl ());
				do
				{
					parentFolder.ascend ();
					folderUrls.addSorted (NEW Url (parentFolder), LAMBDA_ARRAY_COMPARE (Url, u1, u2) return u2->getPath ().length () - u1->getPath ().length (); });
				}
				while(subFolderUrl.ascend ()); // also try parent folders that are sort-subFolders

				if(!isRemove)
					newSubFolder << subFolder.subString (sourceFolder.length ()); // keep the internal folder structure inside moved folders

				movePreset (*preset, newSubFolder);
			}
		}
	}

	// remove empty source folders
	for(auto folder : iterate_as<Url> (folderUrls))
		if(File::isFolderEmpty (*folder))
			File (*folder).remove ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API PresetManager::removeSortFolder (const IAttributeList& metaInfo, StringRef path)
{
	// move contained presets to parent folder (or root)
	String targetFolder (SortFolderList::getParentFolder (path));
	movePresetsInternal (metaInfo, path, targetFolder, true);

	if(SortFolderList* folderList = getSortFolderList (metaInfo))
		folderList->removeFolder (path);

	signalSubFoldersChanged (metaInfo, Signals::kPresetSubFolderRemoved, path);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API PresetManager::moveSortFolder (const IAttributeList& metaInfo, StringRef oldPath, StringRef _newPath)
{
	String newPath (SortFolderList::makeLegalFolderPath (_newPath));

	{
		ScopedVar<bool> scope (folderSignalSuspended, true);
		movePresetsInternal (metaInfo, oldPath, newPath); // note: read-only presets might be copied instead of moved!
	}

	// update sort folder and all subFolders
	if(SortFolderList* folderList = getSortFolderList (metaInfo))
		folderList->moveSortFolder (oldPath, newPath);

	signalSubFoldersChanged (metaInfo, Signals::kPresetSubFolderRemoved, oldPath);
	signalSubFoldersChanged (metaInfo, Signals::kPresetSubFolderAdded, newPath);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API PresetManager::renameSortFolder (const IAttributeList& metaInfo, StringRef path, StringRef newName)
{
	String newPath (SortFolderList::getParentFolder (path));
	if(!newPath.isEmpty ())
		newPath	<< Url::strPathChar;
	newPath << newName;

	moveSortFolder (metaInfo, path, SortFolderList::makeLegalFolderPath (newPath));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PresetManager::signalSubFoldersChanged (const IAttributeList& metaInfo, StringID msgId, StringRef path)
{
	if(!folderSignalSuspended && presetStore)
		presetsSignal.signal (Message (msgId, presetStore->getClassKey (metaInfo), path));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

PresetManager::FavoritesList* PresetManager::getFavoritesList (const IPreset& preset, bool create) const
{
	IAttributeList* metaInfo = preset.getMetaInfo ();
	return metaInfo ? getFavoritesList (*metaInfo, create) : nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

PresetManager::FavoritesList* PresetManager::getFavoritesList (const IAttributeList& metaInfo, bool create) const
{
	if(!presetStore)
		return nullptr;

	String classId (presetStore->getClassKey (metaInfo));
	ASSERT (!classId.isEmpty ());
	return static_cast<FavoritesList*> (presetFavorites.getSortFolderList (classId, create));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

PresetManager::FavoriteItem* PresetManager::getFavoriteItem (const IPreset& preset, bool create) const
{
	if(FavoritesList* list = getFavoritesList (preset, create))
	{
		Url url;
		if(preset.getUrl (url))
		{
			FavoriteItem::makePersistentUrl (url, preset.getMetaInfo ());
			return list->getPresetItem (url, create);
		}
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PresetManager::removeFavoriteItem (const IPreset& preset)
{
	if(FavoritesList* list = getFavoritesList (preset, false))
	{
		Url url;
		if(preset.getUrl (url))
		{
			FavoriteItem::makePersistentUrl (url, preset.getMetaInfo ());
			if(FavoriteItem* item = list->getPresetItem (url, false))
				list->removePresetItem (item);
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API PresetManager::isFavorite (const IPreset& preset) const
{
	return getFavoriteItem (preset, false) != nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringRef CCL_API PresetManager::getFavoriteFolder (const IPreset& preset) const
{
	FavoriteItem* item = getFavoriteItem (preset, false);
	return item ? item->getSortPath () : String::kEmpty;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API PresetManager::setFavorite (const IPreset& preset, tbool state, StringRef folder)
{
	if(state)
	{
		FavoriteItem* item = getFavoriteItem (preset, state);
		ASSERT (item)
		if(item)
			item->setSortPath (folder);
	}
	else
		removeFavoriteItem (preset);

	if(IAttributeList* metaInfo = preset.getMetaInfo ())
		signalFavoritesChanged (*metaInfo);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IPreset* PresetManager::openFavoritePreset (const FavoriteItem& item, const IAttributeList& metaInfo)
{
	if(!presetStore)
		return nullptr;

	String classId (presetStore->getClassKey (metaInfo));

	auto makeUrlCondition = [&] () -> Expression
	{
		Url url (item.getPresetUrl ()); // relative to location
		if(url.isRelative ())
		{
			url.normalize (IUrl::kRemoveDotSegments);

			// search pattern: "protocol://%s/relativePath"
			url = Url (url.getProtocol (), "%", url.getPath ());
			UrlFullString urlPattern (url);
			return Member ("url").like (urlPattern);
		}
		else
		{
			// package urls must match exactly
			UrlFullString urlString (url);
			return Member ("url") == urlString;
		}
	};

	Expression condition = Member ("classID") == classId && makeUrlCondition ();
	AutoPtr<Iterator> iter (presetStore->query (condition));
	for(auto descriptor : iterate_as<PresetDescriptor> (*iter))
		if(IPreset* preset = openPreset ((descriptor->getUrl ())))
			return preset;
				
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IUnknownIterator* CCL_API PresetManager::getFavoritePresets (const IAttributeList& metaInfo)
{
	class PresetList: public UnknownList,
					  public IUnknownIterator
	{
	public:
		PresetList ()
		: iter (list)
		{}

		void first () { iter.first (); }

		// IUnknownIterator
		tbool CCL_API done () const override { return iter.done (); }
		IUnknown* CCL_API nextUnknown () override { return iter.next (); }

		CLASS_INTERFACE (IUnknownIterator, UnknownList)

	private:
		ListIterator<IUnknown*> iter;
	};

	if(FavoritesList* favoritesList = getFavoritesList (metaInfo, false))
	{
		PresetList* presetList = NEW PresetList;

		for(auto item : iterate_as<FavoriteItem> (favoritesList->getFavoriteItems ()))
			if(IPreset* preset = openFavoritePreset (*item, metaInfo))
				presetList->add (preset);

		presetList->first ();
		return presetList;
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API PresetManager::addFavoriteFolder (const IAttributeList& metaInfo, StringRef path)
{
	String folder (SortFolderList::makeLegalFolderPath (path));
	getFavoritesList (metaInfo, true)->addOnce (folder);

	signalFavoritesChanged (metaInfo, folder);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API PresetManager::removeFavoriteFolder (const IAttributeList& metaInfo, StringRef path)
{
	if(FavoritesList* favoritesList = getFavoritesList (metaInfo, false))
	{
		String subFolderPrefix (path);
		subFolderPrefix << Url::strPathChar;

		// move contained favorites to parent folder (or root level)
		String newSortPath (SortFolderList::getParentFolder (path));

		for(auto item : iterate_as<FavoriteItem> (favoritesList->getFavoriteItems ()))
		{
			StringRef favPath (item->getSortPath ());
			if(favPath == path || favPath.startsWith (subFolderPrefix))
				item->setSortPath (newSortPath);
		}

		// remove the sort folder and it's subFolders
		favoritesList->removeFolder (path);

		signalFavoritesChanged (metaInfo);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API PresetManager::moveFavoriteFolder (const IAttributeList& metaInfo, StringRef oldPath, StringRef _newPath)
{
	if(FavoritesList* favoritesList = getFavoritesList (metaInfo, false))
	{
		String newPath (SortFolderList::makeLegalFolderPath (_newPath));

		String subFolderPrefix (oldPath);
		subFolderPrefix << Url::strPathChar;

		// update sort folder of affected favorite items
		for(auto item : iterate_as<FavoriteItem> (favoritesList->getFavoriteItems ()))
		{
			StringRef favPath (item->getSortPath ());
			if(favPath == oldPath || favPath.startsWith (subFolderPrefix))
			{
				String newFavPath (newPath);
				newFavPath << favPath.subString (oldPath.length ());
				item->setSortPath (newFavPath);
			}
		}

		// update sort folder and all subFolders
		favoritesList->moveSortFolder (oldPath, newPath);

		signalFavoritesChanged (metaInfo);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API PresetManager::renameFavoriteFolder (const IAttributeList& metaInfo, StringRef path, StringRef newName)
{
	String newPath (SortFolderList::getParentFolder (path));
	if(!newPath.isEmpty ())
		newPath	<< Url::strPathChar;
	newPath << newName;

	moveFavoriteFolder (metaInfo, path, SortFolderList::makeLegalFolderPath (newPath));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IUnknownIterator* CCL_API PresetManager::getFavoriteFolders (const IAttributeList& metaInfo) const
{
	FavoritesList* list = getFavoritesList (metaInfo, false);
	return list ? list->newIterator () : nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API PresetManager::hasFavoriteFolder (const IAttributeList& metaInfo, StringRef path) const
{
	if(FavoritesList* list = getFavoritesList (metaInfo, false))
	{
		if(path.isEmpty ()) // "has any favorites"
			return !list->isEmpty () || !list->getFavoriteItems ().isEmpty ();

		// explicit folders
		if(list->contains (path))
			return true;

		// folders of favorite items
		for(auto item : iterate_as<FavoriteItem> (list->getFavoriteItems ()))
			if(item->getSortPath () == path)
				return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PresetManager::signalFavoritesChanged (const IAttributeList& metaInfo, StringRef folder)
{
	if(presetStore)
		presetsSignal.deferSignal (NEW Message (Signals::kPresetFavoritesChanged, presetStore->getClassKey (metaInfo), folder));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ISearcher* CCL_API PresetManager::createSearcher (ISearchDescription& description)
{
	if(!presetStore)
		return nullptr;

	return presetStore->createSearcher (description);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API PresetManager::scanPresets (tbool onlyChangedLocations)
{
	if(!presetStore)
		return;

	AutoPtr<IProgressNotify> progress;
	if(showProgressDialog)
	{
		progress = ccl_new<IProgressNotify> (ClassID::ProgressDialog);
		progress->setTitle (XSTR (ScanningPresets));
	}

	UnknownPtr<IProgressDialog> dialog (progress);
	if(dialog)
		dialog->setOpenDelay (0.3);

	{
		ProgressNotifyScope progressScope (progress);

		// rescan all preset locations
		PresetStoreSynchronizer synchronizer (*presetStore);
		synchronizer.scanLocations (progress, onlyChangedLocations != 0);
	}

	presetsSignal.signal (Message (Signals::kPresetsRefreshed));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PresetManager::setPresetRevision (int revision)
{
	if(!presetStore)
		return;

	Variant storeRevision (-1);
	presetStore->getDataStore ().getMetaInfo (storeRevision, "presetRevision");

	if(storeRevision.asInt () < revision)
	{
		presetStore->getDataStore ().setMetaInfo ("presetRevision", revision);
		needFullRescan = true;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PresetManager::startup ()
{
	ASSERT (!presetStore)
	presetStore = NEW PresetStore;

	#if 1
	bool onlyChangedLocations = !needFullRescan; // only added/removed root locations (e.g. soundsets), except if a full scan is reqested
	scanPresets (onlyChangedLocations);
	#else
	if(presetStore.isEmpty ())
		scanPresets (false);
	#endif
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API PresetManager::notify (ISubject* subject, MessageRef msg)
{
	if(msg == Signals::kFileMoved)
	{
		UnknownPtr<IUrl> oldPath (msg.getArg (0).asUnknown ());
		UnknownPtr<IUrl> newPath (msg.getArg (1).asUnknown ());
		bool succeeded = msg.getArgCount () > 2 ? msg.getArg (2).asBool () : true;
		if(oldPath && newPath && succeeded)
		{
			if(oldPath->isFolder () && presetStore && isInManagedFolder (*newPath))
			{
				// a folder (possibly containing presets) was moved / renamed:
				// remove old folder's presets from store
				IPresetFileRegistry& registry (System::GetPresetFileRegistry ());
				AutoPtr<Iterator> iterator (presetStore->queryFolderDeep (*oldPath));
				for(auto* descriptor : iterate_as<PresetDescriptor> (*iterator))
					if(IPresetFileHandler* handler = registry.getHandlerForFile (descriptor->getUrl ()))
					{
						AutoPtr<IPreset> preset (handler->openPreset (descriptor->getUrl (), descriptor));
						if(preset)
							onPresetRemoved (descriptor->getUrl (), *preset);
					}

				// add presets from new folder
				onFolderAdded (*newPath);
			}
		}
	}
	SuperClass::notify (subject, msg);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PresetManager::onFolderAdded (UrlRef folder, IProgressNotify* progress)
{
	ForEachFile (System::GetFileSystem ().newIterator (folder, IFileIterator::kAll), p)
		if(progress)
		{
			progress->updateAnimated ();
			if(progress->isCanceled ())
				break;
		}

		if(p->isFolder ())
			onFolderAdded (*p);
		else
		{
			AutoPtr<IPreset> newPreset (openPreset (*p));
			if(newPreset)
				onPresetCreated (*p, *newPreset);
		}
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PresetManager::onScanPresets (CmdArgs args)
{
	if(!args.checkOnly ())
		scanPresets (false);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PresetManager::onScanPlugIns (CmdArgs args)
{
	if(!args.checkOnly ())
	{
		UnknownList restartList;
		{
			AutoPtr<IProgressNotify> progress = ccl_new<IProgressNotify> (ClassID::ProgressDialog);
			UnknownPtr<IProgressDialog> (progress)->setOpenDelay (1.);
			progress->setTitle (XSTR (ScanningPlugIns));
			// Please note that caller has to open progress dialog explicitly

			SignalSource (Signals::kPlugIns).signal (Message (Signals::kRescanPlugIns,
													static_cast<IProgressNotify*> (progress),
													static_cast<IUnknownList*> (&restartList)));
		}

		if(!restartList.isEmpty ())
		{
			String message (XSTR (PluginsNeedRestart));
			message << "\n\n";
			int count = 0;
			ForEachUnknown (restartList, unk)
				if(UnknownPtr<IUrl> url = unk)
				{
					String name;
					url->getName (name, false);
					message << name << "\n";
					if(++count > 5)
						break;
				}
			EndFor

			Alert::info (message);
		}
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PresetManager::onResetBlocklist (CmdArgs args)
{
	if(!args.checkOnly ())
	{
		if(Alert::ask (XSTR (AskResetBlocklist)) == Alert::kYes)
			SignalSource (Signals::kPlugIns).signal (Message (Signals::kResetBlocklist));
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PresetManager::onRemovePlugInSettings (CmdArgs args)
{
	if(!args.checkOnly ())
	{
		bool yes = Alert::ask (XSTR (AskRemovePlugInSettings)) == Alert::kYes;
		PlugInSettingsHelper::makeRemoveMarker (yes); // user can revoke decision by saying "no" later
		if(yes)
			SignalSource (Signals::kApplication).deferSignal (NEW Message (Signals::kRequestRestart));
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_METHOD_NAMES (PresetManager)
	DEFINE_METHOD_ARGR ("presetExists", "metaInfo: Attributes, presetName: string, fileType: FileType = null", "bool")
END_METHOD_NAMES (PresetManager)

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API PresetManager::invokeMethod (Variant& returnValue, MessageRef msg)
{
	if(msg == "presetExists")
	{
		// Meta info, required - see presetExists() for relevant attributes.
		AutoPtr<Attributes> metaInfo;
		if(UnknownPtr<IAttributeList> metaInfoArg = msg.getArg (0).asUnknown ())
		{
			metaInfo = NEW Attributes;
			metaInfo->copyFrom (*metaInfoArg);
		}
		ASSERT (metaInfo && metaInfo->isEmpty () == false)

		// Preset name, required.
		String presetName = msg.getArg (1);
		ASSERT (presetName.isEmpty () == false)

		// File type, optional.
		Boxed::FileType* fileType = nullptr;
		if(msg.getArgCount () > 2)
		{
			fileType = unknown_cast<Boxed::FileType> (msg[2].asUnknown ());
			ASSERT (fileType != nullptr)
		}

		returnValue = presetExists (metaInfo, presetName, fileType);
		return true;
	}
	else
		return SuperClass::invokeMethod (returnValue, msg);
}
