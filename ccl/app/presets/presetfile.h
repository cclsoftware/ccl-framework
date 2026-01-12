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
// Filename    : ccl/app/presets/presetfile.h
// Description : Preset File
//
//************************************************************************************************

#ifndef _ccl_presetfile_h
#define _ccl_presetfile_h

#include "ccl/app/presets/preset.h"

#include "ccl/base/storage/url.h"
#include "ccl/base/singleton.h"
#include "ccl/base/collections/objectarray.h"
#include "ccl/base/collections/stringdictionary.h"

#include "ccl/public/base/iprogress.h"
#include "ccl/public/system/threadsync.h"

namespace CCL {

class LocalString;
class PackageInfo;
class ArchiveHandler;
class SignalSink;
class PresetMetaAttributes;

interface IPackageFile;

//************************************************************************************************
// PresetFile
//************************************************************************************************

class PresetFile: public Preset
{
public:
	DECLARE_CLASS (PresetFile, Preset)

	PresetFile (UrlRef url = Url (), PackageInfo* metaInfo = nullptr);
	~PresetFile ();

    // extra content
	interface IExtraContentHandler;
	static void registerExtraContentHandler (IExtraContentHandler* handler);

	PROPERTY_SHARED_AUTO (IProgressNotify, progress, Progress)

	// IPreset
	tbool CCL_API isReadOnly () const override;
	IAttributeList* CCL_API getMetaInfo () const override;
	tbool CCL_API getUrl (IUrl& url) const override;
	tbool CCL_API store (IUnknown* target) override;
	tbool CCL_API restore (IUnknown* target) const override;
	tbool CCL_API toDescriptor (IPresetDescriptor& descriptor) const override;
	tbool CCL_API fromDescriptor (IPresetDescriptor& descriptor) override;

protected:
	static Vector<IExtraContentHandler*> extraContentHandlers;
	static IPackageFile* createPackageForSave (UrlRef path);

	Url url;
	mutable PackageInfo* metaInfo;

	PackageInfo& getInfo () const;
	void setupMetaInfo ();

	virtual StringRef getMimeType () const;
	virtual IAttributeList* readMetaInfo () const;
	virtual bool storeContent (ArchiveHandler& handler, IUnknown* target);
	virtual bool restoreContent (ArchiveHandler& handler, IUnknown* target) const;

public:
	/** Determines if preset data should be compressed. */
	static int getPackageItemAttributesForType (const FileType& fileType);
};

//************************************************************************************************
// PresetFile::IExtraContentHandler
//************************************************************************************************

interface PresetFile::IExtraContentHandler
{
	virtual bool storeExtraContent (UrlRef url, ArchiveHandler& handler, IUnknown* target) = 0;	
	virtual bool restoreExtraContent (UrlRef url, ArchiveHandler& handler, IUnknown* target) const = 0;	
};

//************************************************************************************************
// PresetPackageHandler
//************************************************************************************************

class PresetPackageHandler: public PresetHandler,
							public Singleton<PresetPackageHandler>
{
public:
	PresetPackageHandler ();
	~PresetPackageHandler ();

	static const String kPresetFolder;
	static const LocalString& getPlural ();
	static void addCategorySubfolder (StringRef category, StringRef subFolderName);
	static bool getCategorySubfolder (StringRef category, String& subFolderName);

	void addRootFolder (UrlRef folder);
	void removeAllRootFolders ();

	/// the primary root folder is the default location for writing preset files
	UrlRef getPrimaryRootFolder ();
	void setPrimaryRootFolder (UrlRef folder);

	/// the factory root folder is a read-only location of installed preset files
	UrlRef getFactoryRootFolder (IUrl& folder);

	/// define an alternative / legacy name for a given vendor; presets found in a subfolder with such an alternative name will be presented as if they were in the subFolder for the actual vendor
	void setAlternativeVendorName (StringRef vendorName, StringRef alternativeName);

	bool getAlternativeSubFolder (String& subFolder, IAttributeList& metaInfo); // alternative in addition to IPresetHandler::getSubFolder 

	// PresetHandler
	int CCL_API getFlags () override;
	tbool CCL_API canHandle (IUnknown* target) override;
	tbool CCL_API getWriteLocation (IUrl& url, IAttributeList* metaInfo) override;
	tbool CCL_API getReadLocation (IUrl& url, IAttributeList* metaInfo, int index) override;
	tbool CCL_API getSubFolder (String& subFolder, IAttributeList& metaInfo) override;
	const FileType& CCL_API getFileType () override;
	IPreset* CCL_API openPreset (UrlRef url, IPresetDescriptor* descriptor = nullptr) override;
	IPreset* CCL_API createPreset (UrlRef url, IAttributeList& metaInfo) override;
	void CCL_API notify (ISubject* subject, MessageRef msg) override;

private:
	AutoPtr<Url> primaryFolder;
	ObjectArray rootFolders;
	StringDictionary alternativeVendors;
	SignalSink& systemSink;
	Threading::CriticalSection lock;
	static const CCL::UID catListId;

	static void removeTrailingDot (String& name);
	void finishPath (IUrl& url, IAttributeList* metaInfo);
	void adjustMetaInfo (PresetMetaAttributes& metaAttributes, String* alternativeClassName = nullptr) const;
	bool getSubFolderInternal (String& subFolder, const PresetMetaAttributes& metaAttributes, const String* vendorName = nullptr, const String* className = nullptr) const;
};

//************************************************************************************************
// PresetLocationHandler
//************************************************************************************************

class PresetLocationHandler: public PresetHandler,
							 public Singleton<PresetLocationHandler>
{
public:
	PresetLocationHandler ();

	void addLocation (UrlRef path); 	///< add arbitrary file system location to preset system
	void removeLocation (UrlRef path);	///< remove arbitary file system location from preset system
	void locationsChanged ();

	// PresetHandler
	tbool CCL_API getReadLocation (IUrl& url, IAttributeList* metaInfo, int index) override;

protected:
	ObjectArray presetLocations;
};

} // namespace CCL

#endif // _ccl_presetfile_h
