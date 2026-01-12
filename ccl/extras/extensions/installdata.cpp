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
// Filename    : ccl/extras/extensions/installdata.cpp
// Description : Installer Data
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "ccl/extras/extensions/installdata.h"

#include "ccl/base/boxedtypes.h"
#include "ccl/base/storage/storage.h"
#include "ccl/base/storage/file.h"
#include "ccl/base/security/cryptomaterial.h"

#include "ccl/public/system/formatter.h"
#include "ccl/public/system/isysteminfo.h"
#include "ccl/public/system/ilocalemanager.h"
#include "ccl/public/system/ipackagemetainfo.h"
#include "ccl/public/systemservices.h"
#include "ccl/public/cclversion.h"

using namespace CCL;
using namespace Install;

//************************************************************************************************
// Install::Item
//************************************************************************************************

DEFINE_CLASS_PERSISTENT (Item, Object, "InstallItem")

//////////////////////////////////////////////////////////////////////////////////////////////////

Item::Item (StringRef id, StringRef title)
: id (id),
  title (title)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Item::equals (const Object& obj) const
{
	const Item* other = ccl_cast<Item> (&obj);
	return other ? id == other->id : SuperClass::equals (obj);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Item::load (const Storage& storage)
{
	Attributes& a = storage.getAttributes ();
	id = a.getString ("id");
	title = a.getString ("title");
	iconName = a.getString ("icon");
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Item::save (const Storage& storage) const
{
	Attributes& a = storage.getAttributes ();
	if(!id.isEmpty ())
		a.set ("id", id);
	if(!title.isEmpty ())
		a.set ("title", title);
	if(!iconName.isEmpty ())
		a.set ("icon", iconName);
	return true;
}

//************************************************************************************************
// Install::ContainerItem
//************************************************************************************************

DEFINE_CLASS_HIDDEN (ContainerItem, Item)

//////////////////////////////////////////////////////////////////////////////////////////////////

ContainerItem::ContainerItem ()
{
	children.objectCleanup (true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const Container& ContainerItem::getChildren () const
{
	return children;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Item* ContainerItem::findChild (StringRef id) const
{
	return (Item*)children.findEqual (Item (id));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ContainerItem::load (const Storage& storage)
{
	if(!SuperClass::load (storage))
		return false;

	Attributes& a = storage.getAttributes ();

	AutoPtr<Item> item;
	while((item = a.unqueueObject<Item> (nullptr)) != nullptr)
		children.add (item.detach ());

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ContainerItem::save (const Storage& storage) const
{
	SuperClass::save (storage);

	Attributes& a = storage.getAttributes ();
	a.queue (nullptr, children);
	return true;
}

//************************************************************************************************
// Install::Medium
//************************************************************************************************

DEFINE_CLASS_PERSISTENT (Medium, Item, "InstallMedium")

//////////////////////////////////////////////////////////////////////////////////////////////////

Medium::Medium ()
: autoDetect (false)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Medium::load (const Storage& storage)
{
	if(!SuperClass::load (storage))
		return false;

	Attributes& a = storage.getAttributes ();
	autoDetect = a.getBool ("detect");
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Medium::save (const Storage& storage) const
{
	SuperClass::save (storage);

	Attributes& a = storage.getAttributes ();
	a.set ("detect", autoDetect);
	return true;
}

//************************************************************************************************
// Install::Package
//************************************************************************************************

DEFINE_CLASS_PERSISTENT (Package, ContainerItem, "InstallPackage")

//************************************************************************************************
// Install::File
//************************************************************************************************

DEFINE_CLASS_PERSISTENT (Install::File, Item, "InstallFile")

//////////////////////////////////////////////////////////////////////////////////////////////////

Install::File::File (StringRef id)
: Item (id),
  fileSize (0),
  restartRequired (false),
  recommended (false),
  minimum (false),
  useSharedLocation (false),
  ambiguousParent (false)
{
	appList.objectCleanup (true);
	dependencies.objectCleanup (true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Install::File::isFolder () const
{
	return fileName.endsWith (Url::strPathChar);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Install::File::addApp (RequiredApp* app)
{
	appList.add (app);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Install::File::addApps (const Container& apps)
{
	appList.add (apps, Container::kClone);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Install::File::addDependencies (const Container& dependencyList)
{
	ForEach (dependencyList, DependentItem, dependency)
		if(dependency->getID () != getID ())
			dependencies.add (dependency->clone ());
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const RequiredApp* Install::File::findApp (StringRef appIdentity, MetaClassRef typeId) const
{
	// Note: RequiredApp items might contain wildcards.
	ArrayForEach (appList, RequiredApp, app)
		if(!app->isClass (typeId))
			continue;
		AutoPtr<SearchDescription> description = SearchDescription::create (Url (), app->getID (), SearchDescription::kMatchWholeWord);
		if(description->matchesName (appIdentity))
			return app;
	EndFor
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Install::File::canInstall (StringRef appIdentity) const
{
	// check if excluded
	if(findApp (appIdentity, ccl_typeid<ExcludedApp> ()))
		return false;

	return findApp (appIdentity, ccl_typeid<RequiredApp> ()) != nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Install::File::CheckResult Install::File::canInstallWithVersion (StringRef appIdentity, const VersionNumber& appVersion) const
{
	// check if excluded
	if(findApp (appIdentity, ccl_typeid<ExcludedApp> ()))
		return kAppUnknown;

	const RequiredApp* app = findApp (appIdentity, ccl_typeid<RequiredApp> ());
	if(app == nullptr)
		return kAppUnknown;

	if(!appVersion.isWithin (app->getMinVersion (), app->getMaxVersion ()))
	{
		if(appVersion < app->getMinVersion ())
			return kAppTooOld;
		return kAppTooNew;
	}

	// check framework
	static const String kFrameworkID (CCLSTR ("ccl"));
	static const VersionNumber kFrameworkVersion (CCL_VERSION_MAJOR, CCL_VERSION_MINOR, CCL_VERSION_REVISION, CCL_VERSION_BUILD);

	static const String kFrameworkABI_ID (CCLSTR ("ccl-abi"));
	static const VersionNumber kFrameworkABIVersion (CCL_ABI_VERSION);

	auto checkFramework = [this] (StringRef id, const VersionNumber& version)
	{
		if(RequiredFramework* frame = ccl_cast<RequiredFramework> (appList.findEqual (Item (id))))
			if(!version.isWithin (frame->getMinVersion (), frame->getMaxVersion ()))
			{
				if(version < frame->getMinVersion ())
					return kAppTooOld;
				return kAppTooNew;
			}
		return kAppOK;
	};

	auto result = checkFramework (kFrameworkID, kFrameworkVersion);
	if(result != kAppOK)
		return result;

	result = checkFramework (kFrameworkABI_ID, kFrameworkABIVersion);
	if(result != kAppOK)
		return result;

	return kAppOK;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const Container& Install::File::getApps () const
{
	return appList;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const Container& Install::File::getDependencies () const
{
	return dependencies;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API Install::File::getTitle (String& title) const
{
	title = getTitle ();
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API Install::File::getFileName (String& fileName) const
{
	fileName = getFileName ();
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API Install::File::getFileType (FileType& fileType) const
{
	Url path;
	path.setName (getFileName ());
	fileType = path.getFileType ();
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API Install::File::getFileSize (int64& _fileSize) const
{
	_fileSize = static_cast<int64> (fileSize);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API Install::File::getFileTime (DateTime& fileTime) const
{
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API Install::File::getMetaInfo (IAttributeList& a) const
{
	AttributeAccessor acc (a);
	acc.set (Meta::kPackageID, getID ());
	acc.set (Meta::kPackageSharedLocation, useSharedLocation);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Install::File::load (const Storage& storage)
{
	Attributes& a = storage.getAttributes ();
	parentId = a.getString ("parent");
	fileName = a.getString ("file");
	mediumId = a.getString ("medium");
	licenseId = a.getString ("license");
	sourceFolder = a.getString ("source");
	targetFolder = a.getString ("target");
	unpackFolder = a.getString ("unpack");

	Variant size;
	Format::ByteSize::scan (size, a.getString ("size"));
	fileSize = size.asDouble ();

	restartRequired = a.getBool ("restart");

	auto getBoolForLanguage = [] (Attributes& a, StringID key)
	{
		MutableCString langKey (key);
		langKey += "-";
		langKey += System::GetLocaleManager ().getLanguage ();
		if(a.contains (langKey))
			return a.getBool (langKey);
		else 
			return a.getBool (key);
	};

	recommended = getBoolForLanguage (a, "recommended");
	minimum = getBoolForLanguage (a, "minimum");
	useSharedLocation = a.getBool ("sharedLocation");

	finishActionName = a.getString ("finishAction");
	description = a.getString ("description");

	AutoPtr<Item> item;
	while((item = a.unqueueObject<Item> (nullptr)) != nullptr)
	{
		if(RequiredApp* app = ccl_cast<RequiredApp> (item))
			appList.add (item.detach ());
		else if(DependentItem* dependent = ccl_cast<DependentItem> (item))
			dependencies.add (item.detach ());
	}

	return SuperClass::load (storage);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Install::File::save (const Storage& storage) const
{
	SuperClass::save (storage);

	Attributes& a = storage.getAttributes ();

	if(!parentId.isEmpty ())
		a.set ("parent", parentId);
	if(!fileName.isEmpty ())
		a.set ("file", fileName);
	if(!mediumId.isEmpty ())
		a.set ("medium", mediumId);
	if(!licenseId.isEmpty ())
		a.set ("license", licenseId);
	if(!sourceFolder.isEmpty ())
		a.set ("source", sourceFolder);
	if(!targetFolder.isEmpty ())
		a.set ("target", targetFolder);
	if(!unpackFolder.isEmpty ())
		a.set ("unpack", unpackFolder);

	if(fileSize != 0.)
		a.set ("size", Format::ByteSize::print (fileSize));

	if(restartRequired)
		 a.set ("restart", true);
	if(recommended)
		a.set ("recommended", true);
	if(minimum)
		a.set ("minimum", true);

	if(!finishActionName.isEmpty ())
		a.set ("finishAction", finishActionName);

	if(!description.isEmpty ())
		a.set ("description", description);

	if(useSharedLocation)
		a.set ("sharedLocation", true);
		
	a.queue (nullptr, appList);
	a.queue (nullptr, dependencies);

	return true;
}

//************************************************************************************************
// Install::Executable
//************************************************************************************************

DEFINE_CLASS_PERSISTENT (Executable, File, "InstallExecute")

//************************************************************************************************
// Install::DependentItem
//************************************************************************************************

DEFINE_CLASS_PERSISTENT (DependentItem, Item, "InstallDependent")

//************************************************************************************************
// Install::RequiredApp
//************************************************************************************************

DEFINE_CLASS (RequiredApp, Item)
static const VersionNumber kUnlimitedVersion (1000, 1000, 1000, 1000000);

//////////////////////////////////////////////////////////////////////////////////////////////////

RequiredApp::RequiredApp (StringRef id)
: Item (id)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool RequiredApp::load (const Storage& storage)
{
	if(!SuperClass::load (storage))
		return false;

	Attributes& a = storage.getAttributes ();

	minVersion.scan (a.getString ("minVersion"));

	String maxString (a.getString ("maxVersion"));
	maxString.trimWhitespace ();
	if(maxString.isEmpty () || maxString == CCLSTR ("*"))
		maxVersion = kUnlimitedVersion;
	else
		maxVersion.scan (maxString);

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool RequiredApp::save (const Storage& storage) const
{
	SuperClass::save (storage);

	Attributes& a = storage.getAttributes ();

	if(minVersion != VersionNumber ())
		a.set ("minVersion", minVersion.print ());

	if(minVersion != VersionNumber ())
	{
		if(maxVersion >= kUnlimitedVersion)
			a.set ("maxVersion", "*");
		else
			a.set ("maxVersion", maxVersion.print ());
	}

	return true;
}

//************************************************************************************************
// Install::ExcludedApp
//************************************************************************************************

DEFINE_CLASS (ExcludedApp, RequiredApp)

//************************************************************************************************
// Install::RequiredFramework
//************************************************************************************************

DEFINE_CLASS (RequiredFramework, RequiredApp)

//************************************************************************************************
// Install::Action
//************************************************************************************************

DEFINE_STRINGID_MEMBER_ (Action, kExecute, "execute")
DEFINE_CLASS_PERSISTENT (Action, ContainerItem, "InstallAction")

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Action::load (const Storage& storage)
{
	if(!SuperClass::load (storage))
		return false;

	Attributes& a = storage.getAttributes ();
	type = a.getCString ("type");
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Action::save (const Storage& storage) const
{
	SuperClass::save (storage);

	Attributes& a = storage.getAttributes ();
	a.set ("type", type);
	return true;
}

//************************************************************************************************
// IncludeItem
//************************************************************************************************

DEFINE_CLASS_PERSISTENT (IncludeItem, Item, "InstallInclude")

//////////////////////////////////////////////////////////////////////////////////////////////////

bool IncludeItem::load (const Storage& storage)
{
	if(!SuperClass::load (storage))
		return false;

	Attributes& a = storage.getAttributes ();
	url = a.getString ("url");
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool IncludeItem::save (const Storage& storage) const
{
	SuperClass::save (storage);

	Attributes& a = storage.getAttributes ();
	a.set ("url", url);
	return true;
}

//************************************************************************************************
// Install::Manifest
//************************************************************************************************

void Manifest::mergeItems (Container& dstList, Container& srcList)
{
	Iterator* iter = srcList.newIterator ();
	IterForEach (iter, Item, item)
		Item* existing = (Item*)dstList.findEqual (Item (item->getID ()));
		if(existing)
		{
			CCL_WARN ("Installer: Item \"%s\" already defined!\n", MutableCString (item->getID ()).str ())
		}
		else
		{
			CCL_PRINTF ("Merging installer item %s\n", MutableCString (item->getID ()).str ())

			srcList.remove (item);
			iter->previous (); // (problem with array iterator when removing items)

			dstList.add (item);
		}	
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_CLASS_PERSISTENT (Manifest, StorableObject, "InstallManifest")
const CString Manifest::kResourceID ("Install:ManifestFile");
const String Manifest::kFileName ("installdata.xml");

//////////////////////////////////////////////////////////////////////////////////////////////////

Manifest::Manifest ()
{
	mediumList.objectCleanup (true);
	packageList.objectCleanup (true);
	fileList.objectCleanup (true);
	actionList.objectCleanup (true);
	includeList.objectCleanup (true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Manifest::addPackage (Package* package)
{
	packageList.add (package);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Manifest::addFile (Install::File* file)
{
	fileList.add (file);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const Container& Manifest::getPackages () const
{
	return packageList;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const Container& Manifest::getFiles () const
{
	return fileList;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const Container& Manifest::getIncludes () const
{
	return includeList;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Install::File* Manifest::getFirstFile () const
{
	return !fileList.isEmpty () ? (Install::File*)fileList.at (0) : nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Install::File* Manifest::findFile (StringRef fileId) const
{
	if(fileId.isEmpty ())
		return nullptr;
	return (Install::File*)fileList.findEqual (Item (fileId));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Manifest::getFilesForPackage (Container& result, StringRef packageId, const FileTypeFilter* filter) const
{
	ArrayForEach (fileList, File, file)
		if(file->getParentID () == packageId)
		{
			if(filter && !file->isFolder ())
			{
				FileType fileType;
				file->getFileType (fileType);
				if(!filter->matches (fileType))
					continue;
			}
			
			result.add (file);
		}
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Medium* Manifest::getFirstMedium () const
{
	if(mediumList.isEmpty ())
		return nullptr;
	return (Medium*)mediumList.at (0);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Medium* Manifest::findMedium (StringRef mediumId) const
{
	if(mediumId.isEmpty ())
		return nullptr;
	return (Medium*)mediumList.findEqual (Item (mediumId));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Action* Manifest::findAction (StringRef actionId) const
{
	if(actionId.isEmpty ())
		return nullptr;
	return (Action*)actionList.findEqual (Item (actionId));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Manifest::merge (Manifest& other)
{
	mergeItems (mediumList, other.mediumList);
	mergeItems (packageList, other.packageList);
	mergeItems (fileList, other.fileList);
	mergeItems (actionList, other.actionList);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Manifest::removeAll ()
{
	mediumList.removeAll ();
	packageList.removeAll ();
	fileList.removeAll ();
	actionList.removeAll ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Manifest::loadFromBase64 (StringRef data)
{
	Security::Crypto::Material manifestData;
	manifestData.fromBase64 (data);
	return loadFromStream (manifestData.asStream ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

String Manifest::toBase64 () const
{
	Security::Crypto::Material manifestData;
	saveToStream (manifestData);
	return manifestData.toBase64 ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Manifest::load (const Storage& storage)
{
	Attributes& a = storage.getAttributes ();

	version.scan (a.getString ("version"));

	AutoPtr<Item> item;
	while((item = a.unqueueObject<Item> (nullptr)) != nullptr)
	{
		if(File* f = ccl_cast<File> (item))
			fileList.add (item.detach ());
		else if(Medium* m = ccl_cast<Medium> (item))
			mediumList.add (item.detach ());
		else if(Package* p = ccl_cast<Package> (item))
			packageList.add (item.detach ());
		else if(Action* a = ccl_cast<Action> (item))
			actionList.add (item.detach ());
		else if(IncludeItem* inc = ccl_cast<IncludeItem> (item))
			includeList.add (item.detach ());
	}

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Manifest::save (const Storage& storage) const
{
	Attributes& a = storage.getAttributes ();

	if(version != VersionNumber ())
		a.set ("version", version.print ());

	a.queue (nullptr, includeList);
	a.queue (nullptr, packageList);
	a.queue (nullptr, mediumList);
	a.queue (nullptr, fileList);
	a.queue (nullptr, actionList);
	return true;
}

//************************************************************************************************
// Install::ManifestLoader
//************************************************************************************************

ManifestLoader::ManifestLoader (Manifest& root)
: root (root)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ManifestLoader::loadAll (UrlRef rootPath, StringRef includeFilter)
{
	ASSERT (rootPath.isFile ())
	if(!root.loadFromFile (rootPath))
		return false;

	Url basePath (rootPath);
	basePath.ascend ();
	ASSERT (!basePath.isEmpty ())

	ForEach (root.getIncludes (), IncludeItem, item)
		// filter includes
		if(item->getID () != includeFilter)
			continue;

		ASSERT (!item->getUrl ().isEmpty ())
		Url url (item->getUrl ());
		if(url.isRelative ())
			url.makeAbsolute (basePath);

		Manifest m;
		if(m.loadFromFile (url))
			root.merge (m);
		else
			CCL_WARN ("Installer: Failed to load included manifest %s!\n", MutableCString (item->getUrl ()).str ())
	EndFor
	return true;
}

//************************************************************************************************
// Install::HistoryEntry
//************************************************************************************************

DEFINE_CLASS_PERSISTENT (HistoryEntry, Item, "InstallEntry")

//////////////////////////////////////////////////////////////////////////////////////////////////

HistoryEntry::HistoryEntry (StringRef id, StringRef title)
: Item (id, title)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool HistoryEntry::load (const Storage& storage)
{
	Attributes& a = storage.getAttributes ();

	Boxed::DateTime dt;
	a.get (dt, "date");
	date = dt;
	a.get (path, "path");
	return SuperClass::load (storage);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool HistoryEntry::save (const Storage& storage) const
{
	SuperClass::save (storage);

	Attributes& a = storage.getAttributes ();
	a.set ("date", Boxed::DateTime (date), true);
	a.set ("path", path, true);
	return true;
}

//************************************************************************************************
// Install::History
//************************************************************************************************

DEFINE_CLASS_PERSISTENT (History, StorableObject, "InstallHistory")
const String History::kFileName ("installhistory.xml");

//////////////////////////////////////////////////////////////////////////////////////////////////

void History::getLocation (Url& path, bool shared)
{
	System::GetSystem ().getLocation (path, shared ? System::kSharedAppSettingsFolder : System::kAppSettingsFolder);
	path.descend (kFileName, Url::kFile);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

History::History ()
{
	entries.objectCleanup (true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

HistoryEntry* History::lookup (StringRef id) const
{
	return (HistoryEntry*)entries.findEqual (Item (id));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

HistoryEntry* History::setInstalled (const Item& item, const DateTime& date, UrlRef path)
{
	HistoryEntry* e = lookup (item.getID ());
	if(e == nullptr)
		entries.add (e = NEW HistoryEntry (item.getID ()));

	e->setTitle (item.getTitle ());
	e->setDate (date);
	e->setPath (path);
	return e;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool History::setFailed (const Item& item)
{
	HistoryEntry* e = lookup (item.getID ());
	if(e)
	{
		entries.remove (e);
		e->release ();
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool History::store (const VersionNumber& newVersion)
{
	setVersion (newVersion);

	Url sharedHistoryPath;
	getLocation (sharedHistoryPath, true);
	if(saveToFile (sharedHistoryPath) == false) // this can happen, if the folder is not writable
	{
		Url userHistoryPath;
		getLocation (userHistoryPath, false);
        return saveToFile (userHistoryPath);
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool History::restore ()
{
	Url sharedHistoryPath;
	getLocation (sharedHistoryPath, true);
	bool loaded = loadFromFile (sharedHistoryPath);
	if(!loaded)
	{
		Url userHistoryPath;
		getLocation (userHistoryPath, false);
		loaded = loadFromFile (userHistoryPath);
		if(loaded)
			saveToFile (sharedHistoryPath); // copy from user to shared history path
	}
	return loaded;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool History::load (const Storage& storage)
{
	Attributes& a = storage.getAttributes ();

	version.scan (a.getString ("version"));
	
	a.unqueue (entries, nullptr, ccl_typeid<HistoryEntry> ());
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool History::save (const Storage& storage) const
{
	Attributes& a = storage.getAttributes ();
	
	if(version != VersionNumber ())
		a.set ("version", version.print ());
	
	a.queue (nullptr, entries);
	return true;
}
