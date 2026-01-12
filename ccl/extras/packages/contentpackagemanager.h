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
// Filename    : ccl/extras/packages/contentpackagemanager.h
// Description : Content Package Manager
//
//************************************************************************************************

#ifndef _contentpackagemanager_h
#define _contentpackagemanager_h

#include "ccl/extras/packages/packagemanager.h"
#include "ccl/extras/packages/unifiedpackageaction.h"

#include "ccl/public/gui/commanddispatch.h"
#include "ccl/public/extras/icontentpackagemanager.h"

namespace CCL {
class PathSelectorWithHistory;
class PathSelector;
namespace Packages {
class AppVersionPackageFilterComponent;
class StaticFileTypePackageFilterComponent;
class FileTypePackageFilterComponent;
class ContentStateFilterComponent;
class OriginPackageFilterComponent;
class UpdateCheckProgress;
class ContentPackageInfo;

//************************************************************************************************
// ContentPackageManager
//************************************************************************************************

class ContentPackageManager: public PackageManager,
							 public IContentPackageManager,
							 public CommandDispatcher<ContentPackageManager>,
							 public ComponentSingleton<ContentPackageManager>
{
public:
	DECLARE_CLASS (ContentPackageManager, PackageManager)
	DECLARE_METHOD_NAMES (ContentPackageManager)

	ContentPackageManager (StringRef name = nullptr, StringRef title = nullptr);
	~ContentPackageManager ();

	enum StartupLevel
	{
		kStartupLocalSources,
		kStartupAll
	};

	void configure (StringRef appIdentity, const VersionNumber& appVersion);
	void addFileType (const FileType& type, StringRef targetFolder, StringRef title = nullptr);

	void startup (bool forceRun = false);
	void finishStartup (int level, bool defer = true);

	bool isInstalled (StringRef packageId) const;

	// Commands
	DECLARE_COMMANDS (ContentPackageManager)
	DECLARE_COMMAND_CATEGORY ("Application", Component)
	bool onCheckUserContent (CmdArgs);
	bool onCheckUpdates (CmdArgs);
	bool onInstallFromFile (CmdArgs);
	bool onInstallPackages (CmdArgs);

	// PackageManager
	void addPackage (UnifiedPackage* package) override;
	bool makeVisible (StringRef packageId, bool defer = false) override;
	void requestUpdate (IUnifiedPackageSource& source, int updateFlags) override;
	tresult CCL_API initialize (IUnknown* context = nullptr) override;
	tresult CCL_API terminate () override;
	tbool CCL_API getProperty (Variant& var, MemberID propertyId) const override;
	tbool CCL_API invokeMethod (Variant& returnValue, MessageRef msg) override;
	void CCL_API notify (ISubject* subject, MessageRef msg) override;
	tbool CCL_API canExecute (StringID actionId, const INotification& n) const override;
	tresult CCL_API execute (StringID actionId, INotification& n) override;
	void onCompletion (const UnifiedPackageAction& action, bool succeeded) override;

	// IContentPackageManager
	String CCL_API getAppVersion () const override;
	tresult CCL_API checkPackageVersion (StringRef packageId, IUpdateCheckObserver* observer) override;
	tresult CCL_API cancelVersionCheck (IUpdateCheckObserver* observer) override;
	tresult CCL_API triggerPackageInstallation (StringRef packageId) override;

	CLASS_INTERFACE2 (IContentPackageManager, INotificationActionHandler, PackageManager)

protected:
	static const String kContentServerPlaceholder;
	static const String kSettingsName;
	static const int kMaxNotificationCount = 5;

	struct InstallLocation
	{
		IUnifiedPackageHandler* handler = nullptr;
		UnifiedPackageInstallLocation info;
		PathSelector* selector;
	};
	Vector<InstallLocation> installLocations;
	
	ObjectArray staticFilters;
	ObjectArray packageInfo;
	DateTime lastContentUpdate;

	ContentStateFilterComponent* contentStateFilter;
	AppVersionPackageFilterComponent* versionFilter;
	StaticFileTypePackageFilterComponent* staticFileTypeFilter;
	FileTypePackageFilterComponent* fileTypeFilter;
	OriginPackageFilterComponent* originFilter;
	PathSelectorWithHistory* sourceSelector;
	PathSelectorWithHistory* targetSelector;

	int numUpdates;
	bool needsRestart;
	bool needsUpdateCheck;
	int startupLevel;
	bool suspendUpdateChecks;
	
	VersionNumber appVersion;
	Vector<UpdateCheckProgress*> pendingUpdateChecks;

	DECLARE_STRINGID_MEMBER (kCheckForUpdates)
	DECLARE_STRINGID_MEMBER (kFinishStartup)
	DECLARE_STRINGID_MEMBER (kInstallNewContentAction)
	DECLARE_STRINGID_MEMBER (kPackageIdAttribute)

	void setSourcePath (UrlRef path);
	void setTargetPath (UrlRef path);
	void updateStats ();
	void updateActionStates ();
	void runDialog ();
	bool checkUpdates (bool silent = false, bool cached = true);
	void checkNewContent (bool silent = false);
	void showNewContent ();
	void updateLocalSources ();
	void updateInstallLocations ();
	tresult triggerPackageInstallation (Vector<String> packageIds = {}, bool silent = false, bool refreshUserContent = false);
	void savePackageList ();
	void loadPackageList ();
	void sendNewContentNotification (const UnifiedPackage& package);
	void sendNewContentNotification (Vector<UnifiedPackage*>& packages);
	void onNotificationRemoved (MessageRef message);
	ContentPackageInfo* findPackageInfo (StringRef packageId, bool create = false);

	// PackageManager
	void applyConfiguration (int value) override;
	void retrievePackages (UrlRef url = UnifiedPackageUrl (), bool refresh = false) override;
	void performSelectedAction (int index, bool confirmed = false) override;
};

} // namespace Packages
} // namespace CCL

#endif // _contentpackagemanager_h
