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
// Filename    : ccl/extras/extensions/backupmanager.cpp
// Description : Backup Manager
//
//************************************************************************************************

#define TEST_BACKUP (0 && DEBUG)
#define BACKUP_UPLOAD_ENABLED 1

#include "ccl/extras/extensions/backupmanager.h"

#include "ccl/app/controls/listviewmodel.h"

#include "ccl/base/message.h"
#include "ccl/base/trigger.h"
#include "ccl/base/signalsource.h"
#include "ccl/base/storage/file.h"
#include "ccl/base/storage/packageinfo.h"
#include "ccl/base/storage/settings.h"
#include "ccl/base/storage/archivehandler.h"
#include "ccl/base/storage/storableobject.h"
#include "ccl/base/collections/stringlist.h"

#include "ccl/public/gui/iparameter.h"
#include "ccl/public/gui/framework/itheme.h"
#include "ccl/public/gui/framework/dialogbox.h"
#include "ccl/public/gui/framework/ialert.h"
#include "ccl/public/gui/framework/iuserinterface.h"
#include "ccl/public/gui/framework/iprogressdialog.h"
#include "ccl/public/gui/framework/isystemshell.h"
#include "ccl/public/guiservices.h"
#include "ccl/public/plugservices.h"

#include "ccl/public/text/translation.h"
#include "ccl/public/system/cclerror.h"
#include "ccl/public/system/formatter.h"
#include "ccl/public/system/isysteminfo.h"
#include "ccl/public/system/ipackagefile.h"
#include "ccl/public/system/ipackagehandler.h"
#include "ccl/public/system/ifileutilities.h"
#include "ccl/public/plugins/versionnumber.h"
#include "ccl/public/systemservices.h"
#include "ccl/public/plugservices.h"
#include "ccl/public/cclversion.h"

#include "ccl/public/network/web/itransfermanager.h"
#include "ccl/public/netservices.h"

#include "ccl/public/app/signals.h"
#include "ccl/public/app/idocumentmetainfo.h"
#include "ccl/public/extras/ibackupitem.h"

namespace CCL {
namespace Install {

//************************************************************************************************
// BackupDescription
//************************************************************************************************

struct BackupDescription
{
	String appName;
	String computerName;
	String osName;
	DateTime timeUTC;

	static const String kPackageMarker;
	static const String kContentType;
	static const String kComputerMarker;
	static const String kOSMarker;
	static const String kTimeMarker;
	static const CStringPtr kTimeFormat;

	BackupDescription& prepare ()
	{
		appName = RootComponent::instance ().getApplicationTitle ();
		int majorVersion = VersionNumber ().scan (RootComponent::instance ().getApplicationVersion ()).major;
		if(majorVersion > 0)
			appName << " " << majorVersion;
		System::GetSystem ().getComputerName (computerName);
		osName = CCLSTR (CCL_OS_NAME);
		DateTime localTime;
		System::GetSystem ().getLocalTime (localTime);
		System::GetSystem ().convertLocalTimeToUTC (timeUTC, localTime);
		return *this;
	}

	DateTime getLocalTime () const
	{
		DateTime localTime;
		if(timeUTC != DateTime ())
			System::GetSystem ().convertUTCToLocalTime (localTime, timeUTC);
		return localTime;
	}

	void prepareInfo (PackageInfo& info)
	{
		String packageId;
		packageId << RootComponent::instance ().getApplicationID ();
		packageId << kPackageMarker;
		packageId << printTime ();
		packageId << "." CCL_PLATFORM_ID_CURRENT;

		info.setPackageID (packageId);		
		info.set (Meta::kDocumentMimeType, kContentType);
		info.set (Meta::kDocumentGenerator, RootComponent::instance ().getGeneratorName ());
		info.set ("Backup:ComputerName", computerName);
		info.set ("Backup:OSName", osName);
		info.set ("Backup:Time", Format::PortableDateTime::print (timeUTC));
	}

	String toFileName () const
	{
		return LegalFileName (String () << appName <<
							  kComputerMarker << computerName <<
							  kOSMarker << osName <<
							  kTimeMarker << printTime () <<
							  "." << FileTypes::Zip ().getExtension ());
	}

	void parseFromFileName (StringRef fileName)
	{
		// appName{computer marker}computer{OS maker}system{time marker}time.zip
		int computerIndex = fileName.index (kComputerMarker);
		int osIndex = fileName.index (kOSMarker);
		int timeIndex = fileName.index (kTimeMarker);
		int extensionIndex = fileName.lastIndex (CCLSTR ("."));

		auto fileNameFromTo = [&fileName] (int fromIndex, int toIndex)
		{
			return fileName.subString (fromIndex, toIndex - fromIndex);
		};

		appName = fileNameFromTo (0, computerIndex);
		computerName = fileNameFromTo (computerIndex + kComputerMarker.length (), osIndex);
		osName = fileNameFromTo (osIndex + kOSMarker.length (), timeIndex);
		String timeString = fileNameFromTo (timeIndex + kTimeMarker.length (), extensionIndex);
		scanTime (timeString);
	}

