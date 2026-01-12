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
// Filename    : ccl/extras/packages/extensionmanagerpackages.cpp
// Description : Extension Packages
//
//************************************************************************************************

#define SHOW_DEVELOPMENT_PACKAGES (1 && DEBUG)

#include "ccl/extras/packages/extensionmanagerpackages.h"
#include "ccl/extras/packages/packagehandlerregistry.h"
#include "ccl/extras/packages/unifiedpackageinstaller.h"

#include "ccl/extras/extensions/appupdater.h"
#include "ccl/extras/extensions/extensionmanager.h"
#include "ccl/extras/extensions/extensionmanagement.h"

#include "ccl/app/utilities/pluginclass.h"
#include "ccl/app/application.h"

#include "ccl/base/storage/file.h"

#include "ccl/public/base/iprogress.h"
#include "ccl/public/text/translation.h"
#include "ccl/public/gui/framework/iprogressdialog.h"
#include "ccl/public/system/isysteminfo.h"
#include "ccl/public/systemservices.h"
#include "ccl/public/plugservices.h"

using namespace CCL;
using namespace Packages;
using namespace Install;

//////////////////////////////////////////////////////////////////////////////////////////////////
// Strings
//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_XSTRINGS ("PackageActions")
	XSTRING (AbortUninstall, "Abort Uninstall")
END_XSTRINGS

//////////////////////////////////////////////////////////////////////////////////////////////////
// Initialization
//////////////////////////////////////////////////////////////////////////////////////////////////

static ExtensionManagerPackageHandler theExtensionPackageHandler;

CCL_KERNEL_INIT_LEVEL (RegisterExtensionPackageHandler, kSetupLevel)
{
	PackageHandlerRegistry::instance ().registerHandler (&theExtensionPackageHandler);
	return true;
}

CCL_KERNEL_TERM_LEVEL (UnRegisterExtensionPackageHandler, kSetupLevel)
{
	PackageHandlerRegistry::instance ().unregisterHandler (&theExtensionPackageHandler);
}

//************************************************************************************************
// ExtensionManagerPackageHandler
//************************************************************************************************

