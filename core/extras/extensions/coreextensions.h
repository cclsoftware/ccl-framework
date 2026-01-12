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
// Filename    : core/extras/extensions/coreextensions.h
// Description : Extension Management
//
//************************************************************************************************

#ifndef _coreextensions_h
#define _coreextensions_h

#include "core/extras/extensions/coremetainfo.h"
#include "core/extras/extensions/coresignature.h"

#include "core/portable/corefile.h"
#include "core/portable/corepluginmanager.h"

namespace Core {
namespace Portable {

class OutputStorage;

//////////////////////////////////////////////////////////////////////////////////////////////////
/*
	This is a mini ecosystem for extension management on embedded platforms,
	inspired by classes with same or similar names in CCL.
*/
//////////////////////////////////////////////////////////////////////////////////////////////////

//************************************************************************************************
// PackageInfo
//************************************************************************************************

class PackageInfo: public Attributes
{
public:
	PackageInfo ();

	CStringPtr getID () const { return getString (Meta::kID); }
	CStringPtr getName () const { return getString (Meta::kName); }
	CStringPtr getVersion () const { return getString (Meta::kVersion); }

	bool loadFromPackage (FilePackage& package);
};

//************************************************************************************************
// LicenseManager
//************************************************************************************************

class LicenseManager: public StaticSingleton<LicenseManager>
{
public:
	LicenseManager ();
	~LicenseManager ();

	static CStringPtr kFolderName;
	static CStringPtr kUserFileName;
	static CStringPtr kExtensionFileName;

	// configuration
	PROPERTY_OBJECT (FileName, licenseFolder, LicenseFolder)
	PROPERTY_VARIABLE (CStringPtr, systemKey, SystemKey)
	PROPERTY_CSTRING_BUFFER (32, systemIdentity, SystemIdentity)
	PROPERTY_CSTRING_BUFFER (32, appIdentity, AppIdentity)
	PROPERTY_BOOL (useBinaryFormat, UseBinaryFormat)
	PROPERTY_POINTER (Security::SignatureVerifier, signatureVerifier, SignatureVerifier)

	void setPublicKey (const void* data, uint32 length, bool copy = false);

	// token (aka license file) management
	void restoreAll ();

	PROPERTY_CSTRING_BUFFER (32, userIdentity, UserIdentity)
	PROPERTY_CSTRING_BUFFER (64, userDisplayName, UserDisplayName)

	bool setUserToken (CStringPtr token, int length);
	void removeUserToken ();
	bool getSystemFromUserToken (CString32& systemId, CStringPtr token, int length) const;

	bool setExtensionTokens (const AttributeQueue* tokenArray); // null to reset

	enum LicenseStatus
	{
		kNotFound,
		kValid,
		kInvalid
		//TODO: kExpired
	};

	LicenseStatus getLicenseStatus (CStringPtr id) const;

	// authorization policy
	const Attributes* getPrivileges (CStringPtr resourceSid) const;
	bool isAccessible (CStringPtr resourceSid, CStringPtr itemSid) const
	{
		if(const Attributes* resource = getPrivileges (resourceSid))
			if(const Attributes* app = AuthorizationPolicy::findMatchingItem (*resource, appIdentity))
				return AuthorizationPolicy::checkAccess (*app, itemSid);
		return false;
	}

protected:
	IO::Buffer publicKey;

	struct Item
	{
		CString64 id;
		LicenseStatus state;

		Item (CStringPtr id = 0)
		: id (id),
		  state (kNotFound)
		{}
	};

	Vector<Item> items;
	Attributes policy;

	bool decodeAndVerifyToken (Attributes& claims, CStringPtr token, int length) const;
	bool setUserTokenInternal (CStringPtr token, int length);
	bool saveToken (CStringPtr fileName, CStringPtr token, int length);
	void saveExtensionTokens (OutputStorage& storage, const AttributeQueue& tokenArray) const;
	void loadExtensionTokens (const AttributeQueue& tokenArray);
	bool addToPolicy (CStringPtr jsonSnippet, int length);
};

//************************************************************************************************
// ProductItem
//************************************************************************************************

class ProductItem
{
public:
	ProductItem (CStringPtr id = 0, CStringPtr name = 0)
	: id (id),
	  name (name)
	{}

