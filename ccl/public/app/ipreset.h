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
// Filename    : ccl/public/app/ipreset.h
// Description : Preset Interfaces
//
//************************************************************************************************

#ifndef _ccl_ipreset_h
#define _ccl_ipreset_h

#include "ccl/public/base/iunknown.h"
#include "ccl/public/base/debug.h"
#include "ccl/public/base/cclmacros.h"
#include "ccl/public/text/cclstring.h"

#include "ccl/public/storage/filetype.h"

namespace CCL {

interface IAttributeList;
interface IStringDictionary;
interface IPresetDescriptor;
interface IFileDescriptor;
interface IFileTypeFilter;
interface ISearcher;
interface ISearchDescription;
interface IUnknownList;
interface IProgressNotify;
interface IObserver;
interface IMutableArray;
interface IUnknownIterator;

//////////////////////////////////////////////////////////////////////////////////////////////////

/** Class category for preset file handlers. */
#define PLUG_CATEGORY_PRESETFILEHANDLER CCLSTR ("PresetFileHandler")

//************************************************************************************************
// IPreset
/**	\ingroup app_preset */
//************************************************************************************************

interface IPreset: IUnknown
{
	/** Check if preset is read-only. */
	virtual tbool CCL_API isReadOnly () const = 0;

	/** Check if preset has been modified. */
	virtual tbool CCL_API isModified () const = 0;

	/** Get preset name. */
	virtual StringRef CCL_API getPresetName () const = 0;
	
	/** Get meta information. */
	virtual IAttributeList* CCL_API getMetaInfo () const = 0;
	
	/** Get preset location. */
	virtual tbool CCL_API getUrl (IUrl& url) const = 0;

	/** Get arbitrary data associated with this preset. */
	virtual IUnknown* CCL_API getUserData () const = 0;
	
	/** Transfer data from target to preset. */
	virtual tbool CCL_API store (IUnknown* target) = 0;

	/** Transfer data from preset to target. */
	virtual tbool CCL_API restore (IUnknown* target) const = 0;

	/** Assign meta info and optional data to preset descriptor. */
	virtual tbool CCL_API toDescriptor (IPresetDescriptor& descriptor) const = 0;

	/** Take meta info and optional data from preset descriptor. */
	virtual tbool CCL_API fromDescriptor (IPresetDescriptor& descriptor) = 0;

	/** Assign arbitrary data associated with this preset. (shared) */
	virtual tbool CCL_API setUserData (IUnknown* data) = 0;

	DECLARE_IID (IPreset)
};

DEFINE_IID (IPreset, 0x340b7644, 0x8eee, 0x4f23, 0xb9, 0xd3, 0xfe, 0xf0, 0x8f, 0x20, 0xda, 0xdf)

//************************************************************************************************
// IPresetDescriptor
/**	\ingroup app_preset */
//************************************************************************************************

interface IPresetDescriptor: IUnknown
{
	virtual StringRef CCL_API getPresetName () = 0;

	virtual IStream* CCL_API getData () = 0;

	DECLARE_IID (IPresetDescriptor)
};

DEFINE_IID (IPresetDescriptor, 0x8EF87E9E, 0x80A4, 0x4CB3, 0xAA, 0xBE, 0x36, 0x31, 0x9B, 0x0E, 0x23, 0xD4)

//************************************************************************************************
// IPresetCollection
/**	\ingroup app_preset */
//************************************************************************************************

interface IPresetCollection: IUnknown
{
	/** Get number of presets. */
	virtual int CCL_API countPresets () = 0;

	/** Open preset at given index (must be released by caller). */
	virtual IPreset* CCL_API openPreset (int index) = 0;

	/** Open preset with parameters (must be released by caller). */
	virtual IPreset* CCL_API openPreset (const IStringDictionary& parameters) = 0;

	/** Create preset (must be released by caller). */
	virtual IPreset* CCL_API createPreset (IAttributeList& metaInfo) = 0;

	/** Open additional stream (must be released by caller). */
	virtual	IStream* CCL_API openStream (StringRef path, int mode) = 0;
	
	DECLARE_IID (IPresetCollection)
};

DEFINE_IID (IPresetCollection, 0xf860434e, 0xe8de, 0x4307, 0x8d, 0xbd, 0xc9, 0x1a, 0x3c, 0x5f, 0x5b, 0xc6)

//************************************************************************************************
// IPresetCollector
/**	\ingroup app_preset */
//************************************************************************************************

interface IPresetCollector: IUnknown
{
	/** Load preset collection. */
	virtual tbool CCL_API load (IPresetCollection& collection) = 0;

