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
// Filename    : ccl/extras/extensions/installdata.h
// Description : Installer Data
//
//************************************************************************************************

#ifndef _ccl_installdata_h
#define _ccl_installdata_h

#include "ccl/base/storage/url.h"
#include "ccl/base/collections/objectarray.h"
#include "ccl/base/storage/storableobject.h"

#include "ccl/public/text/cstring.h"
#include "ccl/public/base/datetime.h"
#include "ccl/public/system/ifileitem.h"
#include "ccl/public/plugins/versionnumber.h"

namespace CCL {
namespace Install {

class RequiredApp;

//************************************************************************************************
// Install::Item
//************************************************************************************************

class Item: public Object
{
public:
	DECLARE_CLASS (Item, Object)

	Item (StringRef id = nullptr, StringRef title = nullptr);

	PROPERTY_STRING (id, ID)
	PROPERTY_STRING (title, Title)
	PROPERTY_STRING (iconName, IconName)

	// Object
	bool equals (const Object& obj) const override;
	bool load (const Storage& storage) override;
	bool save (const Storage& storage) const override;
};

//************************************************************************************************
// Install::ContainerItem
//************************************************************************************************

class ContainerItem: public Item
{
public:
	DECLARE_CLASS (ContainerItem, Item)

	ContainerItem ();

	const Container& getChildren () const;
	Item* findChild (StringRef id) const;

	// Item
	bool load (const Storage& storage) override;
	bool save (const Storage& storage) const override;

protected:
	ObjectArray children;
};

//************************************************************************************************
// Install::Medium
//************************************************************************************************

class Medium: public Item
{
public:
	DECLARE_CLASS (Medium, Item)

	Medium ();

	PROPERTY_BOOL (autoDetect, AutoDetect)

	// Item
	bool load (const Storage& storage) override;
	bool save (const Storage& storage) const override;
};

//************************************************************************************************
// Install::Package
//************************************************************************************************

class Package: public ContainerItem
{
public:
	PROPERTY_STRING (description, Description)
	PROPERTY_STRING (type, Type)
	PROPERTY_STRING (savedChildID, SavedChildID) // can be used at runtime, not persistent

	DECLARE_CLASS (Package, ContainerItem)
};

//************************************************************************************************
// Install::File
//************************************************************************************************

class File: public Item,
			public IFileDescriptor
{
public:
	DECLARE_CLASS (File, Item)

	File (StringRef id = nullptr);

	PROPERTY_STRING (parentId, ParentID)
	PROPERTY_STRING (savedParentID, SavedParentID) // can be used at runtime, not persistent
	PROPERTY_STRING (fileName, FileName)
	bool isFolder () const;
	PROPERTY_STRING (mediumId, MediumID)
	PROPERTY_STRING (licenseId, LicenseID)
	PROPERTY_STRING (targetFolder, TargetFolder)
	PROPERTY_STRING (sourceFolder, SourceFolder)
	PROPERTY_STRING (unpackFolder, UnpackFolder)
	PROPERTY_STRING (description, Description)
	PROPERTY_VARIABLE (double, fileSize, FileSize)
	PROPERTY_BOOL (restartRequired, RestartRequired)
	PROPERTY_BOOL (recommended, Recommended)
	PROPERTY_BOOL (minimum, Minimum)
	PROPERTY_BOOL (useSharedLocation, UsingSharedLocation) // install to shared location (company-wide, not app-specific)
	PROPERTY_BOOL (ambiguousParent, ParentAmbiguous) // parent id is ambiguous

	PROPERTY_STRING (finishActionName, FinishActionName)

	void addApp (RequiredApp* app);
	void addApps (const Container& apps);
	void addDependencies (const Container& dependencies);
	bool canInstall (StringRef appIdentity) const;

	enum CheckResult { kAppUnknown = -2, kAppOK = 0, kAppTooOld = -1, kAppTooNew = 1 };
	CheckResult canInstallWithVersion (StringRef appIdentity, const VersionNumber& version) const;

	const Container& getApps () const;
	const Container& getDependencies () const;

	// Item
	using Item::getTitle;
	bool load (const Storage& storage) override;
	bool save (const Storage& storage) const override;

	// IFileDescriptor
	tbool CCL_API getFileType (FileType& fileType) const override;

	CLASS_INTERFACE (IFileDescriptor, Item)

protected:
	ObjectArray appList;
	ObjectArray dependencies;

	const RequiredApp* findApp (StringRef appIdentity, MetaClassRef typeId) const;

	// IFileDescriptor
	tbool CCL_API getTitle (String& title) const override;
	tbool CCL_API getFileName (String& fileName) const override;
	tbool CCL_API getFileSize (int64& fileSize) const override;
	tbool CCL_API getFileTime (DateTime& fileTime) const override;
	tbool CCL_API getMetaInfo (IAttributeList& a) const override;
};

//************************************************************************************************
// Install::Executable
//************************************************************************************************

class Executable: public File
{
public:
	DECLARE_CLASS (Executable, File)
};

//************************************************************************************************
// Install::DependentItem
//************************************************************************************************

class DependentItem: public Item
{
public:
	DECLARE_CLASS (DependentItem, Item)
};

//************************************************************************************************
// Install::RequiredApp
//************************************************************************************************

class RequiredApp: public Item
{
public:
	DECLARE_CLASS (RequiredApp, Item)

