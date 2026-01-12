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
// Filename    : ccl/app/presets/presetmanager.h
// Description : Preset Manager
//
//************************************************************************************************

#include "ccl/app/component.h"
#include "ccl/app/presets/presetstore.h"
#include "ccl/app/utilities/sortfolderlist.h"

#include "ccl/base/signalsource.h"
#include "ccl/base/collections/objectarray.h"

#include "ccl/public/system/threadsync.h"
#include "ccl/public/gui/commanddispatch.h"

#include "ccl/public/app/ipreset.h"

#ifndef _ccl_presetmanager_h
#define _ccl_presetmanager_h

namespace CCL {

interface IUnknownList;
interface IAttributeList;
interface IProgressNotify;
interface IUrlFilter;
interface IPresetFileHandler;
interface IPreset;
interface ISearcher;
interface ISearchDescription;

class PresetMetaAttributes;

//************************************************************************************************
// PresetManager
//************************************************************************************************

class PresetManager: public Component,
					 public IPresetManager,
					 public ComponentSingleton<PresetManager>,
					 public CommandDispatcher<PresetManager>
{
public:
	DECLARE_CLASS (PresetManager, Component)
	DECLARE_METHOD_NAMES (PresetManager)

	PresetManager ();
	~PresetManager ();

	void setPresetRevision (int revision);
	void startup ();	///< enables preset store; might do an initial preset scan
	static void forceFullScanOnStartup ();
	static void suppressProgressDialog (bool suppress = true);
	static StringRef getScanningPresetsText ();

	// IPresetManager
	IPreset* CCL_API openPreset (UrlRef url) override;
	IPreset* CCL_API openPreset (const IFileDescriptor& descriptor) override;
	IPreset* CCL_API openDefaultPreset (IPresetFileHandler& handler, IAttributeList* metaInfo) override;

	tbool CCL_API supportsFileType (const FileType& fileType) override;

	IUnknownList* CCL_API getPresets (IAttributeList* metaInfo, IProgressNotify* progress) override;
	void CCL_API getPresetsInBackground (IObserver* observer, IAttributeList* metaInfo) override;
	void CCL_API cancelGetPresets (IObserver* observer) override;

	void CCL_API collectSubFolders (IMutableArray& subFolders, const IAttributeList* metaInfo) override;

	ISearcher* CCL_API createSearcher (ISearchDescription& description) override;
	tbool CCL_API presetExists (IAttributeList* metaInfo, StringRef name, const FileType* fileType = nullptr) override;
	tbool CCL_API hasPresets (IAttributeList* metaInfo) override;
	tbool CCL_API removePreset (IPreset& preset) override;
	tbool CCL_API renamePreset (IPreset& preset, StringRef newName, IUrl* newUrl = nullptr) override;

	void CCL_API scanPresets (tbool onlyChangedLocations) override;

	void CCL_API onPresetCreated (UrlRef url, IPreset& preset) override;
	void CCL_API onPresetRemoved (UrlRef url, IPreset& preset) override;

	tbool CCL_API movePreset (IPreset& preset, StringRef newSubfolder) override;
	void CCL_API addSortFolder (const IAttributeList& metaInfo, StringRef path) override;
	void CCL_API removeSortFolder (const IAttributeList& metaInfo, StringRef path) override;
	void CCL_API moveSortFolder (const IAttributeList& metaInfo, StringRef oldPath, StringRef newPath) override;
	void CCL_API renameSortFolder (const IAttributeList& metaInfo, StringRef path, StringRef newName) override;
	tbool CCL_API hasSortFolder (const IAttributeList& metaInfo, StringRef path) const override;
	IUnknownIterator* CCL_API getSortFolders (const IAttributeList& metaInfo) const override;

	tbool CCL_API isFavorite (const IPreset& preset) const override;
	StringRef CCL_API getFavoriteFolder (const IPreset& preset) const override;
	void CCL_API setFavorite (const IPreset& preset, tbool state, StringRef folder = nullptr) override;
	IUnknownIterator* CCL_API getFavoritePresets (const IAttributeList& metaInfo) override;

	void CCL_API addFavoriteFolder (const IAttributeList& metaInfo, StringRef path) override;
	void CCL_API removeFavoriteFolder (const IAttributeList& metaInfo, StringRef path) override;
	void CCL_API moveFavoriteFolder (const IAttributeList& metaInfo, StringRef oldPath, StringRef newPath) override;
	void CCL_API renameFavoriteFolder (const IAttributeList& metaInfo, StringRef path, StringRef newName) override;
	IUnknownIterator* CCL_API getFavoriteFolders (const IAttributeList& metaInfo) const override;
	tbool CCL_API hasFavoriteFolder (const IAttributeList& metaInfo, StringRef path) const override;

	// Command Methods
	DECLARE_COMMANDS (PresetManager)
	DECLARE_COMMAND_CATEGORY ("Presets", Component)
	bool onScanPresets (CmdArgs);
	bool onScanPlugIns (CmdArgs);
	bool onResetBlocklist (CmdArgs);
	bool onRemovePlugInSettings (CmdArgs);

	// Component
	tbool CCL_API invokeMethod (Variant& returnValue, MessageRef msg) override;

	CLASS_INTERFACE (IPresetManager, Component)

private:
	mutable SortFolderListCollection sortFolders;
	mutable SortFolderListCollection presetFavorites;
	Threading::CriticalSection lock;
	PresetStore* presetStore;
	ObjectArray infoCache;
	static bool needFullRescan;
	static bool showProgressDialog;
	SignalSink fileSystemSink;
	SignalSource presetsSignal;
	bool folderSignalSuspended;

	static const String kSettingsName;

	static void getSettingsPath (IUrl& path);
	void loadSettings ();
	void saveSettings ();

	bool isInManagedFolder (UrlRef url);
	void onFolderAdded (UrlRef folder, IProgressNotify* progress = nullptr);
	IPreset* openPreset (IPresetFileHandler& handler, UrlRef url);

	SortFolderList* getSortFolderList (const IAttributeList& metaInfo) const;
	bool movePresetInternal (IPreset& preset, UrlRef newUrl, UrlRef oldUrl, const String* newSubFolder = nullptr);
	void movePresetsInternal (const IAttributeList& metaInfo, StringRef sourceFolder, StringRef targetFolder, bool isRemove = false);
	void signalSubFoldersChanged (const IAttributeList& metaInfo, StringID msgId, StringRef path);

	class FavoritesList;
	class FavoriteItem;
	IPreset* openFavoritePreset (const FavoriteItem& item, const IAttributeList& metaInfo);
	FavoritesList* getFavoritesList (const IAttributeList& metaInfo, bool create) const;
	FavoritesList* getFavoritesList (const IPreset& preset, bool create) const;
	FavoriteItem* getFavoriteItem (const IPreset& preset, bool create) const;
	void removeFavoriteItem (const IPreset& preset);
	void signalFavoritesChanged (const IAttributeList& metaInfo, StringRef folder = nullptr);

	// Component
	tresult CCL_API terminate () override;
	void CCL_API notify (ISubject* subject, MessageRef msg) override;
};

} // namespace CCL

#endif // _ccl_presetmanager_h
