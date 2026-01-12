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
// Filename    : ccl/extras/packages/contentpackagemanager.cpp
// Description : Content Package Manager
//
//************************************************************************************************

#include "ccl/extras/packages/contentpackagemanager.h"

#include "ccl/app/components/pathselector.h"

#include "ccl/base/asyncoperation.h"
#include "ccl/base/boxedtypes.h"
#include "ccl/base/message.h"
#include "ccl/base/storage/file.h"
#include "ccl/base/storage/storage.h"
#include "ccl/base/storage/settings.h"

#include "ccl/public/base/iprogress.h"
#include "ccl/public/text/translation.h"
#include "ccl/public/gui/iparameter.h"
#include "ccl/public/gui/framework/ialert.h"
#include "ccl/public/gui/framework/ifileselector.h"
#include "ccl/public/gui/framework/itheme.h"
#include "ccl/public/gui/framework/dialogbox.h"
#include "ccl/public/system/isysteminfo.h"
#include "ccl/public/system/formatter.h"
#include "ccl/public/systemservices.h"
#include "ccl/public/plugservices.h"
#include "ccl/public/guiservices.h"

#include "ccl/extras/packages/packagehandlerregistry.h"
#include "ccl/extras/packages/unifiedpackageinstaller.h"
#include "ccl/extras/packages/extensionmanagerpackages.h"
#include "ccl/extras/packages/factorycontentpackages.h"
#include "ccl/extras/extensions/appupdater.h"

namespace CCL {
namespace Packages {
	
//************************************************************************************************
// ContentPackageInfo
//************************************************************************************************

class ContentPackageInfo: public Object
{
public:
	DECLARE_CLASS (ContentPackageInfo, Object)
	
	PROPERTY_STRING (packageId, PackageID)
	PROPERTY_FLAG (flags, 1<<0, isKnown) // package has been seen by user

	ContentPackageInfo (StringRef packageID = nullptr);

	// Object
	bool equals (const Object& obj) const override;
	bool load (const Storage& storage) override;
	bool save (const Storage& storage) const override;

private:
	int flags;
};

//************************************************************************************************
// ContentStateFilterComponent
//************************************************************************************************

class ContentStateFilterComponent: public PackageFilterComponent
{
public:
	DECLARE_CLASS (ContentStateFilterComponent, PackageFilterComponent);

	ContentStateFilterComponent (PackageManager* manager = nullptr, IObjectFilter* fileTypeFilter = nullptr);

	enum States { kInstalled, kUpdateAvailable, kDownloadAvailable, kNumStates };

	int getStateForPackage (const UnifiedPackage& package) const;

	// PackageFilterComponent
	bool matches (const UnifiedPackage& package) const override;
	void update () override;

private:
	SharedPtr<IObjectFilter> fileTypeFilter;

	bool matches (const UnifiedPackage& package, int state) const;
};

//************************************************************************************************
// UpdateCheckProgress
//************************************************************************************************

class UpdateCheckProgress: public Object,
						   public AbstractProgressNotify
{
public:
	UpdateCheckProgress (IUpdateCheckObserver* observer, StringRef packageId)
	: canceled (false),
	  packageId (packageId),
	  observer (observer)
	{}

	PROPERTY_STRING (packageId, PackageID)
	PROPERTY_SHARED (IUpdateCheckObserver, observer, Observer)

	void cancel () { canceled = true; };

	// AbstractProgressNotify
	tbool CCL_API isCanceled () override { return canceled; }
	IProgressNotify* CCL_API createSubProgress () override { return NEW UpdateCheckProgress (nullptr, nullptr); }

	CLASS_INTERFACE (IProgressNotify, Object)

private:
	bool canceled;
};

} // namespace Packages
} // namespace CCL

using namespace CCL;
using namespace Packages;

//////////////////////////////////////////////////////////////////////////////////////////////////
// Tags
//////////////////////////////////////////////////////////////////////////////////////////////////

