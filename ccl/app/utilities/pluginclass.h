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
// Filename    : ccl/app/utilities/pluginclass.h
// Description : Plug-In Class
//
//************************************************************************************************

#ifndef _ccl_pluginclass_h
#define _ccl_pluginclass_h

#include "ccl/public/app/ipluginpresentation.h"

#include "ccl/app/utilities/sortfolderlist.h"

#include "ccl/base/singleton.h"
#include "ccl/base/storage/attributes.h"
#include "ccl/base/collections/objecthashtable.h"
#include "ccl/base/collections/stringlist.h"

#include "ccl/public/gui/graphics/iimage.h"

namespace CCL {

class Url;
interface IUrlFilter;
interface IClassDescription;

class PlugInCategory;

//************************************************************************************************
// Access process-wide IPluginPresentation / IPlugInSnapshots singletons
//************************************************************************************************

namespace System
{
	extern IPluginPresentation& GetPluginPresentation ();
	extern IPlugInSnapshots& GetPluginSnapshots ();
}

//************************************************************************************************
// PlugIn
//************************************************************************************************

namespace PlugIn
{
	enum SearchOptions
	{
		kCheckKnownLocation = 1 ///< Check if the path matches a known plug-in search path
	};
	
	/** Get the path of a plug-in module based on a plug-in class description. */
	bool getModulePath (IUrl& modulePath, const IClassDescription& description, int searchOptions = 0);

	/** Get the path of a plug-in module based on a plug-in class ID. */
	bool getModulePath (IUrl& modulePath, UIDRef classID, int searchOptions = 0);

	/** Check if a given path is a plug-in module or a file or folder inside a plug-in bundle and return the module path. */
	bool findModulePath (IUrl& path);
	
	/** Find plug-in module paths in a list of input paths. */
	void findModulePaths (IUnknownList& pluginPaths, const IUnknownList& paths);

	/** Get alternative class IDs for a given main class ID. */
	void getAlternativeCIDs (Vector<UID>& altIDs, UIDRef classID);

	/** Check if an ID is an alternative of the main class ID. */
	bool isAlternativeCID (UIDRef classID, UIDRef altID);
	
	/** Find plug-ins which may differ in format or version, but otherwise equal the plug-in with the given ID. */
	void findDuplicates (Vector<UID>& classIds, UIDRef classID);
}

//************************************************************************************************
// PlugInClass
//************************************************************************************************

class PlugInClass: public Object
{
public:
	DECLARE_CLASS (PlugInClass, Object)

	PlugInClass (UIDRef classID = kNullUID, StringRef name = nullptr, StringRef category = nullptr);
	PlugInClass (const IClassDescription& description);

	PROPERTY_OBJECT (UID, classID, ClassID)
	PROPERTY_STRING (name, Name)
	PROPERTY_STRING (category, Category)
	PROPERTY_STRING (subCategory, SubCategory)

	void assign (const IClassDescription& description);
	void parseClassName (StringRef className); ///< UID string or "category:name"
	void setCategories (const PlugInCategory& categories);
	void getPresetMetaInfo (IAttributeList& attr) const;

	IImage* getExactIcon (bool withSubCategory = true) const;
	IImage* getCategoryIcon () const;
	IImage* getIcon (bool exact = false) const;	///< get icon from main theme

	String getClassVendor () const; ///< get class vendor from plug-in manager
	String getClassFolder () const; ///< get class folder from plug-in manager

	PROPERTY_STRING (title, Title) ///< overrides name for display (optional)
	PROPERTY_VARIABLE (int, menuPriority, MenuPriority) ///< sorting order in menus (optional)

	String makeTitleWithVendor () const; ///< vendor name and title

	void setClassAttribute (StringID id, VariantRef value);
	bool getClassAttribute (Variant& value, StringID id) const;
	String getClassAttributeString (StringID id) const;

	// Object
	bool toString (String& string, int flags = 0) const override;
	bool equals (const Object& obj) const override;
	int compare (const Object& obj) const override;
	bool load (const Storage& storage) override;
	bool save (const Storage& storage) const override;

protected:
	AutoPtr<Attributes> additionalAttributes;
};

//************************************************************************************************
// PlugInCategory
//************************************************************************************************

class PlugInCategory: public Object
{
public:
	DECLARE_CLASS (PlugInCategory, Object)

