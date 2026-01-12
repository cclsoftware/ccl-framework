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
// Filename    : ccl/app/presets/memorypreset.h
// Description : Memory Preset
//
//************************************************************************************************

#ifndef _ccl_memorypreset_h
#define _ccl_memorypreset_h

#include "ccl/app/presets/preset.h"

#include "ccl/base/singleton.h"
#include "ccl/base/storage/packageinfo.h"

namespace CCL {

class Settings;
class PresetCategory;
interface IFileSystem;

//************************************************************************************************
// MemoryPreset
/** Stores preset data of an CCL::Object as attributes using Object::save/load. */
//************************************************************************************************

class MemoryPreset: public Preset
{
public:
	DECLARE_CLASS (MemoryPreset, Preset)

	MemoryPreset (IAttributeList* metaInfo = nullptr);

	// Preset
	IAttributeList* CCL_API getMetaInfo () const override;
	tbool CCL_API getUrl (IUrl& url) const override;
	tbool CCL_API store (IUnknown* target) override;
	tbool CCL_API restore (IUnknown* target) const override;
	bool load (const Storage& storage) override;
	bool save (const Storage& storage) const override;

private:
	AutoPtr<PackageInfo> metaInfo;
	AutoPtr<Attributes> data;
};

//************************************************************************************************
// MemoryPresetHandler
//************************************************************************************************

class MemoryPresetHandler: public PresetHandler,
						   public UnmanagedSingleton<MemoryPresetHandler>
{
public:
	MemoryPresetHandler ();
	~MemoryPresetHandler ();

	PresetCategory* getCategory (StringRef categoryName, bool create);

	static void makeCategoryUrl (IUrl& url, StringRef category);
	static void makeCategoryUrl (IUrl& url, IAttributeList* metaInfo);

	// PresetHandler
	int CCL_API getFlags () override;
	tbool CCL_API canHandle (IUnknown* target) override;
	tbool CCL_API getWriteLocation (IUrl& url, IAttributeList* metaInfo) override;
	tbool CCL_API getReadLocation (IUrl& url, IAttributeList* metaInfo, int index) override;
	const FileType& CCL_API getFileType () override;
	IPreset* CCL_API openPreset (UrlRef url, IPresetDescriptor* descriptor = nullptr) override;
	IPreset* CCL_API createPreset (UrlRef url, IAttributeList& metaInfo) override;

private:
	Settings* settings;

	Settings& getSettings ();
};

} // namespace CCL

#endif // _ccl_memorypreset_h
