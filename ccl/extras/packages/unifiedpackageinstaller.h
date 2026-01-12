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
// Filename    : ccl/extras/packages/unifiedpackageinstaller.h
// Description : Unified Package Installer
//
//************************************************************************************************

#ifndef _ccl_unifiedpackageinstaller_h
#define _ccl_unifiedpackageinstaller_h

#include "ccl/base/singleton.h"

#include "ccl/extras/packages/unifiedpackageaction.h"
#include "ccl/extras/extensions/contentinstallengine.h"

#include "ccl/public/gui/framework/idleclient.h"

namespace CCL {
namespace Packages {

class InstallTransaction;

//************************************************************************************************
// PackageInstallerStrings
//************************************************************************************************

namespace PackageInstallerStrings
{
	StringRef InstallFailed ();
	StringRef RestartRequired ();
}

//************************************************************************************************
// UnifiedPackageInstaller
/** UnifiedPackageHandler used to install packages. */
//************************************************************************************************

class UnifiedPackageInstaller: public UnifiedPackageHandler,
							   public StaticSingleton<UnifiedPackageInstaller>,
							   public Install::IContentInstallEngineObserver,
							   public IdleClient
{
public:
	DECLARE_CLASS (UnifiedPackageInstaller, Object)

	UnifiedPackageInstaller ();
	~UnifiedPackageInstaller ();

	DECLARE_STRINGID_MEMBER (kRunInstallation)

	PROPERTY_BOOL (checkAuthorization, AuthorizationCheckEnabled) // only start transactions for authorized packages

	void initialize (const VersionNumber& version);
	void terminate ();
	void setContentServer (Install::IContentServer* server);
	void setAppProductID (StringRef identity);
	void addFileType (const FileType& fileType, StringRef targetFolder = nullptr);
	void setTargetPath (UrlRef path);
	void setSourcePath (UrlRef path);
	bool isInstallingPackage (StringRef packageId) const;
	bool getPackageLocation (Url& path, StringRef packageId) const;
	Install::ContentInstallEngine& getInstallEngine ();
	VersionNumber getHistoryVersion () const;
	bool isInstalled (Install::File* package) const;

	// UnifiedPackageHandler
	bool canHandle (UnifiedPackage* package) const override;
	void getActions (Container& actions, UnifiedPackage* package = nullptr) override;
	void updateAction (UnifiedPackageAction& action) override;
	bool performAction (UnifiedPackageAction& action) override;
	bool cancelAction (UnifiedPackageAction& action) override;
	Component* createComponent (UnifiedPackage* package) override;
	void CCL_API notify (ISubject* subject, MessageRef msg) override;
	bool pauseAction (UnifiedPackageAction& action, bool state) override;
	
	CLASS_INTERFACE2 (IContentInstallEngineObserver, ITimerTask, UnifiedPackageHandler)

private:
	Install::ContentInstallEngine engine;
	enum class Step { kPrepare, kInstall };

	Install::History history;
	VersionNumber version;
	Url installationSourcePath;

	ObjectArray installQueue;
	ObjectArray activeTransactions;
	ObjectArray finishedTransactions;
	bool preparingInstallation;
	bool restartRequired;
	bool insideInstallationDone;

	bool addTransaction (Install::File* file, UrlRef srcPath, bool isExtension, bool isLocal, UnifiedPackageAction* action);
	bool isAuthorized (const UnifiedPackage& package) const;

	void runInstallation ();	
	void performInstallationStep (Step step);
	
	bool cancelInstallation (StringRef id);
	InstallTransaction* findTransaction (StringRef id) const;
	IFileInstallHandler* findHandlerForFile (UrlRef path) const;

	void saveHistory ();
	
	// IContentInstallEngineObserver
	void onFileInstallationSucceeded (const Install::File& file, const DateTime& time, UrlRef path) override;
	void onFileInstallationFailed (const Install::File& file) override;
	void onFileInstallationCanceled (const Install::File& file) override;
	void onInstallationDone () override;
	void onRestartRequired () override;
	void updateFileInstallationProgress (const Install::File& file, double progress) override;
	void onFileInstallationPaused (const Install::File& file, bool state) override;

	// IdleClient
	void onIdleTimer () override;
};

} // namespace Packages
} // namespace CCL

#endif // _ccl_unifiedpackageinstaller_h