	/** Save preset collection. */
	virtual tbool CCL_API save (IPresetCollection& collection) const = 0;

	DECLARE_IID (IPresetCollector)
};

DEFINE_IID (IPresetCollector, 0x30802b95, 0x7bce, 0x481d, 0xa1, 0x38, 0x4b, 0xd0, 0x65, 0x1c, 0x71, 0xbb)

//************************************************************************************************
// IPresetFileHandler
/**	\ingroup app_preset */
//************************************************************************************************

interface IPresetFileHandler: IUnknown
{
	/** Capability flags. */ 
	DEFINE_ENUM (Flags)
	{
		kCanImport			= 1<<0,	///< format can be imported
		kCanExport			= 1<<1,	///< format can be exported
		kRescanRegularly	= 1<<2,	///< preset locations need to be rescanned regularly (e.g. on program start)
		kIsVIPFormat		= 1<<3,	///< very important preset (VIP) format
		kStoresDescriptorData = 1<<4 ///< presets of this handler use the preset descriptor "data" to cache information (see IPreset::toDescriptor / fromDescriptor)
	};

	/** Get handler capability flags. */
	virtual int CCL_API getFlags () = 0;

	/** Check if target can be handled. */
	virtual tbool CCL_API canHandle (IUnknown* target) = 0;

	/** Get location for writing presets, metaInfo can be null (root location). */
	virtual tbool CCL_API getWriteLocation (IUrl& url, IAttributeList* metaInfo = nullptr) = 0;
	
	/** Get location(s) for reading presets, metaInfo can be null (root location). */
	virtual tbool CCL_API getReadLocation (IUrl& url, IAttributeList* metaInfo, int index) = 0;

	/** Get subFolder (relative to a root location) for given metaInfo. */
	virtual tbool CCL_API getSubFolder (String& subFolder, IAttributeList& metaInfo) = 0;

	/** Get preset file type (legacy method, handlers with multiple types should return the most important / default type here). */
	virtual const FileType& CCL_API getFileType () = 0;

	/** Open preset from given location; use optional descriptor to restore cached meta information. */
	virtual IPreset* CCL_API openPreset (UrlRef url, IPresetDescriptor* descriptor = nullptr) = 0;

	/** Create preset at given location. */
	virtual IPreset* CCL_API createPreset (UrlRef url, IAttributeList& metaInfo) = 0;

	/** Get handler capability flags for target - canHandle and getFlags combined. */
	virtual int CCL_API getTargetFlags (IUnknown* target) = 0;

	/** Get supported preset file types; returns empty FileType when done. */
	virtual const FileType& CCL_API getFileType (int index) = 0;

	DECLARE_STRINGID_MEMBER (kPresetLocationsChanged)

	DECLARE_IID (IPresetFileHandler)
};

DEFINE_IID (IPresetFileHandler, 0x4351B0E8, 0xA282, 0x45AE, 0xBF, 0x9C, 0x1A, 0x9D, 0x44, 0xA4, 0x5B, 0x58)

DEFINE_STRINGID_MEMBER (IPresetFileHandler, kPresetLocationsChanged, "LocationsChanged")

//************************************************************************************************
// IPresetMetaDataProvider
/**	\ingroup app_preset */
//************************************************************************************************

interface IPresetMetaDataProvider: IUnknown
{
	/** Get preset meta information. */
	virtual tbool CCL_API getPresetMetaInfo (IAttributeList& metaInfo) = 0;

	DECLARE_IID (IPresetMetaDataProvider)
};

DEFINE_IID (IPresetMetaDataProvider, 0x2e962128, 0x1615, 0x4407, 0x9e, 0xf2, 0x14, 0x84, 0xa2, 0x37, 0x37, 0x1c)

//************************************************************************************************
// IPresetMediator
/**	\ingroup app_preset */
//************************************************************************************************

interface IPresetMediator: IPresetMetaDataProvider
{
	/** Get preset target. */
	virtual IUnknown* CCL_API getPresetTarget () = 0;

	/** Get type of associated preset handler (optional). */
	virtual StringRef CCL_API getDefaultPresetType () = 0;

	/** Get name for storing or exporting a preset. */
	virtual String CCL_API makePresetName (tbool forExport) = 0;

