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
// Filename    : ccl/extras/extensions/appupdater.cpp
// Description : Application Updater
//
//************************************************************************************************

#define TEST_DOWNLOAD (0 && DEBUG)

#include "ccl/extras/extensions/appupdater.h"

#include "ccl/base/message.h"
#include "ccl/base/signalsource.h"
#include "ccl/base/boxedtypes.h"
#include "ccl/base/storage/configuration.h"
#include "ccl/base/storage/attributes.h"

#include "ccl/app/utilities/batchoperation.h"
#include "ccl/public/app/signals.h"

#include "ccl/public/text/translation.h"
#include "ccl/public/base/itrigger.h"
#include "ccl/public/plugins/versionnumber.h"
#include "ccl/public/system/isysteminfo.h"
#include "ccl/public/system/inativefilesystem.h"
#include "ccl/public/systemservices.h"

#include "ccl/public/gui/framework/ialert.h"
#include "ccl/public/gui/framework/isystemshell.h"
#include "ccl/public/guiservices.h"

#include "ccl/public/network/web/iwebcredentials.h"
#include "ccl/public/network/web/itransfermanager.h"
#include "ccl/public/netservices.h"

namespace CCL {
namespace Install {

//************************************************************************************************
// ApplicationUpdater::UpdateTask
//************************************************************************************************

class ApplicationUpdater::UpdateTask: public BatchOperation::Task,
									  public IUpdateCheckResult
{
public:
	UpdateTask ()
	{
		//hasProgressInfo (false);
	}

	void setDefinition (const ContentDefinition& definition);
	void setErrorText (StringRef errorText);
	void setResponse (StringRef response);
	void setAction (StringRef versionString);

	PROPERTY_SHARED_AUTO (IUnknown, credentials, Credentials)

	// IUpdateCheckResult
	const ContentDefinition& getDefinition () const override { return definition; }
	const VersionNumber& getCurrentVersion () const override { return currentVersion; }
	StringRef getErrorText () const override { return errorText; }
	tbool CCL_API getAction (IUrl& url, String& title) const override { url = actionUrl; title = actionTitle; return true; }

	// BatchOperation::Task
	String getProgressText () override;
	IAsyncOperation* performAsync () override;
	void abort () override;
	void onCanceled () override;

	CLASS_INTERFACE (IUpdateCheckResult, Task)

protected:
	ContentDefinition definition;
	VersionNumber currentVersion;
	String errorText;
	Url actionUrl;
	String actionTitle;

	SharedPtr<IAsyncOperation> pendingOperation;

	void onCompletion (IAsyncOperation& operation)
	{
		setResponse (operation.getResult ().asString ());
		AsyncOperation::deferDestruction (pendingOperation.detach ());
	}
};

//************************************************************************************************
// ApplicationUpdater::UpdateFinalizer
//************************************************************************************************

class ApplicationUpdater::UpdateFinalizer: public Object,
										   public ITriggerAction
{
public:
	UpdateFinalizer (const VersionNumber& version)
	: version (version)
	{}

	PROPERTY_OBJECT (VersionNumber, version, Version)

	// ITriggerAction
	void CCL_API execute (IObject* target) override;

	CLASS_INTERFACE (ITriggerAction, Object)
};

//************************************************************************************************
// ApplicationUpdater::ReleaseNotesFinalizer
//************************************************************************************************

class ApplicationUpdater::ReleaseNotesFinalizer: public Object,
												 public ITriggerAction
{
public:
	// ITriggerAction
	void CCL_API execute (IObject* target) override;

	CLASS_INTERFACE (ITriggerAction, Object)
};

} // namespace Install
} // namespace CCL

using namespace CCL;
using namespace Install;

//////////////////////////////////////////////////////////////////////////////////////////////////
// Strings
//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_XSTRINGS ("Updater")
	XSTRING (CheckForUpdates, "Check for Updates")
	XSTRING (VersionCheck, "Checking version of %(1)")
	XSTRING (LocalVersion, "Installed version: %(1)")
	XSTRING (ServerVersion, "Current version: %(1)")
	XSTRING (VersionNotFound, "The version could not be identified!")
	XSTRING (CheckFailed, "Check for Updates failed!")
	XSTRING (UpToDate, "Your version is up-to-date.")
	XSTRING (NewVersion, "There is a new version available.")
	XSTRING (UpdateNow, "Update Now")
	XSTRING (ViewReleaseNotes, "View Release Notes")
