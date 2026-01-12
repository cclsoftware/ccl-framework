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
// Filename    : ccl/app/presets/presetstore.h
// Description : Preset Store
//
//************************************************************************************************

#ifndef _ccl_presetstore_h
#define _ccl_presetstore_h

#include "ccl/app/presets/presetdescriptor.h"

#include "ccl/base/storage/persistence/datastore.h"
#include "ccl/base/collections/stringlist.h"

namespace CCL {

namespace Persistence {
class Expression; }

interface ISearcher;
interface ISearchDescription;
interface IProgressNotify;
class PresetFileRegistry;

//************************************************************************************************
// PresetStore
//************************************************************************************************

class PresetStore
{
public:
	PresetStore ();

	bool isEmpty () const;
	bool hasPresets (IAttributeList& metaInfo) const;

	IUnknownList* getPresets (IAttributeList* metaInfo, IProgressNotify* progress = nullptr) const;
	bool presetExists (IAttributeList* metaInfo, StringRef name, const FileType* fileType = nullptr) const;

	ISearcher* createSearcher (ISearchDescription& description);

	void onPresetCreated (UrlRef url, const IPreset& preset);	///< adds or updates if necessary
	void onPresetRemoved (UrlRef url, const IPreset& preset);	///< removes preset from db

	void addPreset (UrlRef url, const IPreset& preset);
	void addPreset (PresetDescriptor* newPreset);
	void updatePreset (PresetDescriptor* existingPreset);
	void removePreset (PresetDescriptor* existingPreset);
	void flush (bool force);

	/// query for PresetDescriptors
	PresetDescriptor* getPresetDescriptor (UrlRef url) const; ///< caller owns descriptor
	Iterator* query (Persistence::IExpression* condition = nullptr) const; ///<  returns iterator of PresetDescriptors
	Iterator* queryFolderDeep (UrlRef folder) const; ///<  returns iterator of PresetDescriptors

	void getPresetLocations (Container& locations); ///< adds PresetLocation (DataItem) objects
	void collectClassKeys ();

	void collectSubFolders (IMutableArray& subFolders, const IAttributeList* metaInfo);

	DataStore& getDataStore () const;

	static String getClassKey (const IAttributeList& metaInfo);
	static Persistence::Expression makeClassCondition (const IAttributeList& metaInfo);

private:
	mutable DataStore dataStore;
	mutable StringList cachedClassKeys;

	class PresetSearcher;
	class PresetFilter;
};

//************************************************************************************************
// PresetStoreSynchronizer
//************************************************************************************************

class PresetStoreSynchronizer
{
public:
	PresetStoreSynchronizer (PresetStore& store);

	void scanLocations (IProgressNotify* progress, bool onlyChangedLocations);

private:
	PresetStore& store;
	IPresetFileRegistry& registry;
	FileTypeFilter fileTypes;
	IProgressNotify* progress;
	bool forceFileUpdate;

	class PresetFolder;
	class UrlItem;

	void scanNewFolder (UrlRef folder, StringRef subFolder);
	void synchronizeFolder (UrlRef folder, StringRef subFolder, PresetFolder* presetFolder);
	void removeFolder (PresetFolder& folder);
	void removeFolder (UrlRef folder);

	IPreset* openPresetFile (const Url& url, StringRef subFolder);
	void checkFoundPresetFile (UrlRef path, StringRef subFolder, PresetDescriptor* existingDescriptor);
	String descendSubFolderString (StringRef subFolder, Url subFolderUrl);
};

//////////////////////////////////////////////////////////////////////////////////////////////////
// inline
//////////////////////////////////////////////////////////////////////////////////////////////////

inline void PresetStore::addPreset (PresetDescriptor* newPreset)
{ dataStore.addItem (newPreset); }

inline void PresetStore::updatePreset (PresetDescriptor* existingPreset)
{ dataStore.updateItem (existingPreset); }

inline void PresetStore::removePreset (PresetDescriptor* existingPreset)
{ dataStore.removeItem (existingPreset); }

inline void PresetStore::flush (bool force)
{ dataStore.flush (force); }

inline Iterator* PresetStore::query (Persistence::IExpression* condition) const
{ return dataStore.query<PresetDescriptor> (condition); }

inline DataStore& PresetStore::getDataStore () const
{ return dataStore; }

//////////////////////////////////////////////////////////////////////////////////////////////////

} // namespace CCL

#endif // _ccl_presetstore_h