	String printTime () const
	{
		auto& time = timeUTC;
		MutableCString string;
		string.appendFormat (kTimeFormat, 
			time.getDate ().getYear (), time.getDate ().getMonth (), time.getDate ().getDay (),
			time.getTime ().getHour (), time.getTime ().getMinute (), time.getTime ().getSecond ());
		return String (string);
	}

	bool scanTime (StringRef timeString)
	{
		int year = 0, month = 0, day = 0, hour = 0, minute = 0, second = 0;
		MutableCString string (timeString);
		if(::sscanf (string, kTimeFormat, &year, &month, &day, &hour, &minute, &second) != 6)
			return false;

		auto& time = timeUTC;
		time.setDate (Date (year, month, day));
		time.setTime (Time (hour, minute, second));
		return true;
	}
};

const String BackupDescription::kPackageMarker (".backup-package.");
const String BackupDescription::kContentType (CCL_MIME_TYPE "-backup-package");
const String BackupDescription::kComputerMarker ("_@_");
const String BackupDescription::kOSMarker ("_@OS_");
const String BackupDescription::kTimeMarker ("_@T_");
const CStringPtr BackupDescription::kTimeFormat = "%04d%02d%02d-%02d%02d%02d";

//************************************************************************************************
// RestoreFilter
//************************************************************************************************

class RestoreFilter: public StorableObject
{
public:
	RestoreFilter ();

	PROPERTY_OBJECT (Url, pathToBackup, PathToBackup)
	IPackageFile* getBackupFile ();
	
	PROPERTY_BOOL (restoreSettingsEnabled, RestoreSettingsEnabled)
	PROPERTY_BOOL (restoreSettingsFailed, RestoreSettingsFailed)

	struct Rule
	{
		String sourceId;
		Url targetFolder;
	};

	const Vector<Rule> getRules () const { return rules; }
	void addRule (StringRef sourceId, UrlRef targetFolder);

	// StorableObject
	bool load (const Storage& storage) override;
	bool save (const Storage& storage) const override;

protected:
	Vector<Rule> rules;
	AutoPtr<IPackageFile> backupFile;
};

//************************************************************************************************
// BackupUploadFinalizer
//************************************************************************************************

class BackupUploadFinalizer: public TriggerAction
{
public:
	// TriggerAction
	void CCL_API execute (IObject* target) override;
};

//************************************************************************************************
// RestoreAction
//************************************************************************************************

class RestoreAction: public TriggerAction
{
public:
	PROPERTY_OBJECT (Url, restoreFilePath, RestoreFilePath)
	PROPERTY_SHARED_AUTO (RestoreFilter, restoreFilter, RestoreFilter)

	// TriggerAction
	void CCL_API execute (IObject* target) override;
};

//************************************************************************************************
// BackupListSorter
//************************************************************************************************

namespace BackupListSorter
{
	DEFINE_ARRAY_COMPARE (sortByTimeLatestFirst, ListViewItem, item1, item2)
		int64 t1 = item1->getDetails ().getInt64 (BackupManager::kTimeSortingID);
		int64 t2 = item2->getDetails ().getInt64 (BackupManager::kTimeSortingID);
		return static_cast<int> (t2 - t1); // latest first
	}
}

} // namespace Install
} // namespace CCL

using namespace CCL;
using namespace Install;

//////////////////////////////////////////////////////////////////////////////////////////////////
// Strings
//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_XSTRINGS ("BackupManager")
	XSTRING (BackupOperation, "Backup")
	XSTRING (RestoreOperation, "Restore")
	XSTRING (BackupFailed, "Failed to create backup.")
	XSTRING (BackupTooLarge, "Backup is too large.")
	XSTRING (RestoreFailed, "Failed to restore from backup.")
	XSTRING (BackupRestoreSucceeded, "Backup restored successfully.")
	XSTRING (RestoreOnNextStart, "Backup will be restored next time you start $APPNAME.")
	XSTRING (NoBackupsFound, "No backups found.")
	XSTRING (BackupUploadSucceeded, "Backup uploaded successfully.")
	XSTRING (BackupUploadFailed, "Failed to upload backup.")
	XSTRING (AppSettings, "Program Settings")
	XSTRING (Unknown, "Unknown")
	XSTRING (Never, "Never")
	XSTRING (TimeColumn, "Date & Time")
	XSTRING (SizeColumn, "Size")
	XSTRING (ComputerColumn, "Computer")
	XSTRING (AppColumn, "Version")
