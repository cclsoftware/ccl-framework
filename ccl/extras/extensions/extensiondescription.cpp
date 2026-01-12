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
// Filename    : ccl/extras/extensions/extensiondescription.cpp
// Description : Extension Description
//
//************************************************************************************************

#include "ccl/extras/extensions/extensiondescription.h"

#include "ccl/app/utilities/imagefile.h"
#include "ccl/app/utilities/fileicons.h"

#include "ccl/base/storage/packageinfo.h"
#include "ccl/base/storage/propertyfile.h"

#include "ccl/public/text/translation.h"
#include "ccl/public/system/ifileitem.h"

#include "ccl/public/cclversion.h"

using namespace CCL;
using namespace Install;

//////////////////////////////////////////////////////////////////////////////////////////////////
// Strings
//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_XSTRINGS ("FileType")
	XSTRING (InstallFile, "Installation File")
END_XSTRINGS

BEGIN_XSTRINGS ("Extensions")
	XSTRING (Extension, "Extension")
	XSTRING (SignatureCheck, "Signature Verification")
	XSTRING (InvalidSignature, "This file does not have a valid digital signature.")
	XSTRING (InvalidPlatform, "The installation file is not compatible with this platform.")
	XSTRING (InvalidApplication, "The installation file is not compatible with this application.")
	XSTRING (InvalidInstallFile, "Invalid Installation File!")
	XSTRING (InstallationFailed, "Installation failed!")
	XSTRING (UpdatesAvailable, "There are updates available for your Extensions.")
	XSTRING (NoUpdatesFound, "No updates found for your Extensions.")
	XSTRING (AlreadyInstalled, "%(1) is already installed!")
	XSTRING (DeinstallationFailed, "Failed to uninstall %(1)!")
	XSTRING (DirectoryLocked, "Target directory is locked by %(1).")
	XSTRING (UserExtension, "User Extension")
	XSTRING (ProgramExtension, "Program Extension")
	XSTRING (SharedExtension, "Shared Extension")
	XSTRING (DeveloperExtension, "Developer Extension")
END_XSTRINGS

static const String kMacPlatform (CCL_PLATFORM_ID_MAC);
static const String kWinPlatform (CCL_PLATFORM_ID_WIN);
static const String kIOSPlatform (CCL_PLATFORM_ID_IOS);
static const String kAndroidPlatform (CCL_PLATFORM_ID_ANDROID);
static const String kLinuxPlatform (CCL_PLATFORM_ID_LINUX);
static const String kPlatformPrefix ("platform-"); // prefix for special platforms

static const String kPlatformPlaceholder ("$platform");
static const String kRevisionPlaceholder ("$revision");

//************************************************************************************************
// ExtensionStrings
//************************************************************************************************

StringRef ExtensionStrings::Extension () { return XSTR (Extension); }
StringRef ExtensionStrings::InvalidInstallFile () { return XSTR (InvalidInstallFile); }
StringRef ExtensionStrings::SignatureCheck () { return XSTR (SignatureCheck); }
StringRef ExtensionStrings::InvalidSignature () { return XSTR (InvalidSignature); }
StringRef ExtensionStrings::InvalidPlatform () { return XSTR (InvalidPlatform); }
StringRef ExtensionStrings::InvalidApplication () { return XSTR (InvalidApplication); }
StringRef ExtensionStrings::InstallationFailed () { return XSTR (InstallationFailed); }
StringRef ExtensionStrings::UpdatesAvailable () { return XSTR (UpdatesAvailable); }
StringRef ExtensionStrings::NoUpdatesFound () { return XSTR (NoUpdatesFound); }
String ExtensionStrings::AlreadyInstalled (StringRef title) { return String ().appendFormat (XSTR (AlreadyInstalled), title); }
String ExtensionStrings::DeinstallationFailed (StringRef title) { return String ().appendFormat (XSTR (DeinstallationFailed), title); }
String ExtensionStrings::DirectoryLocked (StringRef lockingAppName) { return String ().appendFormat (XSTR (DirectoryLocked), lockingAppName); }
StringRef ExtensionStrings::ExtensionType (ExtensionTypeID id)
{
	switch(id)
	{
	case ExtensionType::kUser : return XSTR (UserExtension);
	case ExtensionType::kProgram : return XSTR (ProgramExtension);
	case ExtensionType::kShared : return XSTR (SharedExtension);
	case ExtensionType::kDeveloper : return XSTR (DeveloperExtension);
	}
	return String::kEmpty;
}

//************************************************************************************************
// ExtensionDescription
//************************************************************************************************

