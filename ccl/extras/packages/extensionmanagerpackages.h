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
// Filename    : ccl/extras/packages/extensionmanagerpackages.h
// Description : Extension Packages source and handler using ExtensionManager
//
//************************************************************************************************

#ifndef _ccl_extensionmanagerpackages_h
#define _ccl_extensionmanagerpackages_h

#include "ccl/extras/packages/unifiedpackagesource.h"
#include "ccl/extras/packages/unifiedpackageaction.h"
#include "ccl/extras/packages/packagefilter.h"

#include "ccl/extras/extensions/extensiondescription.h"

#include "ccl/base/signalsource.h"

#include "ccl/public/base/iasyncoperation.h"

namespace CCL {
namespace Packages {

//************************************************************************************************
// ExtensionPackageSource
/** UnifiedPackageSource using extension data. @sa ExtensionDescription */
//************************************************************************************************

class ExtensionPackageSource: public UnifiedPackageSource<Object>
{
public:
	ExtensionPackageSource ();

	// UnifiedPackageSource
	void retrievePackages (UrlRef url, bool refresh) override;
	UnifiedPackage* createFromFile (UrlRef url) override;

protected:
	void retrievePackage (Install::ExtensionDescription* description);
	virtual UnifiedPackage* createExtensionPackage (Install::ExtensionDescription* description);
	virtual UnifiedPackage* createSubItemPackage (Install::ExtensionDescription::SubItem& subItem, const UnifiedPackage& parent);
};

//************************************************************************************************
// ExtensionManagerPackageSource
/** Package source used to retrieve UnifiedPackage representations of Extension packages. @sa ExtensionDescription */
//************************************************************************************************

class ExtensionManagerPackageSource: public ExtensionPackageSource
{
public:
	ExtensionManagerPackageSource ();
	~ExtensionManagerPackageSource ();

	static bool checkUpdates (const Container& packages, bool silent = false);
	static IAsyncOperation* checkUpdatesAsync (const Container& packages, bool silent = false, IProgressNotify* progress = nullptr);

	// ExtensionPackageSourceBase
	void retrievePackages (UrlRef url, bool refresh) override;
	void CCL_API notify (ISubject* subject, MessageRef msg) override;


private:
	SignalSink* extensionSink;
};
	
//************************************************************************************************
// ExtensionManagerPackageHandler
/** UnifiedPackageHandler used to enable, uninstall or update Extension packages. */
//************************************************************************************************

class ExtensionManagerPackageHandler: public UnifiedPackageHandler
{
public:
	// action ids
	DECLARE_STRINGID_MEMBER (kAbortUninstall)

	bool isCancelEnabled (const UnifiedPackageAction& action) const;
		
	// UnifiedPackageHandler
	bool canHandle (UnifiedPackage* package) const override;
	void getActions (Container& actions, UnifiedPackage* package = nullptr) override;
	void updateAction (UnifiedPackageAction& action) override;
	bool performAction (UnifiedPackageAction& action) override;
	bool cancelAction (UnifiedPackageAction& action) override;
	Component* createComponent (UnifiedPackage* package) override;
	StringRef getActionTitle (StringID actionId) const override;
	StringRef getStateLabel (StringID actionId) const override;
	IImage* getActionIcon (StringID actionId) const override;
	StringID getActionGroupId (StringID actionId) const override;

private:
	bool enable (Install::ExtensionDescription* extension, UnifiedPackageAction& action, bool state);
	bool uninstall (Install::ExtensionDescription* extension, UnifiedPackageAction& action);
};

//************************************************************************************************
// ExtensionFilterComponent
//************************************************************************************************

class ExtensionFilterComponent: public PackageFilterComponent
{
public:
	DECLARE_CLASS (ExtensionFilterComponent, PackageFilterComponent);

	ExtensionFilterComponent (PackageManager* manager = nullptr);

	// PackageFilterComponent
	bool matches (const UnifiedPackage& package) const override;
};

} // namespace Packages
} // namespace CCL

#endif // _ccl_extensionmanagerpackages_h