	/** Store preset. */
	virtual tbool CCL_API storePreset (IPreset& preset) = 0;

	/** Restore preset. */
	virtual tbool CCL_API restorePreset (const IPreset& preset) = 0;

	DECLARE_IID (IPresetMediator)
};

DEFINE_IID (IPresetMediator, 0x25E340AF, 0xD6E6, 0x49CF, 0xB2, 0x58, 0x87, 0x47, 0x6E, 0x4A, 0x27, 0x3A)


//************************************************************************************************
// IPresetManager
/**	\ingroup app_preset */
//************************************************************************************************

interface IPresetManager: IUnknown
{
	/** Open preset from given location. */
	virtual IPreset* CCL_API openPreset (UrlRef url) = 0;

	/** Open preset from given descriptor. */
	virtual IPreset* CCL_API openPreset (const IFileDescriptor& descriptor) = 0;

	/** Open default preset. */
	virtual IPreset* CCL_API openDefaultPreset (IPresetFileHandler& handler, IAttributeList* metaInfo) = 0;

	/** Check if file type is supported. */
	virtual tbool CCL_API supportsFileType (const FileType& fileType) = 0;

	/** Get all presets that match the passed meta info. */
	virtual IUnknownList* CCL_API getPresets (IAttributeList* metaInfo, IProgressNotify* progress = nullptr) = 0;
	
	/** Get all presets in background that match the passed meta info. */
	virtual void CCL_API getPresetsInBackground (IObserver* observer, IAttributeList* metaInfo) = 0;
	
	/** Stop getting presets in background. */
	virtual void CCL_API cancelGetPresets (IObserver* observer) = 0;

	/** Perform scan for presets. */
	virtual	void CCL_API scanPresets (tbool onlyChangedLocations) = 0;

	/** Check if there is a preset with this name and meta info (and optional file type) already exists. */
	virtual tbool CCL_API presetExists (IAttributeList* metaInfo, StringRef name, const FileType* fileType = nullptr) = 0;

	/** Check if any preset exists with given meta info. */
	virtual tbool CCL_API hasPresets (IAttributeList* metaInfo) = 0;

	/** Delete preset. */
	virtual tbool CCL_API removePreset (IPreset& preset) = 0;

	/** Rename preset. */
	virtual tbool CCL_API renamePreset (IPreset& preset, StringRef newName, IUrl* newUrl = nullptr) = 0;

	/** Move preset to another folder. */
	virtual tbool CCL_API movePreset (IPreset& preset, StringRef newSubfolder) = 0;

	/** Manage sort folders per meta info. */
	virtual void CCL_API addSortFolder (const IAttributeList& metaInfo, StringRef path) = 0;
	virtual void CCL_API removeSortFolder (const IAttributeList& metaInfo, StringRef path) = 0;
	virtual void CCL_API moveSortFolder (const IAttributeList& metaInfo, StringRef oldPath, StringRef newPath) = 0;
	virtual void CCL_API renameSortFolder (const IAttributeList& metaInfo, StringRef path, StringRef newName) = 0;
	virtual tbool CCL_API hasSortFolder (const IAttributeList& metaInfo, StringRef path) const = 0;
	virtual IUnknownIterator* CCL_API getSortFolders (const IAttributeList& metaInfo) const = 0;

	/** Favorite presets per meta info. */
	virtual tbool CCL_API isFavorite (const IPreset& preset) const = 0;
	virtual StringRef CCL_API getFavoriteFolder (const IPreset& preset) const = 0;
	virtual void CCL_API setFavorite (const IPreset& preset, tbool state, StringRef folder = nullptr) = 0;
	virtual IUnknownIterator* CCL_API getFavoritePresets (const IAttributeList& metaInfo) = 0;

	virtual void CCL_API addFavoriteFolder (const IAttributeList& metaInfo, StringRef path) = 0;
	virtual void CCL_API removeFavoriteFolder (const IAttributeList& metaInfo, StringRef path) = 0;
	virtual void CCL_API moveFavoriteFolder (const IAttributeList& metaInfo, StringRef oldPath, StringRef newPath) = 0;
	virtual void CCL_API renameFavoriteFolder (const IAttributeList& metaInfo, StringRef path, StringRef newName) = 0;
	virtual IUnknownIterator* CCL_API getFavoriteFolders (const IAttributeList& metaInfo) const = 0;
	virtual tbool CCL_API hasFavoriteFolder (const IAttributeList& metaInfo, StringRef path) const = 0; ///< empty path: has any favorites