const FileType& ExtensionDescription::getFileType ()
{
	static FileType fileType (nullptr, "install", CCL_MIME_TYPE "-install-package");
	return FileTypes::init (fileType, XSTR (InstallFile));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringRef ExtensionDescription::getPlatformName ()
{
	#if CCL_PLATFORM_MAC
	return kMacPlatform;
	#elif CCL_PLATFORM_WINDOWS
	return kWinPlatform;
	#elif CCL_PLATFORM_IOS
	return kIOSPlatform;
	#elif CCL_PLATFORM_ANDROID
	return kAndroidPlatform;
	#elif CCL_PLATFORM_LINUX
	return kLinuxPlatform;
	#else
		#error Set platform here!
	#endif
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ExtensionDescription::replacePlatform (String& id)
{
	id.replace (kPlatformPlaceholder, getPlatformName ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

String ExtensionDescription::extractPlatform (StringRef id)
{
	// identifier can end with ".mac", ".win", or ".platform-xxx"
	String platform;
	int lastIndex = id.lastIndex (".");
	if(lastIndex != -1)
	{
		String lastPart = id.subString (lastIndex + 1);
		if(lastPart == kMacPlatform)
			platform = kMacPlatform;
		else if(lastPart == kWinPlatform)
			platform = kWinPlatform;
		else if(lastPart == kLinuxPlatform)
			platform = kLinuxPlatform;
		else if(lastPart.startsWith (kPlatformPrefix))
			platform = lastPart;
	}
	return platform;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ExtensionDescription* ExtensionDescription::createFromPackage (UrlRef path)
{
	AutoPtr<PackageInfo> metaInfo = NEW PackageInfo;
	
	AutoPtr<ImageFile> imageFile = NEW ImageFile;
	metaInfo->addResource (Meta::kPackageIcon, String ("extension.png"), imageFile); // obsolete!
	// PLEASE NOTE: @2x naming convention doesn't work here, because package resources are loaded via stream!

	AutoPtr<ImageFile> imageFile2 = NEW ImageFile (ImageFile::kIconSet);
	metaInfo->addResource (Meta::kPackageIconSet, String (Meta::kPackageIconSetFileName), imageFile2);

	AutoPtr<Manifest> manifest = NEW Manifest;
	metaInfo->addResource (Manifest::kResourceID, Manifest::kFileName, manifest);

	AutoPtr<Java::PropertyFile> revFile = NEW Java::PropertyFile;
	metaInfo->addResource ("Package:Revision", String ("revision.properties"), revFile);

	if(!metaInfo->loadFromPackage (path))
		return nullptr;

	AutoPtr<ExtensionDescription> e = createWithMetaInfo (*metaInfo);
	if(!e)
		return nullptr;

	e->setPath (path);

	// check for separate revision information
	if(revFile->getProperties ().countEntries () > 0)
	{
		int64 revision = 0;
		if(revFile->getProperties ().getValueAt (0).getIntValue (revision))
		{
			VersionNumber version (e->getVersion ());
			version.build = (int)revision; // revision = build number!!!
			e->setVersion (version);
		}
	}
	
	// check for icon
	if(imageFile2->getImage ())
		e->setIcon (imageFile2->getImage ());
	else if(imageFile->getImage ())
		e->setIcon (imageFile->getImage ());

	if(e->getIcon () == nullptr)
	{
		AutoPtr<IImage> fileIcon = FileIcons::instance ().createIcon (getFileType ());
		e->setIcon (fileIcon);
	}

	e->setManifest (manifest);
	
	return e.detach ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ExtensionDescription* ExtensionDescription::createFromDescriptor (IFileDescriptor& descriptor)
{
	PackageInfo metaInfo;
	descriptor.getMetaInfo (metaInfo);

	AutoPtr<ExtensionDescription> e = createWithMetaInfo (metaInfo);
	if(!e)
		return nullptr;

	// try to load embedded manifest
	String embeddedManifest = metaInfo.getString (Manifest::kResourceID);
	if(!embeddedManifest.isEmpty ())
	{
		AutoPtr<Manifest> manifest = NEW Manifest;
		if(manifest->loadFromBase64 (embeddedManifest))
			e->setManifest (manifest);
	}

	// not used
	//AutoPtr<IImage> fileIcon = FileIcons::instance ().createIcon (getFileType ());
	//e->setIcon (fileIcon);

	return e.detach ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ExtensionDescription* ExtensionDescription::createWithMetaInfo (PackageInfo& metaInfo)
{
	String id = metaInfo.getPackageID ();
	if(id.isEmpty ())
		return nullptr;

	replacePlatform (id); // needed for debug builds
	String platform = extractPlatform (id);

	String title = metaInfo.getStringWithAlternative (Meta::kPackageLocalizedName, Meta::kPackageName);
	if(title.isEmpty ())
		return nullptr;

	String description = metaInfo.getStringWithAlternative (Meta::kPackageLocalizedDescription, Meta::kPackageDescription);
	
	String versionString (metaInfo.getString (Meta::kPackageVersion));
	versionString.replace (kRevisionPlaceholder, BUILD_REVISION_STRING); // needed for debug builds
	VersionNumber version;
	version.scan (versionString);

	String shortId = metaInfo.getString (Meta::kPackageShortID);
	if(shortId.isEmpty ())
		shortId = id;

	String vendor = metaInfo.getString (Meta::kPackageVendor);
	String copyright = metaInfo.getString (Meta::kPackageCopyright);
	String website = metaInfo.getString (Meta::kPackageWebsite);

	AutoPtr<ExtensionDescription> e = NEW ExtensionDescription;
	e->setID (id);
	e->setShortID (shortId);
	e->setTitle (title);
	e->setDescription (description);
	e->setVendor (vendor);
	e->setVersion (version);
	e->setPlatform (platform);
	e->setCopyright (copyright);
	e->setWebsite (website);

	class ExtraInfoFilter: public Unknown,
						   public IAttributeFilter
	{
	public:
		tbool CCL_API matches (StringID id) const override
		{
			return !id.startsWith (Meta::kPackagePrefix); // filter all basic attributes
		}

		CLASS_INTERFACE (IAttributeFilter, Unknown)
	} filter;

	e->extraInfo.addFrom (metaInfo, &filter);

	return e.detach ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_CLASS_HIDDEN (ExtensionDescription, StorableObject)

//////////////////////////////////////////////////////////////////////////////////////////////////

ExtensionDescription::ExtensionDescription (UrlRef path, StringRef id)
: path (NEW Url (path)),
  id (id),
  enabled (false),
  started (false),
  compatibilityResult (File::kAppOK),
  uninstallPending (false),
  updatePending (false),
  updateAvailable (false),
  type (ExtensionType::kUser),
  useCount (0)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ExtensionDescription::setPath (UrlRef path)
{
	ASSERT (this->path)
	this->path->assign (path);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

UrlRef CCL_API ExtensionDescription::getPath () const
{
	ASSERT (path)
	return *path;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

String CCL_API ExtensionDescription::getPlatformIndependentIdentifier () const
{
	if(!platform.isEmpty ())
	{
		String dotPlatform;
		dotPlatform << "." << platform;
		if(id.endsWith (dotPlatform))
			return id.subString (0, id.length ()-dotPlatform.length ());		
	}
	return id;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const Attributes& ExtensionDescription::getExtraInfo () const
{
	return extraInfo;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ExtensionDescription::hasSubItem (StringRef id) const
{
	VectorForEachFast (subItems, SubItem, item)
		if(item.getID () == id)
			return true;
	EndFor
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const Vector<ExtensionDescription::SubItem>& ExtensionDescription::getSubItems () const
{
	return subItems;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ExtensionDescription::addSubItem (const SubItem& item)
{
	subItems.add (item);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ExtensionDescription::isSilentCheckEnabled () const
{
	const CString kSilentCheckID ("Extension:SilentCompatibilityCheck");
	return extraInfo.getBool (kSilentCheckID);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ExtensionDescription::isUsingAppProductID () const
{
	const CString kUseAppProductID ("Extension:UseAppProductID");
	return extraInfo.getBool (kUseAppProductID);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ExtensionDescription::isUsingSharedLocation () const
{
	if(auto entry = getManifestEntry ())
		return entry->isUsingSharedLocation ();
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ExtensionDescription::isHidden () const
{
	const CString kHiddenID ("Extension:Hidden");
	return extraInfo.getBool (kHiddenID);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ExtensionDescription::canPlugInRescanInsteadRestart () const
{
	const CString kPlugInExtensionID ("Extension:PlugInRescanInsteadRestart");
	return extraInfo.getBool (kPlugInExtensionID);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

File* ExtensionDescription::getManifestEntry () const
{
	if(!manifest)
		return nullptr;

	File* file = manifest->findFile (getID ());
	if(file == nullptr) // use first to allow sharing manifest between extensions
		file = manifest->getFirstFile ();

	return file;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API ExtensionDescription::getMetaInfo (IAttributeList& metaInfo) const
{
	metaInfo.setAttribute (Meta::kPackageID, getID ());
	metaInfo.setAttribute (Meta::kPackageShortID, getShortID ());
	metaInfo.setAttribute (Meta::kPackageName, getTitle ());
	metaInfo.setAttribute (Meta::kPackageDescription, getDescription ());
	metaInfo.setAttribute (Meta::kPackageVendor, getVendor ());
	metaInfo.setAttribute (Meta::kPackageVersion, getVersion ().print ());
	metaInfo.setAttribute (Meta::kPackageCopyright, getCopyright ());
	metaInfo.setAttribute (Meta::kPackageWebsite, getWebsite ());
	metaInfo.addFrom (extraInfo);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ExtensionDescription::equals (const Object& obj) const
{
	const ExtensionDescription* other = ccl_cast<ExtensionDescription> (&obj);
	return other ? other->getID () == id : SuperClass::equals (obj);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ExtensionDescription::getProperty (Variant& var, MemberID propertyId) const
{
	if(propertyId == "path")
	{
		ASSERT (path)
		var.takeShared (ccl_as_unknown (path));
		return true;
	}
	else if(propertyId == "id")
	{
		var = getID ();
		return true;
	}
	else if(propertyId == "platformIndependentId")
	{
		String id (getPlatformIndependentIdentifier ());
		var = id;
		var.share ();
		return true;
	}
	return SuperClass::getProperty (var, propertyId);
}