END_XSTRINGS

//////////////////////////////////////////////////////////////////////////////////////////////////
// Tags
//////////////////////////////////////////////////////////////////////////////////////////////////

namespace Tag
{
	enum BackupManagerTags
	{
		kBackupNow = 100,
		kRestoreNow,
		kCancelRestore,
		kUpdateBackupList,
		kLastBackupTime
	};
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// Commands
//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_COMMANDS (BackupManager)
	DEFINE_COMMAND ("Application", "Backup and Restore", BackupManager::onRun)
END_COMMANDS (BackupManager)

//************************************************************************************************
// BackupUploadFinalizer
//************************************************************************************************

void CCL_API BackupUploadFinalizer::execute (IObject* target)
{
	UnknownPtr<Web::ITransfer> transfer (target);
	ASSERT (transfer)
	if(!transfer)
		return;

	if(transfer->getState () == Web::ITransfer::kCompleted)
		Alert::notify (XSTR (BackupUploadSucceeded), Alert::kInformation);
	else if(transfer->getState () == Web::ITransfer::kFailed)
		Alert::notify (XSTR (BackupUploadFailed), Alert::kError);

	// remove transfer
	System::GetTransferManager ().remove (transfer);

	// delete local file
	#if 1//RELEASE
	bool removed = File (transfer->getSrcLocation ()).remove ();
	SOFT_ASSERT (removed, "Failed to remove local backup file!\n")
	#endif
}

//************************************************************************************************
// RestoreAction
//************************************************************************************************

void CCL_API RestoreAction::execute (IObject* target)
{
	UnknownPtr<Web::ITransfer> transfer (target);
	ASSERT (transfer)
	if(!transfer)
		return;

	bool succeeded = false;
	if(transfer->getState () == Web::ITransfer::kCompleted)
	{
		#if 1//RELEASE
		bool fileValid = transfer->getDstLocation ().getFileType () == FileTypes::Zip ();
		#else
		bool fileValid = true;
		#endif

		ASSERT (restoreFilter != nullptr)
		restoreFilter->setPathToBackup (Url (transfer->getDstLocation ()));
		if(fileValid && restoreFilter->saveToFile (restoreFilePath))
		{
			succeeded = true;
			SignalSource (Signals::kApplication).deferSignal (NEW Message (Signals::kRequestRestart, XSTR (RestoreOnNextStart)));
		}
	}

	if(succeeded == false)
		Alert::notify (XSTR (RestoreFailed), Alert::kError);

	// remove transfer
	System::GetTransferManager ().remove (transfer);
}

//************************************************************************************************
// RestoreFilter
//************************************************************************************************

RestoreFilter::RestoreFilter ()
: restoreSettingsEnabled (false),
  restoreSettingsFailed (false)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

IPackageFile* RestoreFilter::getBackupFile ()
{
	if(!backupFile)
		backupFile = System::GetPackageHandler ().openPackage (pathToBackup);
	return backupFile;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void RestoreFilter::addRule (StringRef sourceId, UrlRef targetFolder)
{
	rules.add ({sourceId, targetFolder});
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool RestoreFilter::load (const Storage& storage)
{
	Attributes& a = storage.getAttributes ();
	restoreSettingsEnabled = a.getBool ("restoreSettingsEnabled");
	if(Url* savedPath = a.getObject<Url> ("pathToBackup"))
		pathToBackup.assign (*savedPath);
	IterForEach (a.newQueueIterator ("rules", ccl_typeid<Attributes> ()), Attributes, ruleAttr)
		String sourceId = ruleAttr->getString ("sourceId");
		Url* savedPath = ruleAttr->getObject<Url> ("targetFolder");
		if(!sourceId.isEmpty () && savedPath)
			addRule (sourceId, *savedPath);
	EndFor
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool RestoreFilter::save (const Storage& storage) const
{
	Attributes& a = storage.getAttributes ();
	a.set ("restoreSettingsEnabled", restoreSettingsEnabled);
	a.set ("pathToBackup", pathToBackup.clone (), Attributes::kOwns);
	for(auto& rule : rules)
	{
		Attributes* ruleAttr = NEW Attributes;
		ruleAttr->set ("sourceId", rule.sourceId);
		ruleAttr->set ("targetFolder", rule.targetFolder.clone (), Attributes::kOwns);
		a.queue ("rules", ruleAttr, Attributes::kOwns);
	}
	return true;
}

//************************************************************************************************
// BackupConfiguration
//************************************************************************************************

const Url& BackupConfiguration::getSettingsFolder () const
{
	if(settingsFolder.isEmpty ())
		System::GetSystem ().getLocation (settingsFolder, System::kAppSettingsFolder);
	return settingsFolder;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void BackupConfiguration::addSettingsItem (StringRef name, int type, StringRef title)
{
	UrlWithTitle path (getSettingsFolder (), title);
	path.descend (name, type);
	settingsItems.add (path);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool BackupConfiguration::addSettingsItem (UrlRef path, StringRef title, bool displayOnly)
{
	ASSERT (!displayOnly || path.isEmpty ())
	if(!displayOnly && !getSettingsFolder ().contains (path))
		return false;
	
	settingsItems.add (UrlWithTitle (path, title));
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void BackupConfiguration::collectSettingsItems (IUnknownList& pathList) const
{
	for(auto& path : settingsItems)
		if(!path.isEmpty ())
			pathList.add (path.clone ()->asUnknown ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void BackupConfiguration::collectSettingDescriptions (StringList& descriptions) const
{
	for(auto& path : settingsItems)
		if(!path.getTitle ().isEmpty ())
			descriptions.add (path.getTitle ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

String BackupConfiguration::toRelativeSettingsPath (UrlRef path, StringRef subFolder) const
{
	if(!getSettingsFolder ().contains (path))
		return String ();

	Url relativePath (path);
	relativePath.makeRelative (getSettingsFolder ());
	String pathString (relativePath.getPath ());
	if(pathString.startsWith (CCLSTR ("./")))
		pathString.remove (0, 2);

	if(!subFolder.isEmpty ())
	{
		pathString.prepend (Url::strPathChar);
		pathString.prepend (subFolder);
	}
	return pathString;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void BackupConfiguration::addUserFolder (StringRef title, UrlRef path, StringRef _id, IUrlFilter* filter)
{
	String id (_id);
	if(id.isEmpty ())
		path.getName (id);
	
	userFolders.add ({id, title, path, filter});
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool BackupConfiguration::getUserPathForID (IUrl& path, StringRef id) const
{
	for(auto& folder : userFolders)
		if(folder.id == id)
		{
			path.assign (folder.path);
			return true;
		}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void BackupConfiguration::addPlugInItems ()
{
	ForEachPlugInClass (PLUG_CATEGORY_BACKUPITEM, description)
		IBackupItem* backupItem = ccl_new<IBackupItem> (description.getClassID ());
		ASSERT (backupItem != nullptr)
		if(backupItem)
		{
			Url path;
			String title;
			backupItem->getUserFolder (title, path);
			addUserFolder (title, path);
			ccl_release (backupItem);
		}
	EndFor
}

//************************************************************************************************
// BackupManager
//************************************************************************************************

const String BackupManager::kBackupsFolder ("Backups");
const String BackupManager::kRestoreFileName ("restore-backup.xml");
const String BackupManager::kAppSettingsZipFolder ("($AppSettings)");

//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_CLASS_HIDDEN (BackupManager, Component)
DEFINE_COMPONENT_SINGLETON (BackupManager)
DEFINE_STRINGID_MEMBER_ (BackupManager, kDescriptorID, "descriptor")
DEFINE_STRINGID_MEMBER_ (BackupManager, kSourceID, "source")
DEFINE_STRINGID_MEMBER_ (BackupManager, kTimeID, "time")
DEFINE_STRINGID_MEMBER_ (BackupManager, kTimeSortingID, "timeSorting")
DEFINE_STRINGID_MEMBER_ (BackupManager, kSizeID, "size")
DEFINE_STRINGID_MEMBER_ (BackupManager, kComputerID, "computer")
DEFINE_STRINGID_MEMBER_ (BackupManager, kAppNameID, "appname")

//////////////////////////////////////////////////////////////////////////////////////////////////

BackupManager::BackupManager ()
: Component ("BackupManager"),
  contentServer (nullptr),
  restoreFilter (nullptr),
  restoreOptionsList (nullptr),
  availableBackupsList (nullptr),
  currentDialog (nullptr)
{
	// Note that translations haven't been loaded here - see initialize().

	restoreOptionsList = NEW ListViewModel;
	restoreOptionsList->addObserver (this);
	addObject ("restoreOptionsList", restoreOptionsList);

	availableBackupsList = NEW ListViewModel;
	availableBackupsList->addObserver (this);
	addObject ("availableBackupsList", availableBackupsList);

	paramList.addParam ("backupNow", Tag::kBackupNow)->enable (false);
	paramList.addParam ("restoreNow", Tag::kRestoreNow)->enable (false);
	paramList.addParam ("cancelRestore", Tag::kCancelRestore);
	paramList.addParam ("updateBackupList", Tag::kUpdateBackupList);
	paramList.addString ("lastBackupTime", Tag::kLastBackupTime);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

BackupManager::~BackupManager ()
{
	ASSERT (restoreFilter == nullptr)
	safe_release (restoreFilter);

	restoreOptionsList->removeObserver (this);
	safe_release (restoreOptionsList);

	availableBackupsList->removeObserver (this);
	safe_release (availableBackupsList);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

BackupConfiguration& BackupManager::getConfiguration ()
{
	return configuration; 
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void BackupManager::getStartupRestoreFile (IUrl& path) const
{
	System::GetSystem().getLocation (path, System::kAppSettingsFolder);
	path.descend (kRestoreFileName);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool BackupManager::checkRestorePending () const
{
	Url path;
	getStartupRestoreFile (path);
	return File (path).exists ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void BackupManager::cancelPendingRestore ()
{
	Url path;
	getStartupRestoreFile (path);
	if(File (path).exists ())
	{
		RestoreFilter filter;
		if(filter.loadFromFile (path))
			File (filter.getPathToBackup ()).remove ();

		File (path).remove ();
	}

	if(currentDialog)
		(*currentDialog)->close ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void BackupManager::beforeInitialize ()
{
	Url path;
	getStartupRestoreFile (path);
	if(File (path).exists ())
	{
		ASSERT (restoreFilter == nullptr)
		restoreFilter = NEW RestoreFilter;
		if(!restoreFilter->loadFromFile (path))
		{
			safe_release (restoreFilter);
			CCL_WARN ("Failed to load restore filter!", 0)
			return;
		}

		if(restoreFilter->isRestoreSettingsEnabled ())
			if(!restoreSettings ())
				restoreFilter->setRestoreSettingsFailed (true); // warn later
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void BackupManager::startup ()
{
	if(restoreFilter)
	{
		ErrorContextGuard errorContext;
		bool dataRestored = restoreUserData ();

		Url path;
		getStartupRestoreFile (path);
		bool filterRemoved = File (path).remove ();

		if(!dataRestored || restoreFilter->isRestoreSettingsFailed () || !filterRemoved)
			Alert::errorWithContext (XSTR (RestoreFailed), true);
		else
			Alert::notify (XSTR (BackupRestoreSucceeded));

		// remove backup archive
		File (restoreFilter->getPathToBackup ()).remove ();

		safe_release (restoreFilter);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API BackupManager::initialize (IUnknown* context)
{
	// finish construction as translations aren't available in ctor
	restoreOptionsList->getColumns ().addColumn (20, "", ListViewModel::kCheckBoxID);
	restoreOptionsList->getColumns ().addColumn (200, "", ListViewModel::kTitleID);

	availableBackupsList->getColumns ().addColumn (150, XSTR (TimeColumn), kTimeID);
	availableBackupsList->getColumns ().addColumn (100, XSTR (SizeColumn), kSizeID);
	availableBackupsList->getColumns ().addColumn (150, XSTR (ComputerColumn), kComputerID, 150, IColumnHeaderList::kSizable);
	availableBackupsList->getColumns ().addColumn (100, XSTR (AppColumn), kAppNameID);
	
	ListViewSorter* timeSorter = NEW ListViewSorter (kTimeID, XSTR (TimeColumn), &BackupListSorter::sortByTimeLatestFirst);
	availableBackupsList->addSorter (timeSorter);
	availableBackupsList->sortBy (timeSorter);

	return SuperClass::initialize (context);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IMPLEMENT_COMMANDS (BackupManager, Component)

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API BackupManager::paramChanged (IParameter* param)
{
	switch(param->getTag ())
	{
	case Tag::kBackupNow :
		backupNow ();
		break;

	case Tag::kUpdateBackupList :
		updateBackupList ();
		break;

	case Tag::kRestoreNow :
		restoreNow ();
		break;

	case Tag::kCancelRestore :
		cancelPendingRestore ();
		break;
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API BackupManager::notify (ISubject* subject, MessageRef msg)
{
	if(msg == IItemView::kSelectionChanged)
		updateRestoreEnabled ();
	else if(msg == ListViewModel::kItemChecked)
		syncRestoreOptions ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool BackupManager::onRun (CmdArgs args)
{
	if(!args.checkOnly ())
	{
		// update list of backups from server
		updateBackupList ();

		// check if backup feature is available
		paramList.byTag (Tag::kBackupNow)->enable (isBackupEnabled ());
		
		// rebuild restore options list
		auto addRestoreOption = [&] (StringRef sourceId, StringRef title, bool enabled = true)
		{
			auto item = NEW ListViewItem (title);
			item->getDetails ().set (kSourceID, sourceId);
			item->setChecked (true);
			item->setEnabled (enabled);
			restoreOptionsList->addItem (item);
		};

		restoreOptionsList->removeAll ();
		addRestoreOption (kAppSettingsZipFolder, XSTR (AppSettings));
		
		StringList descriptions;
		configuration.collectSettingDescriptions (descriptions);
		ForEach (descriptions, Boxed::String, string)
			addRestoreOption (String () << kAppSettingsZipFolder << "+", String () << "   " << (*string), false);
		EndFor

		for(auto folder : configuration.getUserFolders ())
			addRestoreOption (folder.id, folder.title);

		updateRestoreEnabled ();

		// run dialog
		if(IView* view = getTheme ()->createView ("BackupManager", asUnknown ()))
		{
			DialogBox dialogBox;
			ScopedVar<DialogBox*> scope (currentDialog, &dialogBox);
			dialogBox->runDialog (view);
		}

		// discard cached credentials
		cachedCredentials.release ();
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool BackupManager::isBackupEnabled () const
{
	#if 1//RELEASE
	bool backupEnabled = contentServer && contentServer->isUserBackupFeatureAvailable ();
	return backupEnabled;
	#else
	return true;
	#endif
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void BackupManager::updateBackupList ()
{
	UnknownList backupsOnServer;
	#if 1//RELEASE
	ASSERT (contentServer != nullptr)
	if(contentServer && isBackupEnabled ())
		contentServer->requestUserBackupList (backupsOnServer, IContentServer::kSuppressErrors);
	#endif

	#if TEST_BACKUP
	if(backupsOnServer.isEmpty ())
	{
		auto descriptor = NEW FileDescriptor (BackupDescription ().prepare ().toFileName (), 4 * 1024 * 1024);
		backupsOnServer.add (descriptor->asUnknown ());
	}
	#endif

	DateTime lastBackupTime;
	availableBackupsList->removeAll ();
	ForEachUnknown (backupsOnServer, unk)
		if(UnknownPtr<IFileDescriptor> descriptor = unk)
		{
			String fileName;
			descriptor->getFileName (fileName);

			BackupDescription description;
			description.parseFromFileName (fileName);

			DateTime time (description.getLocalTime ());
			bool timeValid = time != DateTime ();
			if(timeValid && time > lastBackupTime)
				lastBackupTime = time;

			int64 fileSize = 0;
			descriptor->getFileSize (fileSize);

			auto listItem = NEW ListViewItem;
			auto& details = listItem->getDetails ();
			details.set (kDescriptorID, descriptor, Attributes::kShare);
			if(timeValid)
				details.set (kTimeID, Format::DateTime::print (time));
			else
				details.set (kTimeID, XSTR (Unknown));
			details.set (kTimeSortingID, time.toOrdinal ());
			if(fileSize > 0)
				details.set (kSizeID, Format::ByteSize::print (fileSize));

			String computerString (description.computerName);
			if(!description.osName.isEmpty ())
			{
				if(!computerString.isEmpty ())
					computerString << " (" << description.osName << ")";
				else
					computerString = description.osName;
			}

			details.set (kComputerID, computerString);
			details.set (kAppNameID, description.appName);
			availableBackupsList->addSorted (listItem);
		}
	EndFor

	if(availableBackupsList->isEmpty ())
	{
		auto listItem = NEW ListViewItem;
		listItem->getDetails ().set (kTimeID, XSTR (NoBackupsFound));
		listItem->setEnabled (false);
		availableBackupsList->addItem (listItem);
	}

	availableBackupsList->signal (Message (kChanged));

	String timeString;
	if(lastBackupTime != DateTime ())
		timeString << Format::TimeAgo::print (lastBackupTime) << " (" << Format::DateTime::print (lastBackupTime) << ")";
	else
		timeString = XSTR (Never);
	paramList.byTag (Tag::kLastBackupTime)->fromString (timeString);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IUnknown* BackupManager::requestCredentials ()
{
	if(!cachedCredentials && contentServer)
		cachedCredentials = contentServer->requestCredentials (IContentServer::kContentDownload);
	return cachedCredentials;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool BackupManager::backupNow ()
{
	// make sure everything's saved properly
	{ 
		WaitCursor waitCursor (System::GetGUI ());
		Settings::autoSaveAll ();
	}
	
	Url backupPath;
	BackupDescription description;
	ErrorContextGuard errorContext;
	bool succeeded = makeBackup (description, backupPath);
	
	#if BACKUP_UPLOAD_ENABLED
	// check file size limit
	int64 sizeLimit = contentServer ? contentServer->getMaximumBackupFileSize () : -1;
	if(succeeded && sizeLimit > 0)
	{
		FileInfo info;
		File (backupPath).getInfo (info);
		if(info.fileSize > sizeLimit)
		{
			Alert::error (XSTR (BackupTooLarge));
			File (backupPath).remove ();
			return false;
		}
	}

	if(succeeded == true)
	{		
		AutoPtr<Web::ITransfer> transfer;
		if(auto credentials = requestCredentials ())
			transfer = contentServer->createUploadForBackup (backupPath, credentials);
		
		if(transfer)
		{
			transfer->addFinalizer (NEW BackupUploadFinalizer);			
			System::GetTransferManager ().queue (transfer, Web::ITransferManager::kNonSimultaneous);
		}
		else
			succeeded = false;
	}
	#else
	System::GetSystemShell ().showFile (backupPath);
	#endif

	if(succeeded == false)
	{
		Alert::errorWithContext (XSTR (BackupFailed), true);
		return false;
	}

	if(currentDialog)
		(*currentDialog)->close ();
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool BackupManager::makeBackup (BackupDescription& description, Url& backupPath)
{
	description.prepare ();	

	System::GetSystem ().getLocation (backupPath, System::kUserContentFolder);
	backupPath.descend (kBackupsFolder, Url::kFolder);
	backupPath.descend (description.toFileName ());
	backupPath.makeUnique ();

	AutoPtr<IPackageFile> packageFile = System::GetPackageHandler ().createPackage (backupPath, ClassID::ZipFile);
	packageFile->setOption (PackageOption::kCompressed, true);
	packageFile->setOption (PackageOption::kDetailedProgressEnabled, true);
	if(!packageFile->create ())
		return false;

	ASSERT (packageFile->getFileSystem () != nullptr)
	ArchiveHandler archiveHandler (*packageFile->getFileSystem ());
	
	// exclude some (potentially harmful) file types from backup
	AutoPtr<FileTypeExcludeFilter> fileFilter = NEW FileTypeExcludeFilter;
	fileFilter->addFileType (FileTypes::App ());
	fileFilter->addFileType (FileTypes::Module ());
	fileFilter->addFileType (FileTypes::Zip ()); // suppress nesting
	fileFilter->addFileType (FileTypes::Package ());
	if(const auto jsFileType = System::GetFileTypeRegistry ().getFileTypeByMimeType (String (Scripting::kJavaScript)))
		fileFilter->addFileType (*jsFileType);

	int fileIteratorMode = IFileIterator::kAll|IFileIterator::kIgnoreHidden; // exclude hidden files

	AutoPtr<IProgressNotify> progress = ccl_new<IProgressNotify> (ClassID::ProgressDialog);
	UnknownPtr<IProgressDialog> (progress)->constrainLevels (2, 2);
	progress->setTitle (XSTR (BackupOperation));
	ProgressNotifyScope progressScope (progress);

	// Meta Information
	PackageInfo metaInfo;
	description.prepareInfo (metaInfo);
	metaInfo.saveWithHandler (archiveHandler);
	
	// Application Settings
	UnknownList settingsFiles;
	Settings::backupAll (settingsFiles); // collect files via signal
	configuration.collectSettingsItems (settingsFiles); // collect manually added files

	ForEachUnknown (settingsFiles, unk)
		if(UnknownPtr<IUrl> pathOnDisk = unk)
		{
			String pathString = configuration.toRelativeSettingsPath (*pathOnDisk, kAppSettingsZipFolder);
			if(pathString.isEmpty ())
				continue;

			if(pathOnDisk->isFile ())
			{
				AutoPtr<IStream> data = File::loadBinaryFile (*pathOnDisk);
				if(data)
					archiveHandler.addSaveTask (pathString, *data);
			}
			else
			{
				Url dstPath;
				dstPath.setPath (pathString, Url::kFolder);
				packageFile->embeddToFolder (dstPath, *pathOnDisk, fileIteratorMode, fileFilter, progress);
			}
		}
	EndFor

	class CombinedFileFilter: public UrlFilter
	{
		Vector<IUrlFilter*> filters;
	public:
		void addFilter (IUrlFilter* filter) { filters.add (filter); }
		tbool CCL_API matches (UrlRef url) const override
		{
			for(auto filter : filters)
				if(!filter->matches (url))
					return false;
			return true;				
		}
	};

	// User data folders
	for(auto folder : configuration.getUserFolders ())
	{
		Url dstPath;
		dstPath.setPath (folder.id, Url::kFolder);

		CombinedFileFilter combinedFilter;
		combinedFilter.addFilter (fileFilter);
		if(folder.filter)
			combinedFilter.addFilter (folder.filter);

		packageFile->embeddToFolder (dstPath, folder.path, fileIteratorMode, &combinedFilter, progress);
	}

	bool result = packageFile->flush (progress) && 
				  packageFile->close ();
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IFileDescriptor* BackupManager::getRestoreDescriptor () const
{
	auto item = availableBackupsList->getFirstSelectedItem ();
	//if(!item && !availableBackupsList->isEmpty ())
	//	item = availableBackupsList->getItem (0);
	if(!item)
		return nullptr;

	return UnknownPtr<IFileDescriptor> (item->getDetails ().getUnknown (kDescriptorID));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void BackupManager::updateRestoreEnabled ()
{
	auto descriptor = getRestoreDescriptor ();
	bool pending = checkRestorePending ();
	paramList.byTag (Tag::kRestoreNow)->enable (descriptor != nullptr && pending == false);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void BackupManager::syncRestoreOptions ()
{
	bool first = true, appSettingsChecked = false, changed = false;
	ForEach (*restoreOptionsList, ListViewItem, item)
		String sourceId = item->getDetails ().getString (kSourceID);
		if(first)
		{
			ASSERT (sourceId == kAppSettingsZipFolder)
			appSettingsChecked = item->isChecked ();
			first = false;
		}
		else
		{
			if(sourceId.startsWith (kAppSettingsZipFolder))
				if(item->isChecked () != appSettingsChecked)
				{
					item->setChecked (appSettingsChecked);
					changed = true;
				}
		}
	EndFor

	if(changed)
		restoreOptionsList->invalidate ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void BackupManager::restoreNow ()
{
	auto descriptor = getRestoreDescriptor ();
	auto credentials = requestCredentials ();
	if(!descriptor || !credentials)
		return;

	AutoPtr<RestoreFilter> filter = NEW RestoreFilter;
	ForEach (*restoreOptionsList, ListViewItem, item)
		if(!item->isChecked ())
			continue;

		String sourceId = item->getDetails ().getString (kSourceID);
		if(sourceId == kAppSettingsZipFolder)
			filter->setRestoreSettingsEnabled (true);
		else if(!sourceId.startsWith (kAppSettingsZipFolder) && item->isChecked ())
		{
			Url targetFolder;
			if(configuration.getUserPathForID (targetFolder, sourceId))
				filter->addRule (sourceId, targetFolder);
		}
	EndFor

	String fileName;
	descriptor->getFileName (fileName);

	Url dstPath;
	System::GetSystem ().getLocation (dstPath, System::kUserContentFolder);
	dstPath.descend (kBackupsFolder, Url::kFolder);	
	dstPath.descend (LegalFileName (fileName));
	dstPath.makeUnique ();

	Url url;
	contentServer->getBackupURL (url, *descriptor, credentials);
	AutoPtr<Web::ITransfer> transfer = System::GetTransferManager ().createTransfer (dstPath, url, Web::ITransfer::kDownload);
	transfer->setSrcDisplayString (contentServer->getServerTitle ());

	RestoreAction* restoreAction = NEW RestoreAction;
	Url restoreFilePath;
	getStartupRestoreFile (restoreFilePath);
	restoreAction->setRestoreFilePath (restoreFilePath);
	restoreAction->setRestoreFilter (filter);
	transfer->addFinalizer (restoreAction);

	System::GetTransferManager ().queue (transfer, Web::ITransferManager::kNonSimultaneous);

	if(currentDialog)
		(*currentDialog)->close ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool BackupManager::restoreSettings ()
{
	WaitCursor waitCursor (System::GetGUI ());

	IPackageFile* packageFile = restoreFilter ? restoreFilter->getBackupFile () : nullptr;
	if(!packageFile)
		return false;
	
	Url dstPath;
	System::GetSystem().getLocation (dstPath, System::kAppSettingsFolder);
	Url srcPath;
	srcPath.setPath (kAppSettingsZipFolder, Url::kFolder);
	packageFile->extractFolder (srcPath, dstPath, true);

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool BackupManager::restoreUserData ()
{
	IPackageFile* packageFile = restoreFilter ? restoreFilter->getBackupFile () : nullptr;
	if(!packageFile)
		return false;

	AutoPtr<IProgressNotify> progress = ccl_new<IProgressNotify> (ClassID::ProgressDialog);
	UnknownPtr<IProgressDialog> (progress)->constrainLevels (2, 2);
	progress->setTitle (XSTR (RestoreOperation));
	ProgressNotifyScope progressScope (progress);

	for(auto& rule : restoreFilter->getRules ())
	{
		Url srcPath;
		srcPath.setPath (rule.sourceId, Url::kFolder);
		packageFile->extractFolder (srcPath, rule.targetFolder, true, nullptr, progress);
	}

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API BackupManager::getProperty (Variant& var, MemberID propertyId) const
{
	if(propertyId == "backupEnabled")
	{
		var = isBackupEnabled ();
		return true;
	}
	if(propertyId == "restorePending")
	{
		var = checkRestorePending ();
		return true;
	}
	return SuperClass::getProperty (var, propertyId);
}