	/** Create preset searcher. */
	virtual ISearcher* CCL_API createSearcher (ISearchDescription& description) = 0;

	/** Inform preset manager that a new file was created. */
	virtual void CCL_API onPresetCreated (UrlRef url, IPreset& preset) = 0;

	/** Announce removal of the old preset (can be another file, when a factory preset gets replaced (hidden) by a user preset) */
	virtual void CCL_API onPresetRemoved (UrlRef url, IPreset& preset) = 0;

	/** Collect all distinct ocurring subFolder strings for the given meta info. */
	virtual void CCL_API collectSubFolders (IMutableArray& subFolders, const IAttributeList* metaInfo) = 0;

	DECLARE_IID (IPresetManager)
};

DEFINE_IID (IPresetManager, 0x861a7175, 0xac7b, 0x457c, 0xac, 0xcd, 0x8b, 0x35, 0xdf, 0xcc, 0xea, 0x9d)

//************************************************************************************************
// IPresetFileRegistry 
/**	\ingroup app_preset */
//************************************************************************************************

interface IPresetFileRegistry: IUnknown
{	
	/** Register handler - registry takes ownership */
	virtual void CCL_API addHandler (IPresetFileHandler* handler, tbool isDefault = false) = 0; 

	/** Return number of registered handlers. */
	virtual int CCL_API countHandlers () const = 0;
	
	/** Get handler by index. */
	virtual IPresetFileHandler* CCL_API getHandler (int index) const = 0;	

	/** Get handler by target. */
	virtual IPresetFileHandler* CCL_API getHandlerForTarget (IUnknown* target) const = 0;

	/** Get handler by file. */
	virtual IPresetFileHandler* CCL_API getHandlerForFile (UrlRef url) const = 0;

	/** Get handler by file type. */
	virtual IPresetFileHandler* CCL_API getHandlerForFileType (const FileType& fileType)  const= 0;

	/** Get handler by mime type. */
	virtual IPresetFileHandler* CCL_API getHandlerForMimeType (StringID mimeType) const = 0;

	/** Get default handler. */
	virtual IPresetFileHandler* CCL_API getDefaultHandler () const = 0;

	/** Collect file types of handlers with given flags that can handle the target */
	virtual void CCL_API collectFileTypes (IFileTypeFilter& fileTypes, IUnknown* target = nullptr, int requiredHandlerFlags = 0) const = 0; 

	/** Define a "virtual" display subFolder that is prepended to the subFolder attribute of all presets in a location. */
	virtual void CCL_API setSubFolderPrefix (UrlRef location, StringRef subFolder) = 0;

	/** Get defined subFolder for a location of a preset file or folder. */
	virtual StringRef CCL_API getSubFolderPrefix (UrlRef url) const = 0;

	DECLARE_IID (IPresetFileRegistry)
};

DEFINE_IID (IPresetFileRegistry, 0x6c19f466, 0xc0c3, 0x4c91, 0x8b, 0x1e, 0x5e, 0x0, 0x29, 0xc6, 0x74, 0xa8)

//************************************************************************************************
// IPresetNotificationSink
/** Can be implemented by a preset target to receive notifications. 
	\ingroup app_preset */
//************************************************************************************************

interface IPresetNotificationSink: IUnknown
{
	/** Notification before and after a preset is being restored. */
	virtual void CCL_API onPresetChanging (const IPreset& preset, tbool begin) = 0;

	/** Notification after a preset has been restored. */
	virtual void CCL_API onPresetRestored (const IPreset& preset) = 0;

	/** Additional hint for preset notification. */
	DEFINE_ENUM (PresetNotificationHint)
	{
		kPresetHintUndefined = -1,
		kStorePreset,
		kStoreDefaultPreset,
		kExportPreset
	};

	/** Notification before preset is being stored. */
	virtual void CCL_API onPresetStoring (const IPreset& preset, PresetNotificationHint hint) = 0;

	/** Notification after a preset has been stored. */
	virtual void CCL_API onPresetStored (const IPreset& preset, PresetNotificationHint hint) = 0;

	/** Notification whenever the current preset name changed. */
	virtual void CCL_API onCurrentPresetNameChanged (StringRef name) = 0;
	
