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
// Filename    : ccl/app/presets/simplepreset.h
// Description : Simple Preset
//
//************************************************************************************************

#ifndef _ccl_simplepreset_h
#define _ccl_simplepreset_h

#include "ccl/app/presets/preset.h"

#include "ccl/base/collections/objectarray.h"
#include "ccl/base/storage/url.h"

#include "ccl/public/text/cstring.h"

namespace CCL {

//************************************************************************************************
// SimplePreset
//************************************************************************************************

class SimplePreset: public Preset
{
public:
	DECLARE_CLASS (SimplePreset, Preset)

	SimplePreset (UrlRef path = Url (), IAttributeList* metaInfo = nullptr);
	~SimplePreset ();

	PROPERTY_STRING (category, Category)
	PROPERTY_STRING (className, ClassName)

	// Preset
	tbool CCL_API isReadOnly () const override;
	IAttributeList* CCL_API getMetaInfo () const override;
	tbool CCL_API getUrl (IUrl& url) const override;
	tbool CCL_API store (IUnknown* target) override;
	tbool CCL_API restore (IUnknown* target) const override;
	tbool CCL_API toDescriptor (IPresetDescriptor& descriptor) const override;
	tbool CCL_API fromDescriptor (IPresetDescriptor& descriptor) override;
	tresult CCL_API queryInterface (UIDRef iid, void** ptr) override;

protected:
	Url path;
	mutable IAttributeList* metaInfo;
};

//************************************************************************************************
// SimpleXmlPreset
//************************************************************************************************

class SimpleXmlPreset: public SimplePreset
{
public:
	SimpleXmlPreset (UrlRef path = Url (), IAttributeList* metaInfo = nullptr);

	PROPERTY_MUTABLE_CSTRING (rootName, RootName)
	PROPERTY_MUTABLE_CSTRING (idAttributeName, IDAttributeName)

	// SimplePreset
	IAttributeList* CCL_API getMetaInfo () const override;
};

//************************************************************************************************
// SimplePresetHandler
//************************************************************************************************

class SimplePresetHandler: public PresetHandler
{
public:
	DECLARE_CLASS_ABSTRACT (SimplePresetHandler, PresetHandler)

	SimplePresetHandler (const FileType& fileType, int flags = 0);

	static SimplePresetHandler* findHandler (const IAttributeList& metaInfo); ///< find handler based on category
	static void getFactoryFolder (IUrl& path, StringRef subFolder = nullptr); ///< get factory folder for simple presets
	static StringRef getUserPresetFolderName ();

	void registerSelf ();	///< register with PresetFileRegistry

	PROPERTY_STRING (presetFolderName, PresetFolderName)	///< name for getWriteLocation()
	PROPERTY_STRING (presetCategory, PresetCategory)		///< assigned to SimplePreset::category
	PROPERTY_STRING (presetClassName, PresetClassName)		///< assigned to SimplePreset::className

	// PresetHandler
	int CCL_API getFlags () override;
	const FileType& CCL_API getFileType () override;
	tbool CCL_API canHandle (IUnknown* target) override;
	tbool CCL_API getWriteLocation (IUrl& url, IAttributeList* metaInfo) override;
	tbool CCL_API getReadLocation (IUrl& url, IAttributeList* metaInfo, int index) override;
	tbool CCL_API getSubFolder (String& subFolder, IAttributeList& metaInfo) override;
	IPreset* CCL_API openPreset (UrlRef url, IPresetDescriptor* descriptor = nullptr) override;
	IPreset* CCL_API createPreset (UrlRef url, IAttributeList& metaInfo) override;

protected:
	int flags;
	FileType fileType;

	static ObjectArray simplePresetHandlers;

	void finishPath (IUrl& url);
	virtual SimplePreset* newPreset (UrlRef url, IAttributeList* metaInfo = nullptr);
};

//************************************************************************************************
// TSimplePresetHandler
/** SimplePresetHandler instance that uses a preset class derived from SimplePreset. */
//************************************************************************************************

template<class PresetClass>
class TSimplePresetHandler: public SimplePresetHandler
{
public:
	TSimplePresetHandler (const FileType& fileType, int flags = 0)
	: SimplePresetHandler (fileType, flags)
	{}

protected:
	// SimplePresetHandler
	SimplePreset* newPreset (UrlRef url, IAttributeList* metaInfo = nullptr) override;
};

//////////////////////////////////////////////////////////////////////////////////////////////////

template<class PresetClass>
SimplePreset* TSimplePresetHandler<PresetClass>::newPreset (UrlRef url, IAttributeList* metaInfo)
{
	SimplePreset* preset = NEW PresetClass (url, metaInfo);
	preset->setCategory (presetCategory);
	preset->setClassName (presetClassName);
	return preset;
}

} // namespace CCL

#endif // _ccl_simplepreset_h
