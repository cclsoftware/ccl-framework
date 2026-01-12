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
// Filename    : ccl/app/presets/presetcollection.h
// Description : Preset Collection
//
//************************************************************************************************

#ifndef _ccl_presetcollection_h
#define _ccl_presetcollection_h

#include "ccl/app/presets/presetfile.h"
#include "ccl/base/storage/attributes.h"

namespace CCL {

//************************************************************************************************
// PresetPart
//************************************************************************************************

class PresetPart: public PersistentAttributes
{
public:
	DECLARE_CLASS (PresetPart, PersistentAttributes)
};

//************************************************************************************************
// PresetPartList
//************************************************************************************************

class PresetPartList: public Object
{
public:
	DECLARE_CLASS (PresetPartList, Object)

	PresetPartList ();

	int countParts () const;
	PresetPart* getPart (int index) const;
	void addPart (PresetPart* part);
	int getPartIndex (const PresetPart* part) const;
	Iterator* newIterator () const;

	bool loadFromHandler (ArchiveHandler& handler);
	bool saveWithHandler (ArchiveHandler& handler);

	// Object
	bool load (const Storage& storage) override;
	bool save (const Storage& storage) const override;

protected:
	ObjectArray parts;
};

//************************************************************************************************
// PresetCollection
//************************************************************************************************

class PresetCollection: public PresetFile,
						public IPresetCollection
{
public:
	DECLARE_CLASS (PresetCollection, PresetFile)

	PresetCollection (UrlRef url = Url (), PackageInfo* metaInfo = nullptr);
	~PresetCollection ();

	PresetPartList& getParts () const;

	// IPresetCollection
	int CCL_API countPresets () override;
	IPreset* CCL_API openPreset (int index) override;
	IPreset* CCL_API openPreset (const IStringDictionary& parameters) override;
	IPreset* CCL_API createPreset (IAttributeList& metaInfo) override;
	IStream* CCL_API openStream (StringRef path, int mode) override;

	tbool CCL_API toDescriptor (IPresetDescriptor& descriptor) const override;
	tbool CCL_API fromDescriptor (IPresetDescriptor& descriptor) override;

	CLASS_INTERFACE (IPresetCollection, PresetFile)

protected:
	friend class PresetSubFile;
	mutable PresetPartList* parts;
	mutable ArchiveHandler* currentHandler;

	// PresetFile
	StringRef getMimeType () const override;
	IAttributeList* readMetaInfo () const override;
	bool storeContent (ArchiveHandler& handler, IUnknown* target) override;
	bool restoreContent (ArchiveHandler& handler, IUnknown* target) const override;
};

//************************************************************************************************
// PresetArchiver
//************************************************************************************************

class PresetArchiver: public PresetCollection
{
public:
	DECLARE_CLASS_ABSTRACT (PresetArchiver, PresetCollection)

	PresetArchiver (ArchiveHandler& archiveHandler, IAttributeList* additionalAttributes = nullptr);

	// PresetCollection
	tbool CCL_API store (IUnknown* target) override;
	tbool CCL_API restore (IUnknown* target) const override;

protected:
	ArchiveHandler& archiveHandler;
	IAttributeList* additionalAttributes;

	// PresetCollection
	IAttributeList* readMetaInfo () const override;
};

//************************************************************************************************
// PresetCollectionHandler
//************************************************************************************************

class PresetCollectionHandler: public PresetHandler,
							   public Singleton<PresetCollectionHandler>
{
public:
	// PresetHandler
	int CCL_API getFlags () override;
	tbool CCL_API canHandle (IUnknown* target) override;
	tbool CCL_API getWriteLocation (IUrl& url, IAttributeList* metaInfo) override;
	tbool CCL_API getReadLocation (IUrl& url, IAttributeList* metaInfo, int index) override;
	tbool CCL_API getSubFolder (String& subFolder, IAttributeList& metaInfo) override;
	const FileType& CCL_API getFileType () override;
	IPreset* CCL_API openPreset (UrlRef url, IPresetDescriptor* descriptor = nullptr) override;
	IPreset* CCL_API createPreset (UrlRef url, IAttributeList& metaInfo) override;
};

} // namespace CCL

#endif // _ccl_presetcollection_h
