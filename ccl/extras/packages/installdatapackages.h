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
// Filename    : ccl/extras/packages/installdatapackages.h
// Description : UnifiedPackageSource and UnifiedPackageHandler using manifest data
//
//************************************************************************************************

#ifndef _ccl_installdatapackages_h
#define _ccl_installdatapackages_h

#include "ccl/extras/packages/unifiedpackagesource.h"
#include "ccl/extras/packages/unifiedpackageaction.h"

#include "ccl/base/signalsource.h"

#include "ccl/public/text/cstring.h"

namespace CCL {

namespace Install {
class Manifest;
class Package;
class File; }

namespace Packages {

//************************************************************************************************
// ManifestPackageSource
/** UnifiedPackageSource using manifest data */
//************************************************************************************************

class ManifestPackageSource: public UnifiedPackageSource<Object>
{
public:
	DECLARE_CLASS (ManifestPackageSource, Object)

	ManifestPackageSource (CStringRef name = nullptr, int flags = 0);
	~ManifestPackageSource ();

	void setAuthorizerId (StringRef authId, const FileType& fileType = FileType ());

	// UnifiedPackageSource
	void retrievePackages (UrlRef url, bool refresh) override;

	// Object
	void CCL_API notify (ISubject* subject, CCL::MessageRef msg) override;

protected:
	struct AuthorizerConfiguration
	{
		String authId;
		FileType fileType;

		AuthorizerConfiguration (StringRef authId = nullptr, const FileType& fileType = nullptr)
		: authId (authId),
		  fileType (fileType)
		{}
	};

	struct InstallData
	{
		AutoPtr<Install::Manifest> manifest;
		int origin;
	};
	Vector<InstallData> installData;

	Vector<AuthorizerConfiguration> authorizerConfigurations;
	MutableCString name;
	Vector<String> packageIds;

	SignalSink authSignalSink;

	bool dataValid;

	void scanCategory (const InstallData& data, const Install::Package* category, StringRef parentCategory = nullptr);
	void scanTopLevelFiles (const InstallData& data);
	void retrievePackage (const InstallData& data, StringRef packageId, StringRef fileId);
	void fixupParents (const InstallData& data, StringRef packageID = nullptr, StringRef fileId = nullptr);
	virtual UnifiedPackage* createCategoryPackage (const InstallData& data, const Install::Package* category, StringRef id = nullptr);
	virtual UnifiedPackage* createFilePackage (const InstallData& data, const Install::File* file);
	virtual void initializeData (bool silent = false);
	virtual void resetData ();
	bool isSingleProduct (StringRef fileId, StringRef productId) const;
	bool isExtensionFile (const Install::File& file) const;
	bool isInstalled (const Install::File& file) const;
	String getLicenseData (const Install::File& file) const;
};

//************************************************************************************************
// ManifestPackageHandler
/** UnifiedPackageHandler used to claim product keys for manifest files. */
//************************************************************************************************

class ManifestPackageHandler: public UnifiedPackageHandler
{
public:
	// action ids
	DECLARE_STRINGID_MEMBER (kClaimLicense)

	static bool claimLicenseData (UnifiedPackage& package);

	// UnifiedPackageHandler
	bool canHandle (UnifiedPackage* package) const override;
	void getActions (Container& actions, UnifiedPackage* package = nullptr) override;
	void updateAction (UnifiedPackageAction& action) override;
	bool performAction (UnifiedPackageAction& action) override;
	bool cancelAction (UnifiedPackageAction& action) override;
	Component* createComponent (UnifiedPackage* package) override;
	StringRef getActionTitle (StringID actionId) const override;
	void composeTitle (String& title, StringID id, int itemCount, StringRef details) const override;
};
} // namespace Packages
} // namespace CCL

#endif // _ccl_installdatapackages_h