DEFINE_STRINGID_MEMBER_ (ExtensionManagerPackageHandler, kAbortUninstall, "abortUninstall")

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ExtensionManagerPackageHandler::canHandle (UnifiedPackage* package) const
{
	if(ExtensionManager::instance ().isStarted () == false)
		return false;
	return package && package->getData<ExtensionDescription> ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ExtensionManagerPackageHandler::getActions (Container& actions, UnifiedPackage* package)
{
	if(canHandle (package))
	{
		actions.add (createAction (package, kUpdate));
		actions.add (createAction (package, kDisable));
		actions.add (createAction (package, kEnable));
		actions.add (createAction (package, kAbortUninstall));
		actions.add (createAction (package, kUninstall));
		actions.add (createAction (package, kRestart));
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ExtensionManagerPackageHandler::updateAction (UnifiedPackageAction& action)
{
	action.setState (UnifiedPackageAction::kInvalid);
	action.isRequired (false);

	UnifiedPackage* package = action.getPackage ();
	if(!canHandle (package))
		return;

	ExtensionDescription* e = package->getData<ExtensionDescription> ();
	if(action.getId () == kUpdate)
	{
		#if DEBUG
		bool applicationNeedsUpdate = false;
		#else
		bool applicationNeedsUpdate = ApplicationUpdater::instance ().isUpdateAvailable ();
		#endif

		if(applicationNeedsUpdate == false && e->isUpdatePending () == false && e->isUpdateAvailable ())
		{
			if(UnifiedPackageInstaller::instance ().canHandle (package))
			{
				// check for active transaction
				action.setId (kInstall);
				package->isLocalInstallationAllowed (true);
				UnifiedPackageInstaller::instance ().updateAction (action);
				package->isLocalInstallationAllowed (false);
				action.setId (kUpdate);
			}
			else
			{
				// UnifiedPackagInstaller cannot handle this package. Try to update using the ExtensionManager.
				action.setState (UnifiedPackageAction::kEnabled);
			}
		}
	}
	else if(action.getId () == kDisable && e->isEnabled ())
		action.setState (UnifiedPackageAction::kEnabled);
	else if(action.getId () == kEnable && !e->isEnabled ())
	{
		action.setState (UnifiedPackageAction::kEnabled);
		action.isRequired (true);
	}
	else if(action.getId () == kRestart)
	{
		bool restartRequired = false;
		if(e->isUpdatePending ())
			restartRequired = true;
		else if(e->isUninstallPending ())
			restartRequired = true;
		else if(e->isCompatible () && (e->isEnabled () != e->isStarted ()))
			restartRequired = true;
		if(restartRequired)
		{
			action.setState (UnifiedPackageAction::kDisabled);
			action.isRequired (true);
		}
	}
	if(ExtensionManager::instance ().isUserInstalled (*e))
	{
		if(action.getId () == kAbortUninstall && e->isUninstallPending ())
			action.setState (UnifiedPackageAction::kEnabled);
		else if(action.getId () == kUninstall && !e->isUninstallPending ())
		{
			action.setState (UnifiedPackageAction::kEnabled);
			action.needsConfirmation (true);
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ExtensionManagerPackageHandler::performAction (UnifiedPackageAction& action)
{
	UnifiedPackage* package = action.getPackage ();
	if(package == nullptr)
		return false;

	StringID actionId = action.getId ();

	ExtensionDescription* e = package->getData<ExtensionDescription> ();
	if(e == nullptr)
		return false;

	if(actionId == kUpdate)
	{
		if(UnifiedPackageInstaller::instance ().canHandle (package))
		{
			action.setId (kInstall);
			bool succeeded = UnifiedPackageInstaller::instance ().performAction (action);
			action.setId (kUpdate);
			return succeeded;
		}
		else
		{
			// UnifiedPackagInstaller cannot handle this package. Try to update using the ExtensionManager.

			bool succeeded = false;

			AutoPtr<IProgressNotify> progress = ccl_new<IProgressNotify> (ClassID::ProgressDialog);
			ProgressNotifyScope notifyScope (progress);

			Url dstPath;
			System::GetSystem ().getLocation (dstPath, System::kTempFolder);
			succeeded = ExtensionManager::instance ().downloadUpdate (dstPath, *e, progress);
			if(succeeded)
			{
				succeeded = ExtensionManager::instance ().updateFile (dstPath, *e, progress);
				CCL::File (dstPath).remove ();
			}

			if(succeeded)
			{
				// reset state
				e->setUpdateAvailable (false);
				e->setNewVersion (VersionNumber ());
			}

			action.complete (succeeded);
			return true;
		}
		return false;
	}
	if(actionId == kEnable)
		return enable (e, action, true);
	if(actionId == kDisable)
		return enable (e, action, false);
	if(actionId == kUninstall)
		return uninstall (e, action);
	if(actionId == kAbortUninstall)
		return enable (e, action, true);

	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ExtensionManagerPackageHandler::cancelAction (UnifiedPackageAction& action)
{
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ExtensionManagerPackageHandler::isCancelEnabled (const UnifiedPackageAction& action) const
{
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Component* ExtensionManagerPackageHandler::createComponent (UnifiedPackage* package)
{
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringRef ExtensionManagerPackageHandler::getActionTitle (StringID actionId) const
{
	if(actionId == kAbortUninstall) return XSTR (AbortUninstall);
	return UnifiedPackageHandler::getActionTitle (actionId);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringRef ExtensionManagerPackageHandler::getStateLabel (StringID actionId) const
{
	return UnifiedPackageHandler::getStateLabel (actionId);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IImage* ExtensionManagerPackageHandler::getActionIcon (StringID actionId) const
{
	return UnifiedPackageHandler::getActionIcon (actionId);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringID ExtensionManagerPackageHandler::getActionGroupId (StringID actionId) const
{
	if(actionId == kAbortUninstall) return kInstall;
	return UnifiedPackageHandler::getActionGroupId (actionId);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ExtensionManagerPackageHandler::enable (ExtensionDescription* e, UnifiedPackageAction& action, bool state)
{
	ObjectArray extensions;
	extensions.add (e);
	bool succeeded = ExtensionManager::instance ().enable (extensions, state);
	action.requestRestart ();
	action.complete (succeeded);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ExtensionManagerPackageHandler::uninstall (ExtensionDescription* e, UnifiedPackageAction& action)
{
	bool succeeded = ExtensionManager::instance ().uninstall (*e);
	action.requestRestart ();
	action.packageChanged ();
	action.complete (succeeded);
	return true;
}

//************************************************************************************************
// ExtensionPackageSource
//************************************************************************************************

ExtensionPackageSource::ExtensionPackageSource ()
{
	flags |= kLocalSource;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ExtensionPackageSource::retrievePackages (UrlRef url, bool refresh)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

UnifiedPackage* ExtensionPackageSource::createFromFile (UrlRef url)
{
	AutoPtr<ExtensionDescription> description = ExtensionDescription::createFromPackage (url);
	if(description.isValid () == false)
		return nullptr;
	return createExtensionPackage (description);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ExtensionPackageSource::retrievePackage (ExtensionDescription* e)
{
	if(e == nullptr)
		return;

	if(e->isHidden ())
		return;

	AutoPtr<UnifiedPackage> package = createExtensionPackage (e);

	for(auto& item : e->getSubItems ())
	{
		AutoPtr<UnifiedPackage> child = createSubItemPackage (item, *package);
		package->addChild (child);
		announcePackage (child);
	}

	announcePackage (package);

	if(package->getChildren ().count () == 0 && e->getID () != e->getParentProductID () && e->getParentProductID ().isEmpty () == false)
	{
		AutoPtr<UnifiedPackage> productPackage = createPackage (e->getParentProductID ());

		productPackage->setTitle (e->getTitle ());
		productPackage->isLocalPackage (true);
		productPackage->setDescription (package->getDescription ());
		productPackage->setOrigin (package->getOrigin ());
		productPackage->isProduct (true);
		if(e->getPlatformIndependentIdentifier () == e->getParentProductID ())
			package->addChild (productPackage);
		else
			productPackage->addChild (package);
		announcePackage (productPackage);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

UnifiedPackage* ExtensionPackageSource::createExtensionPackage (Install::ExtensionDescription* e)
{
	AutoPtr<UnifiedPackage> package = createPackage (e->getID ());

	package->setTitle (e->getTitle ());
	package->setIcon (e->getIcon ());
	package->setInstalledVersion (e->getVersion ());
	package->setCurrentVersion (e->getNewVersion ());
	if(package->getCurrentVersion () == VersionNumber ())
		package->setCurrentVersion (package->getInstalledVersion ());
	package->setFileType (e->getFileType ());
	package->isLocalPackage (true);
	package->setDescription (e->getDescription ());
	package->setType (ExtensionStrings::ExtensionType (e->getType ()));
	package->setVendor (e->getVendor ());
	package->setWebsite (e->getWebsite ());

	if(ExtensionManagement::isUserInstalled (*e))
	{
		// We don't know if this is purchased content or subscription content
		// package->setOrigin (UnifiedPackage::kPurchasedContentOrigin);
	}
	else if(e->getType () == ExtensionType::kDeveloper)
	{
		#if SHOW_DEVELOPMENT_PACKAGES
		package->setOrigin (UnifiedPackage::kDevelopmentOrigin);
		#else
		return nullptr;
		#endif
	}
	else
		package->setOrigin (UnifiedPackage::kFactoryContentOrigin);

	package->setData (e);

	return package.detach ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

UnifiedPackage* ExtensionPackageSource::createSubItemPackage (ExtensionDescription::SubItem& item, const UnifiedPackage& package)
{
	String childId (item.getID ());
	if(childId == package.getId ())
		childId.append (".item");
	AutoPtr<UnifiedPackage> child = createPackage (childId);
	child->setTitle (item.getTitle ());
	child->setOrigin (package.getOrigin ());
	child->isLocalPackage (true);

	IPlugInSnapshots& snapshots = System::GetPluginSnapshots ();
	for(UIDRef cid : item.getClassIDs ())
	{
		if(IImage* snapshot = snapshots.getSnapshot (cid))
		{
			child->setIcon (snapshot);
			break;
		}
	}
	if(child->getDescription ().isEmpty ())
	{
		for(UIDRef cid : item.getClassIDs ())
		{
			String description;
			if(snapshots.getSnapshotDescription (description, cid))
			{
				child->setDescription (description);
				break;
			}
		}
	}

	return child.detach ();
}

//************************************************************************************************
// ExtensionManagerPackageSource
//************************************************************************************************

ExtensionManagerPackageSource::ExtensionManagerPackageSource ()
: extensionSink (NEW SignalSink (Signals::kExtensionManager))
{
	extensionSink->setObserver (this);
	extensionSink->enable (true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ExtensionManagerPackageSource::~ExtensionManagerPackageSource ()
{
	extensionSink->enable (false);
	delete extensionSink;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ExtensionManagerPackageSource::retrievePackages (UrlRef url, bool refresh)
{
	if(url.isRootPath ())
	{
		for(int i = 0; i < ExtensionManager::instance ().getExtensionCount (); ++i)
			retrievePackage (ExtensionManager::instance ().getExtensionDescription (i));
	}
	else
	{
		String id;
		url.getName (id, true);
		retrievePackage (ExtensionManager::instance ().findExtension (id));
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ExtensionManagerPackageSource::checkUpdates (const Container& packages, bool silent)
{
	// When not passing "progress" to checkUpdatesAsync, the operation runs with a modal progress window.
	// The returned operation is already completed (if not canceled or failed), so it's safe to release it here.
	AutoPtr<IAsyncOperation> operation = checkUpdatesAsync (packages, silent);
	return operation.isValid ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IAsyncOperation* ExtensionManagerPackageSource::checkUpdatesAsync (const Container& packages, bool silent, IProgressNotify* progress)
{
	ObjectArray extensions;
	for(UnifiedPackage* package : iterate_as<UnifiedPackage> (packages))
	{
		Install::ExtensionDescription* e = package->getData<Install::ExtensionDescription> ();
		if(e != nullptr && ExtensionManager::instance ().isUserInstalled (*e))
			extensions.add (e);
	}
	if(!extensions.isEmpty ())
		return ExtensionManager::instance ().checkUpdatesAsync (extensions, silent, progress);
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API ExtensionManagerPackageSource::notify (CCL::ISubject* subject, CCL::MessageRef msg)
{
	if(msg == Signals::kExtensionInstalled)
	{
		requestUpdate (IUnifiedPackageSink::kPackageAdded);
	}
}

//************************************************************************************************
// ExtensionFilterComponent
//************************************************************************************************

DEFINE_CLASS_HIDDEN (ExtensionFilterComponent, PackageFilterComponent);

//////////////////////////////////////////////////////////////////////////////////////////////////

ExtensionFilterComponent::ExtensionFilterComponent (PackageManager* manager)
: PackageFilterComponent (manager, "ExtensionFilter")
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ExtensionFilterComponent::matches (const UnifiedPackage& package) const
{
	if(!package.getChildren ().isEmpty ())
	{
		bool hasMatchingChild = false;
		for(UnifiedPackage* child : package.getChildren ())
		{
			if(matches (*child))
			{
				hasMatchingChild = true;
				break;
			}
		}
		if(!hasMatchingChild)
			return false;
	}

	VersionNumber versionNumber = package.getCurrentVersion ();
	if(versionNumber < package.getInstalledVersion ())
		versionNumber = package.getInstalledVersion ();

	if(!package.isLocalPackage () && versionNumber == VersionNumber ())
		versionNumber = VersionNumber (1000, 1000, 1000, 1000000);

	return ExtensionFilter::instance ().isCompatible (package.getId (), versionNumber);
}