END_XSTRINGS

//////////////////////////////////////////////////////////////////////////////////////////////////
// Commands
//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_COMMANDS (ApplicationUpdater)
	DEFINE_COMMAND_ARGS ("Help", "Check for Updates", ApplicationUpdater::onCheckUpdates, 0, "Cached")
	DEFINE_COMMAND_("Help", "Update Now", ApplicationUpdater::onUpdateNow, CommandFlags::kHidden)
	DEFINE_COMMAND ("Help", "View Release Notes", ApplicationUpdater::onViewReleaseNotes)
END_COMMANDS (ApplicationUpdater)

//************************************************************************************************
// IUpdateCheckResult
//************************************************************************************************

DEFINE_IID_ (IUpdateCheckResult, 0x4419e224, 0x10e9, 0x426a, 0x89, 0xeb, 0xb, 0xdc, 0x67, 0x51, 0x42, 0x78)

//////////////////////////////////////////////////////////////////////////////////////////////////

IUpdateCheckResult* IUpdateCheckResult::getFirst (VariantRef result)
{
	if(UnknownPtr<IUpdateCheckResult> checkResult = result.asUnknown ())
		return checkResult;

	if(UnknownPtr<IContainer> container = result.asUnknown ())
		ForEachUnknown (*container, unk)
			if(UnknownPtr<IUpdateCheckResult> checkResult = unk)
				return checkResult;
		EndFor
	return nullptr;
}

//************************************************************************************************
// ApplicationUpdater::UpdateTask
//************************************************************************************************