	DECLARE_IID (IPresetNotificationSink)
};

DEFINE_IID (IPresetNotificationSink, 0xAFDAC924, 0x547B, 0x46A0, 0x8C, 0xCA, 0xDC, 0x6E, 0xA1, 0x85, 0x68, 0x23)

//************************************************************************************************
// AbstractPreset
/**	\ingroup app_preset */
//************************************************************************************************

class AbstractPreset: public IPreset
{
public:
	tbool CCL_API isReadOnly () const override
	{
		return true;
	}

	tbool CCL_API isModified () const override
	{
		return false;
	}

	StringRef CCL_API getPresetName () const override
	{
		return String::kEmpty;
	}

	IAttributeList* CCL_API getMetaInfo () const override
	{
		return nullptr;
	}

	tbool CCL_API getUrl (IUrl& url) const override
	{
		return false;
	}

	IUnknown* CCL_API getUserData () const override
	{
		return nullptr;
	}

	tbool CCL_API store (IUnknown* target) override
	{
		CCL_NOT_IMPL ("IPreset::store() not implemented!")
		return false;
	}

	tbool CCL_API restore (IUnknown* target) const override
	{
		CCL_NOT_IMPL ("IPreset::restore() not implemented!")
		return false;
	}

	tbool CCL_API toDescriptor (IPresetDescriptor& descriptor) const override
	{
		return false;
	}

	tbool CCL_API fromDescriptor (IPresetDescriptor& descriptor) override
	{
		return false;
	}

	tbool CCL_API setUserData (IUnknown* data) override
	{
		return false;
	}
};

//************************************************************************************************
// AbstractPresetFileHandler
/**	\ingroup app_preset */
//************************************************************************************************

class AbstractPresetFileHandler: public IPresetFileHandler
{
public:
	int CCL_API getFlags () override
	{
		return 0;
	}

	tbool CCL_API canHandle (IUnknown* target) override
	{
		return false;
	}
	
	tbool CCL_API getWriteLocation (IUrl& url, IAttributeList* metaInfo) override
	{
		return false;
	}

	tbool CCL_API getReadLocation (IUrl& url, IAttributeList* metaInfo, int index) override
	{
		return false;
	}

	tbool CCL_API getSubFolder (String& subFolder, IAttributeList& metaInfo) override
	{
		return false;
	}
	
	const FileType& CCL_API getFileType () override
	{
		return FileTypes::Empty ();
	}

	const FileType& CCL_API getFileType (int index) override
	{
		if(index == 0)
			return getFileType (); // fallback to single type implementation
		else
			return FileTypes::Empty ();
	}

	IPreset* CCL_API openPreset (UrlRef url, IPresetDescriptor* descriptor) override
	{
		return nullptr;
	}
	
	IPreset* CCL_API createPreset (UrlRef url, IAttributeList& metaInfo) override
	{
		return nullptr;
	}

	int CCL_API getTargetFlags (IUnknown* target) override
	{
		if(canHandle (target))
			return getFlags ();
		return 0;
	}
};

//************************************************************************************************
// AbstractPresetMediator
/**	\ingroup app_inter */
//************************************************************************************************

class AbstractPresetMediator: public IPresetMediator
{
public:
	IUnknown* CCL_API getPresetTarget () override
	{
		return this;
	}

	StringRef CCL_API getDefaultPresetType () override
	{
		return String::kEmpty;
	}

	tbool CCL_API getPresetMetaInfo (IAttributeList& metaInfo) override
	{
		return false;
	}

	String CCL_API makePresetName (tbool forExport) override
	{
		return String::kEmpty;
	}

	tbool CCL_API storePreset (IPreset& preset) override
	{
		return preset.store (getPresetTarget ());
	}

	tbool CCL_API restorePreset (const IPreset& preset) override
	{
		return preset.restore (getPresetTarget ());
	}
};

//************************************************************************************************
// AbstractPresetNotificationSink
//************************************************************************************************

class AbstractPresetNotificationSink: public IPresetNotificationSink
{
public:
	void CCL_API onPresetChanging (const IPreset& preset, tbool begin) override {}
	void CCL_API onPresetRestored (const IPreset& preset) override {}
	void CCL_API onPresetStoring (const IPreset& preset, PresetNotificationHint hint) override {}
	void CCL_API onPresetStored (const IPreset& preset, PresetNotificationHint hint) override {}
	void CCL_API onCurrentPresetNameChanged (StringRef name) override {}
};

} // namespace CCL

#endif // _ccl_ipreset_h
