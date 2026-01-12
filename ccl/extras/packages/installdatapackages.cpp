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
// Filename    : ccl/extras/packages/installdatapackages.cpp
// Description : UnifiedPackageSource and UnifiedPackageHandler using manifest data
//
//************************************************************************************************

#define SET_TAGS_FOR_CATEGORIES 1

#include "ccl/extras/packages/installdatapackages.h"
#include "ccl/extras/packages/unifiedpackageinstaller.h"
#include "ccl/extras/packages/packagehandlerregistry.h"

#include "ccl/extras/extensions/extensiondescription.h"
#include "ccl/public/extras/icontentinstaller.h"

#include "ccl/app/utilities/imagefile.h"

#include "ccl/public/text/translation.h"
#include "ccl/public/gui/framework/viewbox.h"
#include "ccl/public/gui/framework/itheme.h"
#include "ccl/public/security/iauthorizationmanager.h"
#include "ccl/public/system/ifileutilities.h"
#include "ccl/public/system/inativefilesystem.h"
#include "ccl/public/systemservices.h"

using namespace CCL;
using namespace Packages;
using namespace Install;

//////////////////////////////////////////////////////////////////////////////////////////////////
// Strings
//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_XSTRINGS ("PackageActions")
	// action titles
	XSTRING (ClaimLicense, "Claim Product Key")

	// composed titles
	XSTRING (ClaimLicenses, "Claim %(1) Product Keys")
END_XSTRINGS

//////////////////////////////////////////////////////////////////////////////////////////////////
// Initialization
//////////////////////////////////////////////////////////////////////////////////////////////////

static ManifestPackageHandler theManifestPackageHandler;

CCL_KERNEL_INIT_LEVEL (RegisterManifestPackageHandler, kSetupLevel)
{
	PackageHandlerRegistry::instance ().registerHandler (&theManifestPackageHandler);
	return true;
}

CCL_KERNEL_TERM_LEVEL (UnRegisterManifestPackageHandler, kSetupLevel)
{
	PackageHandlerRegistry::instance ().unregisterHandler (&theManifestPackageHandler);
}

//************************************************************************************************
// ManifestPackageHandler
//************************************************************************************************