	RequiredApp (StringRef id = nullptr);

	PROPERTY_OBJECT (VersionNumber, minVersion, MinVersion)
	PROPERTY_OBJECT (VersionNumber, maxVersion, MaxVersion)

	// Item
	bool load (const Storage& storage) override;
	bool save (const Storage& storage) const override;
};

//************************************************************************************************
// Install::ExcludedApp
//************************************************************************************************

class ExcludedApp: public RequiredApp
{
public:
	DECLARE_CLASS (ExcludedApp, RequiredApp)
};

//************************************************************************************************
// Install::RequiredFramework
//************************************************************************************************

class RequiredFramework: public RequiredApp
{
public:
	DECLARE_CLASS (RequiredFramework, RequiredApp)
};

//************************************************************************************************
// Install::Action
//************************************************************************************************

class Action: public ContainerItem
{
public:
	DECLARE_CLASS (Action, ContainerItem)

	DECLARE_STRINGID_MEMBER (kExecute)

	PROPERTY_MUTABLE_CSTRING (type, Type)

	// ContainerItem
	bool load (const Storage& storage) override;
	bool save (const Storage& storage) const override;
};

//************************************************************************************************
// IncludeItem
//************************************************************************************************

class IncludeItem: public Item
{
public:
	DECLARE_CLASS (IncludeItem, Item)

	PROPERTY_STRING (url, Url)

	// Item
	bool load (const Storage& storage) override;
	bool save (const Storage& storage) const override;
};

//************************************************************************************************
// Install::Manifest
//************************************************************************************************

class Manifest: public StorableObject
{
public:
	DECLARE_CLASS (Manifest, StorableObject)

	Manifest ();

	static const CString kResourceID;
	static const String kFileName;

	PROPERTY_OBJECT (VersionNumber, version, Version)

	void addPackage (Package* package);
	void addFile (File* file);

	const Container& getPackages () const;
	const Container& getFiles () const;
	const Container& getIncludes () const;

	File* getFirstFile () const;
	File* findFile (StringRef fileId) const;
	void getFilesForPackage (Container& result, StringRef packageId, const FileTypeFilter* filter = nullptr) const;
	Medium* getFirstMedium () const;
	Medium* findMedium (StringRef mediumId) const;
	Action* findAction (StringRef actionId) const;

	void merge (Manifest& other);	///< merge items into this manifest
	void removeAll ();

	bool loadFromBase64 (StringRef data);
	String toBase64 () const;

	// StorableObject
	bool load (const Storage& storage) override;
	bool save (const Storage& storage) const override;

protected:
	ObjectArray mediumList;
	ObjectArray packageList;
	ObjectArray fileList;
	ObjectArray actionList;
	ObjectArray includeList;

	static void mergeItems (Container& dstList, Container& srcList);
};

//************************************************************************************************
// Install::ManifestLoader
//************************************************************************************************

class ManifestLoader
{
public:
	ManifestLoader (Manifest& root);

	bool loadAll (UrlRef path, StringRef includeFilter = nullptr);	///< load root and all included manifests

protected:
	Manifest& root;
};

//************************************************************************************************
// Install::HistoryEntry
//************************************************************************************************

class HistoryEntry: public Item
{
public:
	DECLARE_CLASS (HistoryEntry, Item)

	HistoryEntry (StringRef id = nullptr, StringRef title = nullptr);

	PROPERTY_OBJECT (DateTime, date, Date)
	PROPERTY_OBJECT (Url, path, Path)

	// Item
	bool load (const Storage& storage) override;
	bool save (const Storage& storage) const override;
};

//************************************************************************************************
// Install::History
//************************************************************************************************

class History: public StorableObject
{
public:
	DECLARE_CLASS (History, StorableObject)

	History ();

	static const String kFileName;
	static void getLocation (Url& path, bool shared);

	PROPERTY_OBJECT (VersionNumber, version, Version)

	HistoryEntry* lookup (StringRef id) const;
	HistoryEntry* setInstalled (const Item& item, const DateTime& date, UrlRef path);
	bool setFailed (const Item& item);

	bool store (const VersionNumber& newVersion);
	bool restore ();

	// StorableObject
	bool load (const Storage& storage) override;
	bool save (const Storage& storage) const override;

protected:
	ObjectArray entries;
};

} // namespace Install
} // namespace CCL

#endif // _ccl_installdata_h