namespace Tag
{
	enum ContentPackageManagerTags
	{
		kListMode = 200,
		kSpaceFree,
		kShowInstallOptions
	};
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// Strings
//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_XSTRINGS ("ContentPackageManager")
	XSTRING (ContentServerPlaceholder, "Download from $APPCOMPANY Account")
	XSTRING (DiskSpaceExceeded, "Not enough free disk space. %(1) are required, but only %(2) are available on the selected volume.")
	XSTRING (AskContinue, "Do you want to continue installation anyway?")
	XSTRING (InstallNewContent, "Install")
	XSTRING (NewContentAvailable, "New content is available!")
	XSTRING (NewPackageAvailable, "%(1) is now available!")
END_XSTRINGS

//////////////////////////////////////////////////////////////////////////////////////////////////
// Commands
//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_COMMANDS (ContentPackageManager)
	DEFINE_COMMAND_ ("Application", "Check for Available Downloads", ContentPackageManager::onCheckUserContent, CommandFlags::kHidden)
	DEFINE_COMMAND_ ("Application", "Check for Updates", ContentPackageManager::onCheckUpdates, CommandFlags::kHidden)
	DEFINE_COMMAND_ ("Application", "Install from File", ContentPackageManager::onInstallFromFile, CommandFlags::kHidden)
	DEFINE_COMMAND_ARGS ("Application", "Install Packages", ContentPackageManager::onInstallPackages, CommandFlags::kHidden, "userContent,silent,ids")
END_COMMANDS (ContentPackageManager)

//************************************************************************************************
// ContentPackageManager
//************************************************************************************************

DEFINE_SINGLETON_CLASS (ContentPackageManager, PackageManager)
DEFINE_CLASS_UID (ContentPackageManager, 0xc077f684, 0xdc73, 0x4c44, 0x91, 0xaa, 0x10, 0xbc, 0x8, 0x5d, 0xf6, 0xcc) // ClassID::ContentPackageManager
DEFINE_CLASS_NAMESPACE (ContentPackageManager, "Host")
DEFINE_COMPONENT_SINGLETON (ContentPackageManager)
IMPLEMENT_COMMANDS (ContentPackageManager, PackageManager)

DEFINE_STRINGID_MEMBER_ (ContentPackageManager, kCheckForUpdates, "checkForUpdates")
DEFINE_STRINGID_MEMBER_ (ContentPackageManager, kFinishStartup, "finishStartup")
DEFINE_STRINGID_MEMBER_ (ContentPackageManager, kInstallNewContentAction, "installNewContent")
DEFINE_STRINGID_MEMBER_ (ContentPackageManager, kPackageIdAttribute, "packageId")

const String ContentPackageManager::kContentServerPlaceholder = "server";
const String ContentPackageManager::kSettingsName = "ContentPackages";

//////////////////////////////////////////////////////////////////////////////////////////////////

ContentPackageManager::ContentPackageManager (StringRef name, StringRef title)
: PackageManager (name.isEmpty () ? CCLSTR ("PackageManager") : name, title),
  versionFilter (nullptr),
  staticFileTypeFilter (nullptr),
  fileTypeFilter (nullptr),
  contentStateFilter (nullptr),
  originFilter (nullptr),
  sourceSelector (nullptr),
  targetSelector (nullptr),
  numUpdates (0),
  needsRestart (false),
  needsUpdateCheck (false),
  startupLevel (-1),
  suspendUpdateChecks (false)
{
	packageInfo.objectCleanup ();

	paramList.addParam ("listMode", Tag::kListMode);
	paramList.addString ("spaceFree", Tag::kSpaceFree);
	paramList.addParam ("showInstallOptions", Tag::kShowInstallOptions);

	sourceSelector = NEW PathSelectorWithHistory ("installSource");
	addComponent (sourceSelector);
	sourceSelector->addObserver (this);

	targetSelector = NEW PathSelectorWithHistory ("installTarget");
	addComponent (targetSelector);
	targetSelector->addObserver (this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ContentPackageManager::~ContentPackageManager ()
{
	ASSERT (pendingUpdateChecks.isEmpty ())

	for(InstallLocation& location : installLocations)
		location.selector->removeObserver (this);
		
	cancelSignals ();
	sourceSelector->removeObserver (this);
	targetSelector->removeObserver (this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API ContentPackageManager::initialize (IUnknown* context)
{
	SuperClass::initialize (context);

	// initialize path selectors
	if(UnifiedPackageInstaller::instance ().getInstallEngine ().getContentServer () != nullptr)
		sourceSelector->addUrl (Url (kContentServerPlaceholder), XSTR (ContentServerPlaceholder));

	Url contentFolder;
	System::GetSystem ().getLocation (contentFolder, System::kUserContentFolder);
	targetSelector->addUrl (contentFolder);
	System::GetSystem ().getLocation (contentFolder, System::kSharedContentFolder);
	targetSelector->addUrl (contentFolder);

	// configure known origins
	int packageOrigins[] =
	{
		Packages::UnifiedPackage::kUnknownOrigin,
		Packages::UnifiedPackage::kPurchasedContentOrigin,
		Packages::UnifiedPackage::kFactoryContentOrigin,
		Packages::UnifiedPackage::kDevelopmentOrigin
	};
	for(int origin : packageOrigins)
		addOrigin (origin, UnifiedPackageSourceBase::getLocalizedPackageOrigin (origin));

	// configure filters
	if(staticFileTypeFilter == nullptr)
		staticFileTypeFilter = NEW StaticFileTypePackageFilterComponent (this);
	addFilter (staticFileTypeFilter);
	staticFilters.add (staticFileTypeFilter);

	PackageSearchComponent* search = NEW PackageSearchComponent (this);
	search->setHidden (true);
	addFilter (search);
	addChild (return_shared (search));

	TagPackageFilterComponent* tagFilter = NEW TagPackageFilterComponent (this);
	tagFilter->setHidden (true);
	addFilter (tagFilter);
	addChild (return_shared (tagFilter));

	originFilter = NEW OriginPackageFilterComponent (this);
	addFilter (originFilter);

	if(fileTypeFilter == nullptr)
		fileTypeFilter = NEW FileTypePackageFilterComponent (this);
	addFilter (fileTypeFilter);

	if(contentStateFilter == nullptr)
	{
		contentStateFilter = NEW ContentStateFilterComponent (this, staticFileTypeFilter);
		contentStateFilter->addObserver (this);
	}
	contentStateFilter->setHidden (true);
	contentStateFilter->update ();
	contentStateFilter->select (ContentStateFilterComponent::kDownloadAvailable);
	addFilter (contentStateFilter);
	addChild (return_shared (contentStateFilter));

	if(versionFilter == nullptr)
		versionFilter = NEW AppVersionPackageFilterComponent (this);
	addFilter (versionFilter);

	addFilter (NEW ExtensionFilterComponent (this));

	// configure sorters
	addSorter (NEW NamePackageSorter);

	loadPackageList ();
	
	System::GetNotificationCenter ().registerHandler (this);
	UnknownPtr<ISubject> notificationCenter = &System::GetNotificationCenter ();
	signalSlots.advise (notificationCenter, INotificationCenter::kNotificationRemoved,
						this, &ContentPackageManager::onNotificationRemoved);

	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API ContentPackageManager::terminate ()
{
	UnknownPtr<ISubject> notificationCenter = &System::GetNotificationCenter ();
	signalSlots.unadvise (notificationCenter);
	System::GetNotificationCenter ().unregisterHandler (this);

	staticFilters.removeAll ();

	if(contentStateFilter != nullptr)
		contentStateFilter->removeObserver (this);

	if(startupLevel == kStartupAll)
		savePackageList ();

	return SuperClass::terminate ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ContentPackageManager::startup (bool forceRun)
{
	bool firstRun = false;
	bool mustRun = forceRun;

	if(!forceRun)
	{
		AutoPtr<Install::Manifest> manifest = FactoryContentPackageSource ().createManifest ();
		if(manifest == nullptr)
			return;

		VersionNumber manifestVersion = manifest->getVersion ();
		VersionNumber historyVersion = UnifiedPackageInstaller::instance ().getHistoryVersion ();

		if(manifestVersion > historyVersion)
		{
			firstRun = true;
			mustRun = true;
		}
	}
	
	bool mustFinishStartup = mustRun;
	if(!mustFinishStartup)
	{
		int64 now = UnixTime::getTime ();
		int64 lastUpdate = UnixTime::fromLocal (lastContentUpdate);
		if(now - lastUpdate > DateTime::kSecondsInDay * 7) // update content list at least once in a week
			mustFinishStartup = true;
	}

	if(mustFinishStartup)
	{
		finishStartup (kStartupAll, false);
		if(mustRun)
		{
			contentStateFilter->select (ContentStateFilterComponent::kDownloadAvailable);
			if(firstRun)
				setInstallConfiguration (kRecommendedInstall, true);
			runDialog ();
		}
	}
	else
		finishStartup (kStartupLocalSources);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ContentPackageManager::finishStartup (int level, bool defer)
{
	if(startupLevel < level)
	{
		if(defer)
			(NEW Message (kFinishStartup, level))->post (this, 100);
		else
		{
			startupLevel = level;
			if(level == kStartupLocalSources)
				updateLocalSources ();
			else if(level == kStartupAll)
			{
				updateAll (false);
				checkNewContent ();
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ContentPackageManager::requestUpdate (IUnifiedPackageSource& source, int updateFlags)
{
	if(startupLevel < kStartupLocalSources)
		return;
	if((updateFlags & (kPackageRemoved|kPackageChanged)) != 0 && startupLevel < kStartupAll)
	{
		updateLocalSources ();
		return;
	}
	SuperClass::requestUpdate (source, updateFlags);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ContentPackageManager::updateLocalSources ()
{
	deferChanged ();
	for(IUnifiedPackageSource* source : sources)
		if((source->getFlags () & IUnifiedPackageSource::kLocalSource) != 0)
			source->retrievePackages (UnifiedPackageUrl (), false);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ContentPackageManager::updateInstallLocations ()
{
	for(InstallLocation& location : installLocations)
	{
		removeChild (location.selector);
		location.selector->removeObserver (this);
		safe_release (location.selector);
	}
	installLocations.removeAll ();

	for(IUnifiedPackageHandler* handler : PackageHandlerRegistry::instance ().getHandlers ())
	{
		Vector<UnifiedPackageInstallLocation> locations;
		if(!handler->getInstallLocations (locations))
			continue;

		for(const UnifiedPackageInstallLocation& info: locations)
		{
			InstallLocation location;
			location.handler = handler;
			location.selector = NEW PathSelector (String (info.id));
			location.selector->setPath (info.path);
			location.selector->addObserver (this);
			location.info = info;

			addComponent (location.selector);
			installLocations.add (location);
		}
	}

	deferSignal (NEW Message (kPropertyChanged));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ContentPackageManager::getProperty (Variant& var, MemberID propertyId) const
{
	if(propertyId == "sourceMode")
	{
		if(sourceSelector->getPath ().getHostName () == kContentServerPlaceholder)
			var = 1;
		else
			var = 0;
		return true;
	}
	else if(propertyId == "numUpdates")
	{
		var = numUpdates;
		return true;
	}
	else if(propertyId == "installLocationCount")
	{
		var = installLocations.count ();
		return true;
	}
	else if(propertyId.contains ("-"))
	{
		int index = -1;
		MutableCString postfix = propertyId.subString (propertyId.index ("-") + 1);
		index = String (postfix).scanInt ();

		if(propertyId.startsWith ("installLocationDescription"))
		{
			var.fromString (installLocations.at (index).info.description);
			return true;
		}
		if(propertyId.startsWith ("installLocationId"))
		{
			var = installLocations.at (index).info.id;
			return true;
		}
	}
	return SuperClass::getProperty (var, propertyId);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_METHOD_NAMES (ContentPackageManager)
	DEFINE_METHOD_ARGS ("finishStartup", "deferred: bool = true")
	DEFINE_METHOD_ARGR ("findPackage", "packageId: string", "UnifiedPackage")
	DEFINE_METHOD_ARGR ("canInstall", "packageId: string", "bool")
	DEFINE_METHOD_ARGR ("isInstalled", "packageId: string", "bool")
END_METHOD_NAMES (ContentPackageManager)

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ContentPackageManager::invokeMethod (Variant& returnValue, MessageRef msg)
{
	if(msg == "finishStartup")
	{
		bool deferred = true;
		if(msg.getArgCount () > 0)
			deferred = msg[0].asBool ();

		finishStartup (kStartupAll, deferred);

		return true;
	}
	else if(msg == "findPackage")
	{
		String packageId = msg[0];
		returnValue.takeShared (ccl_as_unknown (findPackage (packageId)));
		return true;
	}
	else if(msg == "canInstall")
	{
		String packageId = msg[0];
		returnValue = canInstall (packageId);
		return true;
	}
	else if(msg == "isInstalled")
	{
		String packageId = msg[0];
		returnValue = isInstalled (packageId);
		return true;
	}
	else
		return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API ContentPackageManager::notify (ISubject* subject, MessageRef msg)
{
	if(subject == targetSelector)
	{
		setTargetPath (targetSelector->getPath ());
		updateStats ();
	}
	else if(subject == sourceSelector)
	{
		setSourcePath (sourceSelector->getPath ());
	}
	else if(msg == kChanged && subject == contentStateFilter && contentStateFilter != nullptr)
	{
		if(contentStateFilter->getSelection () == ContentStateFilterComponent::kInstalled)
			setSectionPropertyId ("state");
		else
			setSectionPropertyId ("");

		if(contentStateFilter->getSelection () == ContentStateFilterComponent::kDownloadAvailable)
		{
			originFilter->setEnabled (true);
			originFilter->setHidden (false);
		}
		else
		{
			originFilter->setEnabled (false);
			originFilter->setHidden (true);
		}

		deselectFiltered ();
		setInstallConfiguration (kCustomInstall);
		sortComponents ();
	}
	else if(msg == kChanged)
	{
		for(InstallLocation& location : installLocations)
		{
			if(location.selector == subject)
			{
				if(location.handler->setInstallLocation (location.info.id, location.selector->getPath ()))
					location.info.path = location.selector->getPath ();
				else
					location.selector->setPath (location.info.path);
			}
		}

		updateActionStates ();
	}
	else if(msg == kCheckForUpdates)
	{
		checkUpdates (true, false);
	}
	else if(msg == kFinishStartup && msg.getArgCount () > 0)
	{
		finishStartup (msg[0].asInt (), false);
	}
	SuperClass::notify (subject, msg);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ContentPackageManager::addPackage (UnifiedPackage* package)
{
	for(Object* object : staticFilters)
	{
		UnknownPtr<IObjectFilter> filter (object->asUnknown ());
		if(filter)
		{
			if(package->isTopLevel () && filter->matches (ccl_as_unknown (package)) == false)
				return;

			for(int i = 0; i < package->getChildren ().count (); i++)
			{
				UnifiedPackage* child = package->getChildren ().at (i);
				if(filter->matches (ccl_as_unknown (child)) == false)
				{
					package->removeChild (child);
					i--;
				}
			}
		}
	}

	if(package->isLocalPackage () && findPackage (package->getId ()) == nullptr)
	{
		if(suspendUpdateChecks || startupLevel < kStartupAll)
			needsUpdateCheck = true;
		else
			(NEW Message (kCheckForUpdates))->post (this, 10);
	}

	SuperClass::addPackage (package);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ContentPackageManager::makeVisible (StringRef packageId, bool defer)
{
	if(defer == false)
	{
		resetFilters ();
		UnifiedPackage* package = findPackage (packageId);
		if(package && contentStateFilter)
		{
			int state = contentStateFilter->getStateForPackage (*package);
			if(contentStateFilter->getSelection () != state)
			{
				contentStateFilter->select (state);
				(NEW Message (kMakeVisible, packageId))->post (this, 500);
				return true;
			}
		}
	}
	return SuperClass::makeVisible (packageId, defer);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ContentPackageManager::configure (StringRef identity, const VersionNumber& version)
{
	appVersion = version;
	if(versionFilter == nullptr)
		versionFilter = NEW AppVersionPackageFilterComponent (this);
	versionFilter->addSupportedVersion (identity, version);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ContentPackageManager::addFileType (const FileType& type, StringRef targetFolder, StringRef title)
{
	if(staticFileTypeFilter == nullptr)
		staticFileTypeFilter = NEW StaticFileTypePackageFilterComponent (this);
	staticFileTypeFilter->addFileType (type);

	if(title.isEmpty () == false)
	{
		if(fileTypeFilter == nullptr)
			fileTypeFilter = NEW FileTypePackageFilterComponent (this);
		fileTypeFilter->addFileType (type, title);
	}

	UnifiedPackageInstaller& packageInstaller = UnifiedPackageInstaller::instance ();
	packageInstaller.addFileType (type, targetFolder);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ContentPackageManager::setSourcePath (UrlRef path)
{
	UnifiedPackageInstaller& packageInstaller = UnifiedPackageInstaller::instance ();
	if(path.getHostName () == kContentServerPlaceholder)
		packageInstaller.setSourcePath (Url::kEmpty);
	else
		packageInstaller.setSourcePath (path);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ContentPackageManager::setTargetPath (UrlRef path)
{
	UnifiedPackageInstaller& packageInstaller = UnifiedPackageInstaller::instance ();
	packageInstaller.setTargetPath (path);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ContentPackageManager::updateActionStates ()
{
	numUpdates = 0;

	ObjectArray packages;
	packages.objectCleanup ();
	getPackages (packages);
	for(UnifiedPackage* package : iterate_as<UnifiedPackage> (packages))
	{
		ObjectArray actions;
		actions.objectCleanup ();
		getActions (actions, *package);
		for(UnifiedPackageAction* action : iterate_as<UnifiedPackageAction> (actions))
		{
			if(action->getId () == UnifiedPackageHandler::kUpdate && action->getState () >= UnifiedPackageAction::kEnabled)
				numUpdates++;
		}
	}

	deferSignal (NEW Message (kPropertyChanged));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ContentPackageManager::updateStats ()
{
	VolumeInfo targetInfo;
	System::GetFileSystem ().getVolumeInfo (targetInfo, targetSelector->getPath ());
	paramList.byTag (Tag::kSpaceFree)->fromString (Format::ByteSize::print ((double)targetInfo.bytesFree));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ContentPackageManager::onCheckUserContent (CmdArgs msg)
{
	if(!msg.checkOnly ())
	{
		ScopedVar<bool> scope (suspendUpdateChecks, true);
		refresh ();
		checkUpdates ();
		checkNewContent ();
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ContentPackageManager::onCheckUpdates (CmdArgs msg)
{
	if(!msg.checkOnly ())
		return checkUpdates (false, false);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ContentPackageManager::checkUpdates (bool silent, bool cached)
{
	needsUpdateCheck = false;

	Attributes args;
	if(cached)
		args.set ("Cached", true);

	if(silent)
		Install::ApplicationUpdater::instance ().checkAppUpdatesInBackground ();
	else if(Install::ApplicationUpdater::instance ().onCheckUpdates (CommandMsg (nullptr, nullptr, args.asUnknown ())) == false)
		return false;

	if(Install::ApplicationUpdater::instance ().isUpdateAvailable () == false)
	{
		ObjectArray packages;
		packages.objectCleanup ();
		getPackages (packages);
		if(ExtensionManagerPackageSource::checkUpdates (packages, silent))
			updateAll ();
	}

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ContentPackageManager::checkNewContent (bool silent)
{
	bool mustCheckNewContent = !packageInfo.isEmpty ();
	
	ObjectArray packages;
	packages.objectCleanup ();
	getInstallableProducts (packages);

	Vector<UnifiedPackage*> unknownPackages;

	for(UnifiedPackage* package : iterate_as<UnifiedPackage> (packages))
	{
		ContentPackageInfo* info = findPackageInfo (package->getId (), true);
		info->isKnown (!mustCheckNewContent || info->isKnown () || package->isLocalPackage ());
		if(!silent && !info->isKnown ())
			unknownPackages.add (package);
	}

	if(unknownPackages.count () <= kMaxNotificationCount)
	{
		for(int i = 0; i < unknownPackages.count (); i++)
			sendNewContentNotification (*unknownPackages[i]);
	}
	else
		sendNewContentNotification (unknownPackages);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ContentPackageManager::showNewContent ()
{
	Vector<String> packageIds;
	for(ContentPackageInfo* info : iterate_as<ContentPackageInfo> (packageInfo))
	{
		if(!info->isKnown ())
		{
			packageIds.add (info->getPackageID ());
			info->isKnown (true);
		}
	}
	triggerPackageInstallation (packageIds);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ContentPackageManager::sendNewContentNotification (const UnifiedPackage& package)
{
	NotificationActionProperties actionProperties {MutableCString (kInstallNewContentAction).append (".").append (package.getId ()), XSTR (InstallNewContent)};
	Attributes notificationAttributes;
	IImage* icon = package.getIcon ();
	if(icon == nullptr && package.getChildren ().count () == 1 && package.getChildren ()[0]->getIcon ())
		icon = package.getChildren ()[0]->getIcon ();
	if(icon)
		notificationAttributes.setAttribute (INotification::kIcon, icon, Attributes::kShare);
	String description = package.getDescription ();
	if(description.isEmpty () && package.getChildren ().count () == 1)
		description = package.getChildren ()[0]->getDescription ();
	notificationAttributes.setAttribute (kPackageIdAttribute, package.getId (), Attributes::kShare);
	System::GetNotificationCenter ().sendInAppNotification (String ().appendFormat (XSTR (NewPackageAvailable), package.getTitle ()), description, &notificationAttributes, &actionProperties, 1);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ContentPackageManager::sendNewContentNotification (Vector<UnifiedPackage*>& packages)
{
	NotificationActionProperties actionProperties {kInstallNewContentAction, XSTR (InstallNewContent)};
	Attributes notificationAttributes;
	notificationAttributes.setAttribute (kPackageIdAttribute, String::kEmpty, Attributes::kShare);
	String message;
	for(int i = 0; i < packages.count (); i++)
		message.appendFormat ("%(1)\n", packages[i]->getTitle ());
	System::GetNotificationCenter ().sendInAppNotification (XSTR (NewContentAvailable), message, &notificationAttributes, &actionProperties, 1);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ContentPackageManager::onNotificationRemoved (MessageRef message)
{
	if(message.getArgCount () > 0)
	{
		UnknownPtr<INotification> notification = message[0].asUnknown ();
		if(notification && notification->getAttributes ().contains (kPackageIdAttribute))
		{
			Variant packageId;
			notification->getAttributes ().getAttribute (packageId, kPackageIdAttribute);
			if(packageId.asString ().isEmpty ())
			{
				for(ContentPackageInfo* info : iterate_as<ContentPackageInfo> (packageInfo))
					info->isKnown (true);
			}
			else
			{
				if(ContentPackageInfo* info = findPackageInfo (packageId, true))
					info->isKnown (true);
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ContentPackageInfo* ContentPackageManager::findPackageInfo (StringRef packageId, bool create)
{
	ContentPackageInfo* existingInfo = ccl_cast<ContentPackageInfo> (packageInfo.findEqual (ContentPackageInfo (packageId)));
	ContentPackageInfo* info = existingInfo;
	if(info == nullptr && create)
	{
		info = NEW ContentPackageInfo (packageId);
		packageInfo.add (info);
	}
	return info;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ContentPackageManager::onInstallFromFile (CmdArgs msg)
{
	if(!msg.checkOnly ())
	{
		AutoPtr<IFileSelector> fs = ccl_new<IFileSelector> (CCL::ClassID::FileSelector);
		if(fs.isValid () == false)
			return false;
		for(const FileType& fileType : staticFileTypeFilter->getFileTypes ())
			fs->addFilter (fileType);
		if(fs->run (IFileSelector::kOpenFile))
		{
			bool succeeded = false;

			AutoPtr<UnifiedPackage> package = createPackageFromFile (*fs->getPath ());
			if(package.isValid ())
			{
				AutoPtr<File> file = NEW File (*fs->getPath ());
				if(file->isFile () && file->exists ())
					package->setData<File> (file);

				package->isLocalPackage (false);
				package->isLocalInstallationAllowed (true);

				succeeded = installPackage (package);
			}

			if(succeeded == false)
			{
				String message;
				String name;
				fs->getPath ()->getName (name);
				message.appendFormat (PackageInstallerStrings::InstallFailed (), name);
				reportEvent ({message, Alert::kError});
			}
		}
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ContentPackageManager::onInstallPackages (CmdArgs msg)
{
	if(!msg.checkOnly ())
	{
		IAttributeList* args = CommandAutomator::getArguments (msg);
		bool refreshUserContent = false;
		bool silent = false;

		Vector<String> packageIds;
		if(args)
		{
			refreshUserContent = AttributeReadAccessor (*args).getBool ("userContent");
			silent = AttributeReadAccessor (*args).getBool ("silent");

			AttributeReadAccessor reader (*args);
			IterForEachUnknown (reader.newUnknownIterator ("ids"), unk)
				if(UnknownPtr<IAttribute> attribute = unk)
				{
					String packageId = attribute->getValue ().asString ();
					if(packageId.isEmpty () == false)
						packageIds.add (packageId);
				}
			EndFor
		}

		triggerPackageInstallation (packageIds, silent, refreshUserContent);
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult ContentPackageManager::triggerPackageInstallation (Vector<String> packageIds, bool silent, bool refreshUserContent)
{
	resetFilters ();
	contentStateFilter->select (ContentStateFilterComponent::kDownloadAvailable);

	if(refreshUserContent)
	{
		refresh ();
		checkUpdates (true);
	}
	else
		updateAll (false);

	startupLevel = kStartupAll;

	selectAll (false);
	
	for(StringRef packageId : packageIds)
	{
		if(!makeVisible (packageId, false))
			makeVisible (packageId, true);
		select (packageId, true, !silent);
	}

	if(silent)
	{
		updateSelectedActions (false);
		for(int i = 0; i < selectedActions.count (); i++)
		{
			if(selectedActions.at (i).id == UnifiedPackageHandler::kInstall)
			{
				performSelectedAction (i, true);
				break;
			}
		}
	}
	else
		runDialog ();

	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ContentPackageManager::isInstalled (StringRef packageId) const
{
	UnifiedPackage* package = findPackage (packageId);
	if(package == nullptr)
		return false;
	Install::Manifest* manifest = nullptr;
	for(int i = 0; manifest = package->getData<Install::Manifest> (i); i++)
	{
		if(Install::File* file = manifest->findFile (package->getId ()))
		{
			if(UnifiedPackageInstaller::instance ().isInstalled (file))
				return true;
		}
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ContentPackageManager::performSelectedAction (int index, bool confirmed)
{
	StringID actionId = selectedActions.at (index).id;

	if(confirmed == false && actionId == UnifiedPackageHandler::kInstall)
	{
		VolumeInfo targetInfo;
		System::GetFileSystem ().getVolumeInfo (targetInfo, targetSelector->getPath ());

		int64 requiredSpace = selectedActions.at (index).size;

		if(requiredSpace >= targetInfo.bytesFree)
		{
			SharedPtr<ContentPackageManager> This (this);
			String message;
			message.appendFormat (XSTR (DiskSpaceExceeded), Format::ByteSize::print (requiredSpace), Format::ByteSize::print (targetInfo.bytesFree));
			message.append (String::getLineEnd ());
			message.append (XSTR (AskContinue));
			Promise warn (Alert::askAsync (message));
			warn.then ([This, index] (IAsyncOperation& operation)
			{
				if(operation.getResult ().asInt () == Alert::kYes)
				{
					This->performSelectedAction (index, true);
				}
			});
			return;
		}
	}

	SuperClass::performSelectedAction (index, confirmed);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ContentPackageManager::onCompletion (const UnifiedPackageAction& action, bool succeeded)
{
	if(action.getId () == UnifiedPackageHandler::kInstall)
	{
		if(UnifiedPackage* package = action.getPackage ())
			deferSignal (NEW Message (kPackageInstalled, Variant (package->getId ()), succeeded));
	}
	SuperClass::onCompletion (action, succeeded);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ContentPackageManager::runDialog ()
{
	IView* view = getTheme ()->createView ("PackageManager/PackageManager", this->asUnknown ());
	ASSERT (view != nullptr)
	if(view)
	{
		DialogBox dialog;
		dialog->runDialog (view);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ContentPackageManager::applyConfiguration (int value)
{
	SuperClass::applyConfiguration (value);
	if(value == kMinimalInstall || value == kRecommendedInstall)
	{
		if(originFilter)
			originFilter->select (String ().appendIntValue (UnifiedPackage::kFactoryContentOrigin));
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ContentPackageManager::retrievePackages (UrlRef url, bool refresh)
{
	{
		ScopedVar<bool> scope (suspendUpdateChecks, true);
		SuperClass::retrievePackages (url, refresh);
	}
	updateStats ();
	updateInstallLocations ();
	if(needsUpdateCheck && startupLevel >= kStartupAll && !suspendUpdateChecks)
		checkUpdates (true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

String CCL_API ContentPackageManager::getAppVersion () const
{
	return appVersion.print (VersionNumber::kMedium);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API ContentPackageManager::checkPackageVersion (StringRef packageId, IUpdateCheckObserver* observer)
{
	SharedPtr<UnifiedPackage> package = findPackage (packageId);
	if(!package.isValid ())
		return kResultClassNotFound;
	
	ObjectArray packages;
	packages.add (package);
	UpdateCheckProgress* progress = NEW UpdateCheckProgress (observer, packageId);
	pendingUpdateChecks.add (progress);
	Promise promise (ExtensionManagerPackageSource::checkUpdatesAsync (packages, true, progress));
	promise.then ([this, observer, progress, package] (const IAsyncOperation& operation)
	{
		tresult result = kResultFailed;
		if(operation.getState () == IAsyncOperation::kCompleted)
		{
			result = kResultOk;
			if(auto* component = findPackageComponent (*package))
			{
				ScopedVar<bool> scope (suspendUpdateChecks, true);
				update (component);
			}
		}

		String installedVersion = package->getInstalledVersion ().print (VersionNumber::kMedium);
		String currentVersion = package->getCurrentVersion ().print (VersionNumber::kMedium);

		observer->onVersionCheckCompleted (progress->getPackageID (), installedVersion, currentVersion, result);

		pendingUpdateChecks.remove (progress);
		progress->release ();
	});

	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API ContentPackageManager::cancelVersionCheck (IUpdateCheckObserver* observer)
{
	UpdateCheckProgress** progress = pendingUpdateChecks.findIf ([observer] (const UpdateCheckProgress* item) { return item->getObserver () == observer; });
	if(progress && *progress)
	{
		(*progress)->cancel ();
		return kResultOk;
	}
	return kResultFailed;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API ContentPackageManager::triggerPackageInstallation (StringRef packageId)
{
	return triggerPackageInstallation ({ packageId }, false, true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ContentPackageManager::loadPackageList ()
{
	XmlSettings settings (kSettingsName);
	if(settings.restore ())
	{
		Boxed::DateTime lastUpdate;
		if(settings.getAttributes ("content").get (lastUpdate, "updated"))
			lastContentUpdate = lastUpdate;
		settings.getAttributes ("content").unqueue (packageInfo, nullptr, ccl_typeid<ContentPackageInfo> ());
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ContentPackageManager::savePackageList ()
{
	checkNewContent (true);

	XmlSettings settings (kSettingsName);
	
	DateTime now;
	System::GetSystem ().getLocalTime (now);
	settings.getAttributes ("content").set ("updated", Boxed::DateTime (now));
	settings.getAttributes ("content").queue (nullptr, packageInfo, Attributes::kShare);
	settings.flush ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ContentPackageManager::canExecute (StringID actionId, const INotification& n) const
{
	if(actionId.startsWith (kInstallNewContentAction) && n.getCategory () == INotificationCenter::kInAppNotificationCategory)
		return true;
	return SuperClass::canExecute (actionId, n);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API ContentPackageManager::execute (StringID actionId, INotification& n)
{
	if(actionId == kInstallNewContentAction)
	{
		System::GetNotificationCenter ().removeNotification (&n);
		showNewContent ();
		return kResultOk;
	}
	else if(actionId.startsWith (kInstallNewContentAction))
	{
		String packageId (Text::kUTF8, actionId.subString (kInstallNewContentAction.length () + 1));
		if(installPackage (packageId))
		{
			System::GetNotificationCenter ().removeNotification (&n);
			return kResultOk;
		}
	}
	
	return SuperClass::execute (actionId, n);
}

//************************************************************************************************
// ContentStateFilterComponent
//************************************************************************************************

DEFINE_CLASS_HIDDEN (ContentStateFilterComponent, PackageFilterComponent)

//////////////////////////////////////////////////////////////////////////////////////////////////

ContentStateFilterComponent::ContentStateFilterComponent (PackageManager* manager, IObjectFilter* fileTypeFilter)
: PackageFilterComponent (manager, "PackageContentStateFilter", "ContentState"),
  fileTypeFilter (fileTypeFilter)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

int ContentStateFilterComponent::getStateForPackage (const UnifiedPackage& package) const
{
	for(int i = 0; i < kNumStates; i++)
	{
		if(matches (package, i))
			return i;
	}
	return -1;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ContentStateFilterComponent::matches (const UnifiedPackage& package) const
{
	return matches (package, selectionParameter->getValue ().asInt ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ContentStateFilterComponent::matches (const UnifiedPackage& package, int state) const
{
	ObjectArray actions;
	actions.objectCleanup ();
	manager->getActions (actions, package);

	for(UnifiedPackageAction* action : iterate_as<UnifiedPackageAction> (actions))
	{
		if(action->getId () == UnifiedPackageHandler::kUpdate && action->getState () > UnifiedPackageAction::kDisabled)
			return state == kUpdateAvailable;
	}

	for(UnifiedPackageAction* action : iterate_as<UnifiedPackageAction> (actions))
	{
		if(action->getId () == UnifiedPackageHandler::kUninstall)
			return state == kInstalled;
	}

	for(UnifiedPackageAction* action : iterate_as<UnifiedPackageAction> (actions))
	{
		if(action->getId () == UnifiedPackageHandler::kInstall && action->getState () > UnifiedPackageAction::kDisabled)
			return state == kDownloadAvailable;
	}

	bool hasInstallableChild = false;
	bool hasInstalledChild = false;
	bool hasUpdatableChild = false;
	for(UnifiedPackage* child : package.getChildren ())
	{
		ObjectArray actions;
		actions.objectCleanup ();
		manager->getActions (actions, *child);
		for(UnifiedPackageAction* action : iterate_as<UnifiedPackageAction> (actions))
		{
			if(action->getId () == UnifiedPackageHandler::kUninstall)
				hasInstalledChild = true;
			if(action->getId () == UnifiedPackageHandler::kInstall && action->getState () > UnifiedPackageAction::kDisabled && (fileTypeFilter == nullptr || fileTypeFilter->matches (child->asUnknown ())))
				hasInstallableChild = true;
			if(action->getId () == UnifiedPackageHandler::kUpdate && action->getState () > UnifiedPackageAction::kDisabled)
				hasUpdatableChild = true;

			if(hasInstalledChild && hasInstallableChild && hasUpdatableChild)
				break;
		}
		if(actions.isEmpty () && child->isLocalPackage ())
			hasInstalledChild = true;
		if(hasInstalledChild && hasInstallableChild && hasUpdatableChild)
			break;
	}

	// show top level packages which contain matching childs
	if(hasInstallableChild && state == kDownloadAvailable)
		return true;
	if(hasUpdatableChild && state == kUpdateAvailable)
		return true;
	if((hasInstalledChild || (package.isTopLevel () && hasInstallableChild == false)) && state == kInstalled)
		return true;
	if(hasInstallableChild == false && hasInstalledChild == false && hasUpdatableChild == false && package.isTopLevel () == false)
	{
		if(package.isLocalPackage ())
			return state == kInstalled;
		return true;
	}

	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ContentStateFilterComponent::update ()
{
	if(items.count () == 0)
	{
		addItem ("installed", kInstalled);
		addItem ("updateAvailable", kUpdateAvailable);
		addItem ("downloadAvailable", kDownloadAvailable);

		deferChanged ();
		deferSignal (NEW Message (kPropertyChanged));
	}
}

//************************************************************************************************
// ContentPackageInfo
//************************************************************************************************

DEFINE_CLASS_PERSISTENT (ContentPackageInfo, Object, "ContentPackageInfo")

//////////////////////////////////////////////////////////////////////////////////////////////////

ContentPackageInfo::ContentPackageInfo (StringRef packageId)
: packageId (packageId),
  flags (0)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ContentPackageInfo::equals (const Object& obj) const
{
	const ContentPackageInfo* other = ccl_cast<ContentPackageInfo> (&obj);
	if(other == nullptr)
		return false;
	return other->getPackageID () == packageId;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ContentPackageInfo::load (const Storage& storage)
{
	Attributes& a = storage.getAttributes ();
	packageId = a.getString ("id");
	isKnown (a.getBool ("known"));
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ContentPackageInfo::save (const Storage& storage) const
{
	Attributes& a = storage.getAttributes ();
	a.set ("id", packageId);
	a.set ("known", isKnown ());
	return true;
}