	PROPERTY_CSTRING_BUFFER (64, id, ID)
	PROPERTY_CSTRING_BUFFER (64, name, Name)

	typedef FixedSizeVector<UIDBytes, 1> ClassIDVector; // limited to 1 for now

	bool addClassID (UIDRef cid) { return classIDs.add (cid); };
	const ClassIDVector& getClassIDs () { return classIDs; };

protected:
	ClassIDVector classIDs;
};

//************************************************************************************************
// ProductBundle
//************************************************************************************************

class ProductBundle: public Vector<ProductItem*>
{
public:
	~ProductBundle ();

	void deleteAll ();
	bool loadFromPackage (FilePackage& package, CStringPtr baseFolder = 0);
};

//************************************************************************************************
// ExtensionDescription
//************************************************************************************************

class ExtensionDescription
{
public:
	ExtensionDescription (FilePackage& package, PackageInfo& info);
	~ExtensionDescription ();

	PROPERTY_OBJECT (FileName, fileName, FileName)
	PROPERTY_VARIABLE (int, useCount, UseCount) ///< incremented by extension handler

	FilePackage& getPackage () { return package; }
	const PackageInfo& getInfo () const { return info; }
	bool addSubPackage (FilePackage* package);

	PROPERTY_POINTER (ProductBundle, products, Products) ///< products contained in extension

protected:
	FilePackage& package;
	PackageInfo& info;
	FixedSizeVector<FilePackage*, 2> subPackages;
};

//************************************************************************************************
// IExtensionHandler
//************************************************************************************************

struct IExtensionHandler
{
	virtual void startupExtension (ExtensionDescription& e) = 0;
};

//************************************************************************************************
// ExtensionManager
//************************************************************************************************

class ExtensionManager: public StaticSingleton<ExtensionManager>
{
public:
	ExtensionManager ();
	~ExtensionManager ();

	static CStringPtr kFolderName;
	static CStringPtr kFileType;
	static CStringPtr kUpdateFileType;

	PROPERTY_OBJECT (FileName, extensionFolder, ExtensionFolder)
	PROPERTY_BOOL (restartRequired, RestartRequired)

	void scanAll (); ///< initial scan
	void rescanAll ();
	bool hasFile (CStringPtr fileName) const;
	void makeUpdateFile (FileName& fileName) const;

	void setDeleteOnStart (bool state);
	bool isDeletePending () const;

	bool addMemoryFile (const void* data, uint32 size, CStringPtr fileName);

	bool hasExtensions () const { return !extensions.isEmpty (); }
	const ExtensionDescription* findExtension (CStringPtr id) const;
	const ConstVector<ExtensionDescription*>& getExtensions () const { return extensions; }

	void startupForHandler (IExtensionHandler& handler);

	void saveIndex (IO::Stream& stream, bool useBinaryFormat) const;
	void saveIndex (OutputStorage& storage) const;

protected:
	static CStringPtr kDeleteMarker;

	bool rescanning;
	Vector<ExtensionDescription*> extensions;

	void scanFolder (CStringPtr folder);
	ExtensionDescription* scanPackage (FilePackage& package, CStringPtr baseFolder);
};

//************************************************************************************************
// ExtensionPluginHandler
//************************************************************************************************

class ExtensionPluginHandler: public IExtensionHandler
{
public:
	~ExtensionPluginHandler ();

	static CStringPtr kFolderName;
	static CStringPtr kBuiltInFileType;

	PROPERTY_CSTRING_BUFFER (32, platformSubFolder, PlatformSubFolder)

	void addAvailableResource (BuiltInCodeResource* codeResource); ///< takes ownership!

	// IExtensionHandler
	void startupExtension (ExtensionDescription& e) override;

protected:
	Vector<BuiltInCodeResource*> availableResources;
};

//************************************************************************************************
// ExtensionSkinHandler
//************************************************************************************************

class ExtensionSkinHandler: public IExtensionHandler
{
public:
	ExtensionSkinHandler ();

	static CStringPtr kFolderName;

	PROPERTY_BOOL (delayBitmapDecoding, DelayBitmapDecoding)

	// IExtensionHandler
	void startupExtension (ExtensionDescription& e) override;
};

} // namespace Portable
} // namespace Core

#endif // _coreextensions_h