String ApplicationUpdater::UpdateTask::getProgressText ()
{
	return String ().appendFormat (XSTR (VersionCheck), definition.title);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IAsyncOperation* ApplicationUpdater::UpdateTask::performAsync ()
{
	Promise p = ApplicationUpdater::instance ().getContentServer ()->getContentVersion (definition, credentials);
	pendingOperation = p; // keep first operation alive!
	return return_shared<IAsyncOperation> (p.then (this, &UpdateTask::onCompletion));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ApplicationUpdater::UpdateTask::abort ()
{
	if(pendingOperation)
		pendingOperation->cancel ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ApplicationUpdater::UpdateTask::onCanceled ()
{
	pendingOperation = nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ApplicationUpdater::UpdateTask::setDefinition (const ContentDefinition& _definition)
{
	definition = _definition;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ApplicationUpdater::UpdateTask::setErrorText (StringRef _errorText)
{
	errorText = _errorText;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ApplicationUpdater::UpdateTask::setResponse (StringRef response)
{
	String message = ApplicationUpdater::instance ().getContentServer ()->getContentVersionError (response);
	if(!message.isEmpty ())
	{
		setErrorText (message);
		setAction (response);
	}
	else
		currentVersion.scan (response);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ApplicationUpdater::UpdateTask::setAction (StringRef versionString)
{
	ApplicationUpdater::instance ().getContentServer ()->getContentVersionAction (actionUrl, actionTitle, versionString);
}

//************************************************************************************************
// ApplicationUpdater::UpdateFinalizer
//************************************************************************************************

void CCL_API ApplicationUpdater::UpdateFinalizer::execute (IObject* target)
{
	UnknownPtr<Web::ITransfer> transfer (target);
	ASSERT (transfer.isValid ())
	if(transfer == nullptr || transfer->getState () != Web::ITransfer::kCompleted)
		return;

	Url dstPath (transfer->getDstLocation ());

	// append version number to file name
	#if 1
	ASSERT (version != VersionNumber ())
	String versionSuffix = version.print ();
	String fileName;
	dstPath.getName (fileName, false);
	FileType fileType (dstPath.getFileType ());
	fileName << " " << versionSuffix << "." << fileType.getExtension ();

	Url dstPath2 (dstPath);
	dstPath2.setName (fileName);
	dstPath2.makeUnique ();
	if(System::GetFileSystem ().moveFile (dstPath2, dstPath))
	{
		dstPath = dstPath2;
		transfer->relocate (dstPath);
	}
	#endif

	// try to quit application
	Boxed::Variant quitDone;
	SignalSource (Signals::kApplication).signal (Message (Signals::kRequestQuit, quitDone.asUnknown ()));
	if(!static_cast<VariantRef> (quitDone).asBool ())
	{
		System::GetSystemShell ().showFile (dstPath);
		return;
	}

	// start installer
	System::GetSystemShell ().openUrl (dstPath, System::kRequestAdminPrivileges);
}

//************************************************************************************************
// ApplicationUpdater::ReleaseNotesFinalizer
//************************************************************************************************

void CCL_API ApplicationUpdater::ReleaseNotesFinalizer::execute (IObject* target)
{
	UnknownPtr<Web::ITransfer> transfer (target);
	ASSERT (transfer.isValid ())
	if(transfer == nullptr || transfer->getState () != Web::ITransfer::kCompleted)
		return;

	System::GetSystemShell ().openUrl (transfer->getDstLocation ());
}

//************************************************************************************************
// ApplicationUpdater
//************************************************************************************************

DEFINE_CLASS_HIDDEN (ApplicationUpdater, Component)
DEFINE_COMPONENT_SINGLETON (ApplicationUpdater)

//////////////////////////////////////////////////////////////////////////////////////////////////

ApplicationUpdater::ApplicationUpdater ()
: Component ("ApplicationUpdater"),
  contentServer (nullptr),
  releaseNotesId ("releasenotes"),
  updateAvailable (false)
{
	paramList.addCommand ("Help", "Update Now", "updateNow");
	paramList.addCommand ("Help", "View Release Notes", "viewReleaseNotes");
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ApplicationUpdater::setAppDefinition (const ContentDefinition& _appDefinition)
{
	#if CCL_PLATFORM_MAC
		#define PLATFORM_EXTENSION ".mac"
	#elif (CCL_PLATFORM_WINDOWS && CCL_PLATFORM_ARM && CCL_PLATFORM_64BIT)
		#define PLATFORM_EXTENSION ".win-arm64"
	#elif (CCL_PLATFORM_WINDOWS && CCL_PLATFORM_ARM64EC)
		#define PLATFORM_EXTENSION ".win-arm64ec"
	#elif (CCL_PLATFORM_WINDOWS && CCL_PLATFORM_INTEL)
		#define PLATFORM_EXTENSION ".win"
	#elif CCL_PLATFORM_IOS
		#define PLATFORM_EXTENSION ".ios"
	#elif CCL_PLATFORM_ANDROID
		#define PLATFORM_EXTENSION ".android"
	#elif (CCL_PLATFORM_LINUX && CCL_PLATFORM_ARM && CCL_PLATFORM_64BIT)
		#define PLATFORM_EXTENSION ".linux-arm64"
	#elif (CCL_PLATFORM_LINUX && CCL_PLATFORM_INTEL)
		#define PLATFORM_EXTENSION ".linux"
	#else
		#error Unsupported platform!
	#endif

	appDefinition = _appDefinition;

	appDefinition.contentId << PLATFORM_EXTENSION;

	#undef PLATFORM_EXTENSION

	loadUpdaterConfiguration ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ApplicationUpdater::loadUpdaterConfiguration ()
{
	const String kConfigFileName ("appupdater.config");

	Configuration::Registry& registry = Configuration::Registry::instance ();

	// check for configuration file next to executable
	Url appSupportFolder;
	System::GetSystem ().getLocation (appSupportFolder, System::kAppSupportFolder);
	Url configPath (appSupportFolder);
	configPath.descend (kConfigFileName);
	bool succeeded = registry.loadFromFile (configPath);

	// check for resource file
	if(!succeeded)
	{
		ResourceUrl configResourcePath (System::GetMainModuleRef (), kConfigFileName);
		succeeded = registry.loadFromFile (configResourcePath);
	}

	if(succeeded)
	{
		String contentId;
		if(registry.getValue (contentId, "AppDefinition", "contentID") && !contentId.isEmpty ())
			appDefinition.contentId = contentId;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const ContentDefinition& ApplicationUpdater::getAppDefinition () const
{
	return appDefinition;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ApplicationUpdater::checkAppUpdatesInBackground ()
{
	AutoPtr<IUnknown> credentials = contentServer->requestCredentials (IContentServer::kVersionCheckInBackground);
	if(credentials == nullptr)
		return;

	Promise p = contentServer->getContentVersion (appDefinition, credentials);

	auto toUpdateResult = [&] (IAsyncOperation& op)
	{
		AutoPtr<UpdateTask> task = NEW UpdateTask;
		task->setDefinition (appDefinition);
		task->setResponse (op.getResult ().asString ());
		op.setResult (Variant ().takeShared (task->asUnknown ()));
	};

	appUpdateOperation = p.then (toUpdateResult).then (this, &ApplicationUpdater::onAppCheckCompleted);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ApplicationUpdater::onAppCheckCompleted (IAsyncOperation& operation)
{
	if(operation.getState () == IAsyncInfo::kCompleted)
		if(IUpdateCheckResult* result = IUpdateCheckResult::getFirst (operation.getResult ()))
		{
			String text;
			setAppServerVersion (result->getCurrentVersion ());
			setUpdateAvailable (verifyResult (text, *result));
		}
	appUpdateOperation = nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ApplicationUpdater::verifyResult (String& text, const IUpdateCheckResult& result) const
{
	bool hasNewVersion = false;
	const ContentDefinition& definition = result.getDefinition ();
	const VersionNumber& serverVersion = result.getCurrentVersion ();

	VersionNumber localVersion;
	localVersion.scan (definition.version);

	if(!result.getErrorText ().isEmpty ())
		text << result.getErrorText ();
	else if(serverVersion == VersionNumber ())
		text << XSTR (VersionNotFound);
	else
	{
		//text.appendFormat (XSTR (VersionCheck), definition.title);
		//text << ENDLINE << ENDLINE;

		text.appendFormat (XSTR (LocalVersion), definition.version);
		text << ENDLINE;

		String versionString = serverVersion.print ();
		text.appendFormat (XSTR (ServerVersion), versionString);
		text << ENDLINE << ENDLINE;

		hasNewVersion = serverVersion > localVersion;

		if(hasNewVersion)
			text << XSTR (NewVersion);
		else
			text << XSTR (UpToDate);
	}

	#if TEST_DOWNLOAD
	hasNewVersion = true;
	#endif

	return hasNewVersion;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IAsyncOperation* ApplicationUpdater::checkUpdates (const ContentDefinition definitions[], int count)
{
	AutoPtr<IUnknown> credentials = contentServer->requestCredentials (IContentServer::kVersionCheck);
	if(!credentials)
	{
		// return canceled status if no credentials available
		AsyncOperation* op = NEW AsyncOperation;
		op->setState (AsyncOperation::kCanceled);
		return op;
	}
	else
		return checkUpdates (definitions, count, credentials);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IAsyncOperation* ApplicationUpdater::checkUpdates (const ContentDefinition definitions[], int count, IUnknown* credentials, IProgressNotify* progress)
{
	AutoPtr<BatchOperation> op = NEW BatchOperation;
	for(int i = 0; i < count; i++)
	{
		UpdateTask* task = NEW UpdateTask;
		task->setDefinition (definitions[i]);
		task->setCredentials (credentials);
		op->addTask (task);
	}
	if(progress)
		return op->runAsync (progress);
	else
	{
		op->setModalProgress (true);
		return op->runAsync (XSTR (CheckForUpdates));
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IMPLEMENT_COMMANDS (ApplicationUpdater, Component)

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ApplicationUpdater::onCheckUpdates (CmdArgs args)
{
	if(!args.checkOnly ())
	{
		bool cached = CommandAutomator::Arguments (args).getBool ("Cached");
		if(!cached)
			contentServer->purgeVersionCache ();

		Promise p = checkUpdates (&appDefinition, 1);
		p.then ([&] (IAsyncOperation& operation)
			{
				IUpdateCheckResult* result = nullptr;
				if(operation.getState () == IAsyncInfo::kCompleted)
					result = IUpdateCheckResult::getFirst (operation.getResult ());

				if(result == nullptr)
				{
					if(operation.getState () != IAsyncInfo::kCanceled)
						Alert::error (XSTR (CheckFailed));
				}
				else
				{
					String text;
					bool canDownload = verifyResult (text, *result);
					setAppServerVersion (result->getCurrentVersion ());
					setUpdateAvailable (canDownload);

					if(canDownload == false)
					{
						if(result->getErrorText ().isEmpty ())
							Alert::info (text);
						else
						{
							Url buttonUrl;
							String buttonTitle;
							result->getAction (buttonUrl, buttonTitle);
							if(!(buttonTitle.isEmpty () || buttonUrl.isEmpty ()))
							{
								int alertResult = Alert::ask (text, buttonTitle, Alert::button (Alert::kOk));
								if(alertResult == Alert::kFirstButton)
									System::GetSystemShell ().openUrl (buttonUrl);
							}
							else
								Alert::info (text);
						}
					}
					else
					{
						int result = Alert::ask (text, XSTR (UpdateNow), XSTR (ViewReleaseNotes), Alert::button (Alert::kCancel));
						switch(result)
						{
						case Alert::kFirstButton : onUpdateNow (CommandMsg ()); break;
						case Alert::kSecondButton : onViewReleaseNotes (CommandMsg ()); break;
						}
					}
				}
			});
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ApplicationUpdater::onUpdateNow (CmdArgs args)
{
	if(updateAvailable == false)
		return false;

	if(!args.checkOnly ())
	{
		AutoPtr<ITriggerAction> finalizer = NEW UpdateFinalizer (getAppServerVersion ());
		startDownload (appDefinition, finalizer);
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ApplicationUpdater::onViewReleaseNotes (CmdArgs args)
{
	// Please note: Command is always available via help menu, independent of update check.
	if(!args.checkOnly ())
	{
		AutoPtr<ITriggerAction> finalizer = NEW ReleaseNotesFinalizer;
		ContentDefinition releaseNotesDefinition (appDefinition);
		releaseNotesDefinition.contentId = releaseNotesId;
		startDownload (releaseNotesDefinition, finalizer);
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ApplicationUpdater::startDownload (const ContentDefinition& definition, ITriggerAction* finalizer,
										IUrl* _localPath, IUnknown* userData)
{
	if(AutoPtr<IUnknown> credentials = contentServer->requestCredentials (IContentServer::kContentDownload))
	{
		Url localPath;
		if(_localPath)
			localPath.assign (*_localPath);
		else
			System::GetSystem ().getLocation (localPath, System::kUserDownloadsFolder);

		Url url;
		contentServer->getContentURL (url, definition.productId, definition.contentId, definition.isExtension, credentials);

		AutoPtr<Web::IWebCredentials> webCredentials = contentServer->createCredentialsForURL (credentials);
		AutoPtr<Web::ITransfer> transfer = System::GetTransferManager ().createTransfer (localPath, url, Web::ITransfer::kDownload, webCredentials);
		Web::ITransfer* existing = System::GetTransferManager ().find (transfer);
		if(!existing || existing->getState () >= Web::ITransfer::kCompleted)
		{
			transfer->setSrcDisplayString (contentServer->getServerTitle ());
			if(finalizer)
				transfer->addFinalizer (return_shared (finalizer));
			if(userData)
				transfer->setUserData (return_shared (userData));

			System::GetTransferManager ().queue (transfer);
			return true;
		}
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ApplicationUpdater::downloadFile (IUrl& dstPath, const ContentDefinition& definition, IProgressNotify* progress)
{
	if(AutoPtr<IUnknown> credentials = contentServer->requestCredentials (IContentServer::kContentDownload))
		return downloadFile (dstPath, definition, credentials, progress);
	else
		return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ApplicationUpdater::downloadFile (IUrl& dstPath, const ContentDefinition& definition, IUnknown* credentials, IProgressNotify* progress)
{
	Url srcPath;
	contentServer->getContentURL (srcPath, definition.productId, definition.contentId, definition.isExtension, credentials);

	ASSERT (!dstPath.isEmpty ())
	AutoPtr<Web::IWebCredentials> webCredentials = contentServer->createCredentialsForURL (credentials);
	return System::GetTransferManager ().downloadFile (dstPath, srcPath, webCredentials, progress) == kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API ApplicationUpdater::terminate ()
{
	if(appUpdateOperation)
		appUpdateOperation->cancel ();
	return SuperClass::terminate ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ApplicationUpdater::setUpdateAvailable (bool state)
{
	updateAvailable = state;
	signal (Message (kPropertyChanged));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ApplicationUpdater::isUpdateAvailable () const
{
	return updateAvailable;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ApplicationUpdater::getProperty (Variant& var, MemberID propertyId) const
{
	if(propertyId == "updateAvailable")
	{
		var = updateAvailable;
		return true;
	}
	return SuperClass::getProperty (var, propertyId);
}