	PlugInCategory (StringRef category = nullptr, StringRef subCategory = nullptr, StringRef title = nullptr);

	PROPERTY_STRING (category, Category)
	PROPERTY_STRING (subCategory, SubCategory)

	IImage* getIcon () const; ///< get icon from main theme

	PROPERTY_STRING (title, Title) ///< overrides category for display (optional)

	// Object
	bool toString (String& string, int flags = 0) const override;
	bool equals (const Object& obj) const override;
};

//************************************************************************************************
// PlugInMetaInfo
//************************************************************************************************

class PlugInMetaInfo: public Object
{
public:
	DECLARE_CLASS (PlugInMetaInfo, Object)

	PlugInMetaInfo ();
	PlugInMetaInfo (UIDRef cid);

	bool assign (UIDRef cid);

	PROPERTY_SHARED_AUTO (IImage, image, Image)
	PROPERTY_STRING (text, Text)
};

//************************************************************************************************
// PlugInSnapshots
//************************************************************************************************

class PlugInSnapshots: public Object,
					   public Singleton<PlugInSnapshots>,
					   public IPlugInSnapshots
{
public:
	PlugInSnapshots ();

	static String getTranslatedTitle ();
	static const String kFolderName;	///< snapshot folder name
	static const String kFileName;		///< name of snapshot JSON file
	static IUrlFilter* createBackupFilter ();
	static void getAppLocation (IUrl& path);
	static void getUserLocation (IUrl& path);

	int addDefaultLocations ();
	int addUserLocations ();

	void addSkinSnapshot (UIDRef cid, StringID imageName);

	// IPlugInSnapshots
	int CCL_API addLocation (UrlRef path, tbool deep = true) override;
	tbool CCL_API addSnapshotFile (UrlRef path) override;
	void CCL_API removeLocation (UrlRef path) override;
	tbool CCL_API hasLocation (UrlRef path) const override;

	CLASS_INTERFACE (IPlugInSnapshots, Object)

protected:
	class SnapshotItem;
	class SnapshotPackage;

	ObjectArray packageList;
	ObjectHashTable itemTable;
	SnapshotPackage* skinPackage;

	SnapshotItem* lookup (UIDRef cid) const;
	bool addToTable (SnapshotItem* item, bool replace = false);
	void addPackage (SnapshotPackage* package);
	SnapshotPackage* findPackageForFolder (UrlRef folder) const;
	bool findPackagesForFolder (Vector<SnapshotPackage*>& result, UrlRef folder) const;

	bool makeSnapshotFiles (IUrl& packageFolder, String& defaultFileName, UIDRef cid, IImage* image);
	bool removeSnapshotFiles (IUrl& packageFolder, UIDRef cid);
	void getNamesForClass (String& fileName, String& folderName, UIDRef cid) const;
	void restoreDefaultSnapshot (UIDRef cid);
	
	// IPlugInSnapshots
	IImage* CCL_API getSnapshot (UIDRef cid, StringID which = kDefault) const override;
	tbool CCL_API hasUserSnapshot (UIDRef cid) const override;
	tbool CCL_API setUserSnapshot (UIDRef cid, IImage* image) override;
	tbool CCL_API getSnapshotDescription (String& description, UIDRef cid, StringID which = kDefault) const override;
	tbool CCL_API isHighlight (UIDRef cid) const override;
	tbool CCL_API setDefaultSnapshot (UrlRef snapshotFile, UIDRef cid, UrlRef imageFile1x, UrlRef imageFile2x) override;
};

//************************************************************************************************
// PluginPresentation
//************************************************************************************************

class PluginPresentation: public Object,
						  private Singleton<PluginPresentation>,
						  public IPluginPresentation
{
public:
	PluginPresentation ();
	~PluginPresentation ();

	static String getParentSortFolder (StringRef path);
	static void getSettingsPath (IUrl& path);

	// IPluginPresentation
	tbool CCL_API isHidden (UIDRef cid) const override;
	void CCL_API setHidden (UIDRef cid, tbool state) override;
	tbool CCL_API isFavorite (UIDRef cid) const override;
	String CCL_API getFavoriteFolder (UIDRef cid) const override;
	void CCL_API setFavorite (UIDRef cid, tbool state, StringRef folder = nullptr) override;
	int64 CCL_API getLastUsage (UIDRef cid) const override;
	void CCL_API logUsage (UIDRef cid) override;
	tbool CCL_API isSystemScalingEnabled (UIDRef cid) const override;
	void CCL_API setSystemScalingEnabled (UIDRef cid, tbool state) override;
	tbool CCL_API getAttribute (Variant& value, UIDRef cid, StringID attrId) const override;
	void CCL_API setAttribute (UIDRef cid, StringID attrId, VariantRef value) override;
	void CCL_API removeAttribute (UIDRef cid, StringID attrId) override;
	String CCL_API getSortPath (UIDRef cid) const override;
	void CCL_API setSortPath (UIDRef cid, StringRef path) override;
	void CCL_API addSortFolder (CategoryRef category, StringRef path) override;
	void CCL_API removeSortFolder (CategoryRef category, StringRef path) override;
	void CCL_API moveSortFolder (CategoryRef category, StringRef oldPath, StringRef newPath) override;
	void CCL_API renameSortFolder (CategoryRef category, StringRef path, StringRef newName) override;
	IUnknownIterator* CCL_API getSortFolders (CategoryRef category) const override;
	tbool CCL_API hasSortFolder (CategoryRef category, StringRef path) const override;
	void CCL_API addFavoriteFolder (CategoryRef category, StringRef path) override;
	void CCL_API removeFavoriteFolder (CategoryRef category, StringRef path) override;
	void CCL_API moveFavoriteFolder (CategoryRef category, StringRef oldPath, StringRef newPath) override;
	void CCL_API renameFavoriteFolder (CategoryRef category, StringRef path, StringRef newName) override;
	IUnknownIterator* CCL_API getFavoriteFolders (CategoryRef category) const override;
	tbool CCL_API hasFavoriteFolder (CategoryRef category, StringRef path) const override;
	void CCL_API saveSettings () override;
	void CCL_API reset () override;
	void CCL_API revert () override;

	CLASS_INTERFACE (IPluginPresentation, Object)

private:
	static const String kSettingsName;
	ObjectArray plugInfos;
	ObjectHashTable plugInfoTable;
	mutable SortFolderListCollection sortFolders;
	mutable SortFolderListCollection favoriteFolders;
	int version;

	class PlugInfo;
	class PlugSortFolderList;

	void loadSettings ();
	const PlugInfo* lookup (UIDRef cid) const;
	PlugInfo* getPlugInfo (UIDRef cid, bool create);
	SortFolderList* getSortFolderList (CategoryRef category) const;
	SortFolderList* getFavoriteFolderList (CategoryRef category) const;
	static String getInitialSortPath (UIDRef cid);

	class FolderTraits;
	class SortFolderTraits;
	class FavoriteFolderTraits;
	void removeFolderInternal (const FolderTraits& folderTraits, CategoryRef category, StringRef folderPath);
	void moveFolderInternal (const FolderTraits& folderTraits, CategoryRef category, StringRef oldPath, StringRef newPath);
	void renameFolderInternal (const FolderTraits& folderTraits, CategoryRef category, StringRef path, StringRef newName);
	bool hasFolderInternal (const FolderTraits& folderTraits, CategoryRef category, StringRef path) const;

	friend IPluginPresentation& System::GetPluginPresentation ();

	static String makeLegalFolderPath (StringRef pathString);
};

//************************************************************************************************
// PlugInSettingsHelper
//************************************************************************************************

struct PlugInSettingsHelper
{
	/** Used by application during startup to query if plug-in settings should be removed. */
	static bool isRemoveMarkerPresent ();
	static void makeRemoveMarker (bool state);

private:
	static void getRemoveMarkerFile (Url& path);
};

} // namespace CCL

#endif // _ccl_pluginclass_h
