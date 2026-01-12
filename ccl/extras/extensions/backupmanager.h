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
// Filename    : ccl/extras/extensions/backupmanager.h
// Description : Backup Manager
//
//************************************************************************************************

#ifndef _ccl_backupmanager_h
#define _ccl_backupmanager_h

#include "ccl/app/component.h"
#include "ccl/public/gui/commanddispatch.h"

#include "ccl/extras/extensions/icontentserver.h"

#include "ccl/base/storage/url.h"

namespace CCL {
interface IUnknownList;
class StringList;
class ListViewModel;
class DialogBox;

namespace Install {

class RestoreFilter;
struct BackupDescription;

//************************************************************************************************
// BackupConfiguration
//************************************************************************************************

class BackupConfiguration
{
public:
	// Application Settings
	void addSettingsItem (StringRef name, int type = IUrl::kFile, StringRef title = nullptr);
	bool addSettingsItem (UrlRef path, StringRef title = nullptr, bool displayOnly = false);
	void collectSettingsItems (IUnknownList& pathList) const;
	void collectSettingDescriptions (StringList& descriptions) const;
	String toRelativeSettingsPath (UrlRef path, StringRef subFolder = nullptr) const;

	// User folders
	struct UserFolder
	{
		String id;		// subfolder name used in backup .zip
		String title;	// title displayed in restore options
		Url path;		// path on disk
		SharedPtr<IUrlFilter> filter; // filter (optional)
	};
		
	const Vector<UserFolder>& getUserFolders () const { return userFolders; }
	void addUserFolder (StringRef title, UrlRef path, StringRef id = nullptr, IUrlFilter* filter = nullptr);
	bool getUserPathForID (IUrl& path, StringRef id) const;
	void addPlugInItems ();

protected:
	mutable Url settingsFolder;
	Vector<UrlWithTitle> settingsItems;
	Vector<UserFolder> userFolders;

	const Url& getSettingsFolder () const;
};

//************************************************************************************************
// BackupManager
//************************************************************************************************

class BackupManager: public Component,
					 public ComponentSingleton<BackupManager>,
					 public CommandDispatcher<BackupManager>

{
public:
	DECLARE_CLASS (BackupManager, Component)

	BackupManager ();
	~BackupManager ();

	PROPERTY_POINTER (IContentServer, contentServer, ContentServer) ///< not owned!	
	BackupConfiguration& getConfiguration (); ///< app-specific backup configuration

	void beforeInitialize (); ///< called early at application start to restore settings
	void startup (); ///< called later at application start to restore user data

	// Commands
	DECLARE_COMMANDS (BackupManager)
	DECLARE_COMMAND_CATEGORY ("Application", Component)
	bool onRun (CmdArgs);

	// Component
	tresult CCL_API initialize (IUnknown* context = nullptr) override;
	tbool CCL_API paramChanged (IParameter* param) override;
	void CCL_API notify (ISubject* subject, CCL::MessageRef msg) override;

	// Column Identifier
	DECLARE_STRINGID_MEMBER (kDescriptorID)
	DECLARE_STRINGID_MEMBER (kSourceID)
	DECLARE_STRINGID_MEMBER (kTimeID)
	DECLARE_STRINGID_MEMBER (kTimeSortingID)
	DECLARE_STRINGID_MEMBER (kSizeID)
	DECLARE_STRINGID_MEMBER (kComputerID)
	DECLARE_STRINGID_MEMBER (kAppNameID)

protected:
	static const String kBackupsFolder;
	static const String kRestoreFileName;
	static const String kAppSettingsZipFolder;

	RestoreFilter* restoreFilter;
	BackupConfiguration configuration;
	ListViewModel* restoreOptionsList;
	ListViewModel* availableBackupsList;
	DialogBox* currentDialog;
	AutoPtr<IUnknown> cachedCredentials;

	void getStartupRestoreFile (IUrl& path) const;
	bool checkRestorePending () const;
	void cancelPendingRestore ();
	bool isBackupEnabled () const;
	void updateBackupList ();
	IUnknown* requestCredentials ();
	bool backupNow ();
	bool makeBackup (BackupDescription& description, Url& backupPath);
	IFileDescriptor* getRestoreDescriptor () const;
	void updateRestoreEnabled ();
	void syncRestoreOptions ();
	void restoreNow ();
	bool restoreSettings ();
	bool restoreUserData ();

	// IObject
	tbool CCL_API getProperty (Variant& var, MemberID propertyId) const override;
};

} // namespace Install
} // namespace CCL

#endif // _ccl_backupmanager_h