DEFINE_STRINGID_MEMBER_ (ManifestPackageHandler, kClaimLicense, "claimLicense")

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ManifestPackageHandler::canHandle (UnifiedPackage* package) const
{
	if(package == nullptr)
		return false;

	Manifest* manifest = nullptr;
	for(int i = 0; manifest = package->getData<Manifest> (i); i++)
	{
		if(File* file = manifest->findFile (package->getId ()))
		{
			if(file->getLicenseID ().isEmpty () == false)
				return true;
		}
	}

	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ManifestPackageHandler::getActions (Container& actions, UnifiedPackage* package)
{
	if(canHandle (package))
		actions.add (createAction (package, kClaimLicense));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ManifestPackageHandler::updateAction (UnifiedPackageAction& action)
{
	action.setState (UnifiedPackageAction::kInvalid);

	UnifiedPackage* package = action.getPackage ();
	if(!canHandle (package))
		return;

	if(action.getId () == kClaimLicense && package->getLicenseData ().isEmpty ())
	{
		Manifest* manifest = nullptr;
		for(int i = 0; manifest = package->getData<Manifest> (i); i++)
		{
			if(File* file = manifest->findFile (package->getId ()))
			{
				if(file->getLicenseID ().isEmpty () == false)
					action.setState (UnifiedPackageAction::kEnabled);
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ManifestPackageHandler::performAction (UnifiedPackageAction& action)
{
	if(action.getId () == kClaimLicense)
	{
		if(action.getPackage () == nullptr)
			return false;
		bool succeeded = claimLicenseData (*action.getPackage ());
		action.complete (succeeded);
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ManifestPackageHandler::cancelAction (UnifiedPackageAction& action)
{
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Component* ManifestPackageHandler::createComponent (UnifiedPackage* package)
{
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ManifestPackageHandler::claimLicenseData (UnifiedPackage& package) 
{
	IContentServer* server = UnifiedPackageInstaller::instance ().getInstallEngine ().getContentServer ();
	if(server == nullptr)
		return false;

	Manifest* manifest = nullptr;
	for(int i = 0; manifest = package.getData<Manifest> (i); i++)
	{
		if(File* file = manifest->findFile (package.getId ()))
		{
			if(file->getLicenseID ().isEmpty ())
				continue;

			String licenseData = server->requestLicenseData (file->getLicenseID ());
			if(licenseData.isEmpty ())
				continue;

			package.setLicenseData (licenseData);
			return true;
		}
	}

	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringRef ManifestPackageHandler::getActionTitle (StringID id) const
{
	if(id == kClaimLicense) return XSTR (ClaimLicense);
	return UnifiedPackageHandler::getActionTitle (id);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ManifestPackageHandler::composeTitle (String& title, StringID id, int itemCount, StringRef details) const
{
	if(id == kClaimLicense)
		title = String ().appendFormat (XSTR (ClaimLicenses), itemCount);
	else
		UnifiedPackageHandler::composeTitle (title, id, itemCount, details);
}

//************************************************************************************************
// ManifestPackageSource
//************************************************************************************************

DEFINE_CLASS_HIDDEN (ManifestPackageSource, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

ManifestPackageSource::ManifestPackageSource (CStringRef name, int sourceFlags)
: dataValid (false),
  name (name),
  authSignalSink (Signals::kAuthorization)
{
	authSignalSink.setObserver (this);
	flags = sourceFlags;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ManifestPackageSource::~ManifestPackageSource ()
{
	authSignalSink.enable (false);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ManifestPackageSource::setAuthorizerId (StringRef authId, const FileType& fileType)
{
	authorizerConfigurations.add ({ authId, fileType });
	authSignalSink.enable (true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ManifestPackageSource::retrievePackages (UrlRef url, bool refresh)
{
	if(refresh)
		resetData ();

	if(dataValid == false)
	{
		bool silent = !url.isRootPath ();
		initializeData (silent);
	}

	packageIds.removeAll ();

	for(const InstallData& data : installData)
	{
		if(data.manifest == nullptr)
			continue;

		if(url.isRootPath ())
		{
			for(const Package* entry : iterate_as<const Package> (data.manifest->getPackages ()))
				scanCategory (data, entry);
			scanTopLevelFiles (data);
			fixupParents (data);
		}
		else
		{
			uchar delim = Url::strPathChar.firstChar ();
			AutoPtr<IStringTokenizer> tokenizer = url.getPath ().tokenize (Url::strPathChar);

			String packageId;
			String fileId;

			// ignore top level categories, only use inner categories
			while(tokenizer && tokenizer->done () == false)
			{
				packageId = fileId;
				fileId = tokenizer->nextToken (delim);
			}

			retrievePackage (data, packageId, fileId);
		}
	}

	for(StringRef id : packageIds)
	{
		AutoPtr<UnifiedPackage> package = createPackage (id);
		announcePackage (package);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ManifestPackageSource::scanCategory (const InstallData& data, const Package* category, StringRef parentCategory)
{
	ObjectArray files;
	data.manifest->getFilesForPackage (files, category->getID ());

	String categoryId (category->getID ());
	for(const File* file : iterate_as<const File> (files))
	{
		if(categoryId == file->getID ())
			categoryId.append (".package");
	}

	AutoPtr<UnifiedPackage> package = createCategoryPackage (data, category, categoryId);

#if SET_TAGS_FOR_CATEGORIES
	if(parentCategory.isEmpty () == false)
		package->addTag (parentCategory);
	if(files.count () > 1)
		package->addTag (category->getTitle ());
#endif

	for(const File* file : iterate_as<const File> (files))
	{
		AutoPtr<UnifiedPackage> filePackage = createFilePackage (data, file);
		if(package)
		{
			if(package->isCritical () == false && (file->isParentAmbiguous () || isSingleProduct (file->getID (), categoryId) || (files.count () == 1 && isExtensionFile (*file))))
				filePackage->addChild (package);
			else
				package->addChild (filePackage);

			for(StringRef tag : package->getTags ())
				filePackage->addTag (tag);
		}

		if(!packageIds.contains (filePackage->getId ()))
			packageIds.add (filePackage->getId ());
	}

	if(!category->getSavedChildID ().isEmpty ())
	{
		AutoPtr<UnifiedPackage> filePackage = createPackage (category->getSavedChildID ());
		ASSERT (filePackage->isTopLevel () && filePackage->getId () != package->getId ())
		if(filePackage->getId () != package->getId ())
		{
			if(!filePackage->isTopLevel ())
			{
				// We previously assumed that filePackage is a child of a product package
				// Now that we know that filePackage is a parent of multiple products, fix the package hierarchy
				for(UnifiedPackage* package : iterate_as<UnifiedPackage> (packageCache))
				{
					if(package->getChildren ().contains (filePackage))
					{
						package->removeChild (filePackage);
						filePackage->addChild (package);
						continue;
					}
				}
				filePackage->isTopLevel (true);
			}

			filePackage->addChild (package);
			if(!packageIds.contains (filePackage->getId ()))
				packageIds.add (filePackage->getId ());
		}
	}

	for(const Package* childCategory : iterate_as<Package> (category->getChildren ()))
		scanCategory (data, childCategory, category->getTitle ());

	if(package && !packageIds.contains (package->getId ()))
		packageIds.add (package->getId ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ManifestPackageSource::scanTopLevelFiles (const InstallData& data)
{
	for(const File* file : iterate_as<const File> (data.manifest->getFiles ()))
	{
		if(file->getParentID ().isEmpty () == false || file->getSavedParentID ().isEmpty () == false || packageIds.contains (file->getID ()))
			continue;

		AutoPtr<UnifiedPackage> filePackage = createFilePackage (data, file);

		if(!packageIds.contains (filePackage->getId ()))
			packageIds.add (filePackage->getId ());
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ManifestPackageSource::retrievePackage (const InstallData& data, StringRef packageId, StringRef fileId)
{
	AutoPtr<UnifiedPackage> package = nullptr;
	for(const Package* entry : iterate_as<const Package> (data.manifest->getPackages ()))
		if(entry->getID () == packageId)
		{
			package = createCategoryPackage (data, entry);
			break;
		}
	if(File* file = data.manifest->findFile (fileId))
	{
		AutoPtr<UnifiedPackage> filePackage = createFilePackage (data, file);
		if(!packageIds.contains (filePackage->getId ()))
			packageIds.add (filePackage->getId ());
		if(package)
			package->addChild (filePackage);
	}
	if(package && !packageIds.contains (package->getId ()))
		packageIds.add (package->getId ());
	fixupParents (data, packageId, fileId);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ManifestPackageSource::fixupParents (const InstallData& data, StringRef packageId, StringRef fileId)
{
	// Fixup recursive parents
	for(const File* file : iterate_as<const File> (data.manifest->getFiles ()))
	{
		AutoPtr<UnifiedPackage> filePackage = createPackage (file->getID ());
		if((file->getID () == fileId || fileId.isEmpty ())
			&& file->getSavedParentID ().isEmpty () == false
			&& (file->getSavedParentID () == packageId || packageId.isEmpty ()))
		{
			String productId (file->getSavedParentID ());
			if(productId == file->getID ())
				productId.append (".item");
			AutoPtr<UnifiedPackage> productPackage = createPackage (productId);

			filePackage->setData<Install::Manifest> (data.manifest, name);
			filePackage->setOrigin (filePackage->getOrigin () | data.origin);
			FileType type;
			if(file->getFileType (type))
				filePackage->setFileType (type);
			filePackage->setTitle (file->getTitle ());
			filePackage->setSize (static_cast<int64> (file->getFileSize ()));

			productPackage->setData<Install::Manifest> (data.manifest, name);
			productPackage->setOrigin (productPackage->getOrigin () | data.origin);
			productPackage->isProduct (true);

			if(productPackage->isCritical () == false && (file->isParentAmbiguous () || isSingleProduct (file->getID (), productId) || (filePackage->getChildren ().isEmpty () == false && isExtensionFile (*file))))
				filePackage->addChild (productPackage);
			else
				productPackage->addChild (filePackage);

			if(!packageIds.contains (productPackage->getId ()))
				packageIds.add (productPackage->getId ());

			if(!packageIds.contains (filePackage->getId ()))
				packageIds.add (filePackage->getId ());
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

UnifiedPackage* ManifestPackageSource::createCategoryPackage (const InstallData& data, const Package* category, StringRef id)
{
	UnifiedPackage* package = createPackage (id.isEmpty () ? category->getID () : id);

	package->setTitle (category->getTitle ());
	package->setData<Install::Manifest> (data.manifest, name);
	package->setDescription (category->getDescription ());
	// if(!category->getType ().isEmpty ())
	//	 package->setType (MutableCString (category->getType ()));
	package->setOrigin (package->getOrigin () | data.origin);

	if(category->getIconName ().isEmpty () == false)
		if(auto theme = ViewBox::getModuleTheme ())
			package->setIcon (theme->getImage (MutableCString (category->getIconName ())));

	return package;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

UnifiedPackage* ManifestPackageSource::createFilePackage (const InstallData& data, const File* file)
{
	UnifiedPackage* filePackage = createPackage (file->getID ());
	
	FileType type;
	if(file->getFileType (type))
	{
		filePackage->setFileType (type);
		for(const AuthorizerConfiguration& config : authorizerConfigurations)
		{
			if(config.fileType == type || config.fileType.isValid () == false)
			{
				filePackage->setAuthorizerId (config.authId);
				break;
			}
		}
	}
	bool installed = isInstalled (*file);
	filePackage->isLocalPackage (installed);
	filePackage->setSize (static_cast<int64> (installed ? 0 : file->getFileSize ()));
	filePackage->setTitle (file->getTitle ());
	filePackage->setData<Install::Manifest> (data.manifest, name);
	filePackage->setDescription (file->getDescription ());
	filePackage->setOrigin (filePackage->getOrigin () | data.origin);
	filePackage->isMinimum (file->isMinimum ());
	filePackage->isRecommended (file->isRecommended ());
	filePackage->setLicenseData (getLicenseData (*file));

	if(file->getIconName ().isEmpty () == false)
	{
		if(auto theme = ViewBox::getModuleTheme ())
			filePackage->setIcon (theme->getImage (MutableCString (file->getIconName ())));
		if(filePackage->getIcon () == nullptr)
		{
			AutoPtr<IImage> icon = ImageFile::loadImage (Url (file->getIconName ()));
			filePackage->setIcon (icon);
		}
	}

	ForEach (file->getDependencies (), DependentItem, dependentItem)
		filePackage->addDependency (dependentItem->getID ());
	EndFor

	return filePackage;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ManifestPackageSource::initializeData (bool silent)
{
	dataValid = true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ManifestPackageSource::resetData ()
{
	installData.removeAll ();
	dataValid = false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ManifestPackageSource::isSingleProduct (StringRef fileId, StringRef productId) const
{
	return productId == fileId
		|| productId == (String (fileId).append (".item"))
		|| fileId == (String (productId).append (".").append (ExtensionDescription::extractPlatform (fileId)));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ManifestPackageSource::isExtensionFile (const Install::File& file) const
{
	FileType fileType;
	return file.getFileType (fileType) && fileType == ExtensionDescription::getFileType ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ManifestPackageSource::isInstalled (const File& file) const
{
	Url path;
	IterForEachUnknown (System::GetFileTypeRegistry ().newHandlerIterator (), unk)
		if(UnknownPtr<IFileInstallHandler> fileHandler = unk)
		{
			if(fileHandler->getFileLocation (path, const_cast<File&> (file)))
				return System::GetFileSystem ().fileExists (path);
		}
	EndFor
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

String ManifestPackageSource::getLicenseData (const File& file) const
{
	if(file.getLicenseID ().isEmpty ())
		return String::kEmpty;

	IContentServer* server = UnifiedPackageInstaller::instance ().getInstallEngine ().getContentServer ();
	if(server == nullptr)
		return String::kEmpty;

	return server->getLicenseData (file.getLicenseID ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API ManifestPackageSource::notify (ISubject* subject, CCL::MessageRef msg)
{
	if(msg == Signals::kAuthorizationPolicyChanged)
	{
		requestUpdate (IUnifiedPackageSink::kPackageChanged);
	}
	else
		SuperClass::notify (subject, msg);
}
