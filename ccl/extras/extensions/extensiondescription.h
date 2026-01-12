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
// Filename    : ccl/extras/extensions/extensiondescription.h
// Description : Extension Description
//
//************************************************************************************************

#ifndef _ccl_extensiondescription_h
#define _ccl_extensiondescription_h

#include "ccl/base/storage/attributes.h"

#include "ccl/extras/extensions/installdata.h"

#include "ccl/public/extras/iextensionhandler.h"

#include "ccl/public/gui/graphics/iimage.h"

namespace CCL {

class PackageInfo;

namespace Install {

//************************************************************************************************
// ExtensionType
//************************************************************************************************

namespace ExtensionType
{
	enum Types 
	{
		kUser,		///< installed by user to application-specific location
		kProgram,	///< installed with the application inside program folder/bundle
		kShared,	///< installed by user for multiple applications into shared location
		kDeveloper	///< developer mode (debug build only)
	};
}

typedef ExtensionType::Types ExtensionTypeID;

//************************************************************************************************
// ExtensionStrings
//************************************************************************************************

namespace ExtensionStrings
{
	StringRef Extension ();
	StringRef InvalidInstallFile ();
	StringRef SignatureCheck ();
	StringRef InvalidSignature ();
	StringRef InvalidPlatform ();
	StringRef InvalidApplication ();
	StringRef InstallationFailed ();
	StringRef UpdatesAvailable ();
	StringRef NoUpdatesFound ();
	String AlreadyInstalled (StringRef title);
	String DeinstallationFailed (StringRef title);
	String DirectoryLocked (StringRef lockingAppName);
	StringRef ExtensionType (ExtensionTypeID id);
}

//************************************************************************************************
// ExtensionDescription
//************************************************************************************************

class ExtensionDescription: public Object,
							public IExtensionDescription
{
public:
	DECLARE_CLASS (ExtensionDescription, Object)

	ExtensionDescription (UrlRef path = Url (), StringRef id = nullptr);

	static const FileType& getFileType ();
	static StringRef getPlatformName ();
	static void replacePlatform (String& id);
	static String extractPlatform (StringRef id);

	static ExtensionDescription* createFromPackage (UrlRef path);
	static ExtensionDescription* createFromDescriptor (IFileDescriptor& descriptor);

	void setPath (UrlRef path);
	UrlRef CCL_API getPath () const override; ///< [IExtensionDescription]
	String CCL_API getPlatformIndependentIdentifier () const override; ///< [IExtensionDescription]

	PROPERTY_STRING (id, ID)
	PROPERTY_STRING (shortId, ShortID)
	PROPERTY_STRING (parentProductId, ParentProductID)	///< product identifier of purchased extensions, used for update check
	PROPERTY_STRING (title, Title)
	PROPERTY_STRING (description, Description)
	PROPERTY_STRING (vendor, Vendor)
	PROPERTY_OBJECT (VersionNumber, version, Version)
	PROPERTY_STRING (platform, Platform)
	PROPERTY_STRING (copyright, Copyright)
	PROPERTY_STRING (website, Website)
	const Attributes& getExtraInfo () const;
	bool isSilentCheckEnabled () const;
	bool isUsingAppProductID () const;
	bool isUsingSharedLocation () const;
	bool isHidden () const;
	bool canPlugInRescanInsteadRestart () const;

	class SubItem
	{
	public:
		SubItem (StringRef id = String (), StringRef title = String ())
		: id (id),
		  title (title)
		{}

		PROPERTY_STRING (id, ID)
		PROPERTY_STRING (title, Title)
			
		void addClassID (UIDRef cid) { classIDs.add (cid); };
		const Vector<UID>& getClassIDs () { return classIDs; };

	protected:
		Vector<UID> classIDs;
	};

	bool hasSubItem (StringRef id) const;
	const Vector<SubItem>& getSubItems () const;
	void addSubItem (const SubItem& item);

	PROPERTY_SHARED_AUTO (IImage, icon, Icon)
	PROPERTY_SHARED_AUTO (Manifest, manifest, Manifest)
	File* getManifestEntry () const;

	PROPERTY_VARIABLE (File::CheckResult, compatibilityResult, CompatibilityResult)
	bool isCompatible () const { return compatibilityResult == File::kAppOK; }

	PROPERTY_BOOL (enabled, Enabled)
	PROPERTY_BOOL (started, Started)
	PROPERTY_BOOL (uninstallPending, UninstallPending)
	PROPERTY_BOOL (updatePending, UpdatePending)
	PROPERTY_BOOL (updateAvailable, UpdateAvailable) ///< update available via server download
	PROPERTY_OBJECT (VersionNumber, newVersion, NewVersion)

	PROPERTY_VARIABLE (ExtensionTypeID, type, Type)
	PROPERTY_VARIABLE (int, useCount, UseCount)

	// Object
	bool equals (const Object& obj) const override;
	
	CLASS_INTERFACE (IExtensionDescription, Object)

protected:
	AutoPtr<Url> path;
	CCL::Attributes extraInfo;
	Vector<SubItem> subItems;

	// IExtensionDescription
	StringRef CCL_API getShortIdentifier () const override { return getShortID (); }
	void CCL_API getMetaInfo (IAttributeList& metaInfo) const override;

	// IObject
	tbool CCL_API getProperty (Variant& var, MemberID propertyId) const override;

	static ExtensionDescription* createWithMetaInfo (PackageInfo& metaInfo);
};

} // namespace Install
} // namespace CCL

#endif // _ccl_extensiondescription_h
