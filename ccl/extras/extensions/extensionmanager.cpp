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
// Filename    : ccl/extras/extensions/extensionmanager.cpp
// Description : Extension Manager
//
//************************************************************************************************

#define EXTENSION_DEVELOPER_ENABLED	(1 && DEBUG)	// scan development location

#include "ccl/extras/extensions/extensionmanager.h"
#include "ccl/extras/extensions/extensionmanagement.h"
#include "ccl/extras/extensions/extensionhandler.h"
#include "ccl/extras/extensions/extensiondraghandler.h"
#include "ccl/extras/extensions/extensionpropertiesui.h"
#include "ccl/extras/extensions/appupdater.h"
#include "ccl/public/extras/icontentinstaller.h"

#include "ccl/app/application.h"
#include "ccl/app/utilities/fileicons.h"
#include "ccl/app/utilities/pluginclass.h"

#include "ccl/base/message.h"
#include "ccl/base/development.h"
#include "ccl/base/signalsource.h"
#include "ccl/base/asyncoperation.h"
#include "ccl/base/collections/stringlist.h"
#include "ccl/base/storage/file.h"
#include "ccl/base/storage/settings.h"
#include "ccl/base/storage/jsonarchive.h"

#include "ccl/public/text/translation.h"
#include "ccl/public/text/itextstreamer.h"
#include "ccl/public/gui/framework/ialert.h"
#include "ccl/public/gui/framework/iprogressdialog.h"
#include "ccl/public/gui/framework/iwindow.h"
#include "ccl/public/base/iprogress.h"
#include "ccl/public/base/memorystream.h"
#include "ccl/public/system/ifileitem.h"
#include "ccl/public/system/cclerror.h"
#include "ccl/public/system/ilogger.h"
#include "ccl/public/system/isysteminfo.h"
#include "ccl/public/system/inativefilesystem.h"
#include "ccl/public/system/ipackagehandler.h"
#include "ccl/public/system/ipackagefile.h"
#include "ccl/public/system/ipackagemetainfo.h"
#include "ccl/public/system/ifileutilities.h"
#include "ccl/public/systemservices.h"

#include "ccl/public/gui/framework/iuserinterface.h"
#include "ccl/public/gui/framework/icommandtable.h"
#include "ccl/public/guiservices.h"

#include "ccl/public/app/signals.h"

#include "ccl/public/plugins/iservicemanager.h"
#include "ccl/public/plugservices.h"

#include "ccl/public/network/web/iwebcredentials.h"
#include "ccl/public/network/web/itransfermanager.h"
#include "ccl/public/netservices.h"

#include "ccl/public/security/icryptokeystore.h"
#include "ccl/public/securityservices.h"

#include "ccl/public/cclversion.h"

namespace CCL {
namespace Install {

//************************************************************************************************
// ExtensionFileHandler
//************************************************************************************************

class ExtensionFileHandler: public Object,
							public AbstractFileHandler,
							public AbstractFileInstallHandler
{
public:
	ExtensionFileHandler ();

	// IFileHandler
	tbool CCL_API openFile (UrlRef path) override;
	State CCL_API getState (IFileDescriptor& descriptor) override;
	tbool CCL_API getDefaultLocation (IUrl& dst, IFileDescriptor& descriptor) override;

	// IFileInstallHandler
	tbool CCL_API canHandle (IFileDescriptor& descriptor) override;
	void CCL_API beginInstallation (tbool state) override;
	tbool CCL_API performInstallation (IFileDescriptor& descriptor, IUrl& path) override;
	tbool CCL_API isRestartRequired () const override;
	tbool CCL_API getFileLocation (IUrl& path, IFileDescriptor& descriptor) override;

	CLASS_INTERFACE2 (IFileHandler, IFileInstallHandler, Object)
};

static ExtensionFileHandler theExtensionFileHandler;

//************************************************************************************************
// ExtensionInstaller
//************************************************************************************************

class ExtensionInstaller: public Object
{
public:
	DECLARE_CLASS (ExtensionInstaller, Object)

	ExtensionInstaller ();
	~ExtensionInstaller ();

	static ExtensionInstaller* getActiveInstance ();

	void runInstallation (UrlRef path, bool silent = false, IProgressNotify* outerProgress = nullptr);
	void downloadUpdates (Container& candidates, bool forceDialog);
	bool isRestartRequired () const;

protected:
	static ExtensionInstaller* activeInstance;
	ExtensionManager& manager;
	bool restartRequired;
};

} // namespace Install
} // namespace CCL

using namespace CCL;
using namespace Install;

//////////////////////////////////////////////////////////////////////////////////////////////////
// Strings
//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_XSTRINGS ("ExtensionManager")
	XSTRING (ScanningExtensions, "Scanning Extensions...")
	XSTRING (MigratingExtensionX, "Migrating Extension %(1)...")
	XSTRING (ExtensionNotCompatible, "%(1) is not compatible with $APPNAME!")
	XSTRING (NewerVersionNeeded, "You need a newer version of $APPNAME.")
	XSTRING (ExtensionUninstall, "Removing Extension %(1)...")
	XSTRING (UpdatingExtension, "Updating Extension %(1)...")
	XSTRING (InstallExtension, "Install Extension")
	XSTRING (AskUpdateNow, "Do you want to install these updates now?")
	XSTRING (BuiltInExtensionWarning, "Built-in Extensions can not be uninstalled or updated.")
	XSTRING (ScanningPlugIns, "Scanning Plug-Ins...")
END_XSTRINGS

//////////////////////////////////////////////////////////////////////////////////////////////////
// Initialization
//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_KERNEL_INIT_LEVEL (ExtensionManager, kFirstRun)
{
	System::GetFileTypeRegistry ().registerHandler (&theExtensionFileHandler);
	System::GetFileTypeRegistry ().registerFileType (ExtensionDescription::getFileType ());
	return true;
}

CCL_KERNEL_TERM_LEVEL (ExtensionManager, kFirstRun)
{
	System::GetFileTypeRegistry ().unregisterHandler (&theExtensionFileHandler);
}

//************************************************************************************************
// ExtensionFilter
//************************************************************************************************

void ExtensionFilter::loadFilter ()
{
	AutoPtr<IStream> stream = CCL::File (ResourceUrl ("extensions.json")).open ();
	if(!stream)
		return;

	Attributes rootAttr;
	bool loaded = JsonArchive (*stream).loadAttributes (nullptr, rootAttr);
	ASSERT (loaded == true)
	if(AttributeQueue* conditionArray = rootAttr.getObject<AttributeQueue> (nullptr))
		ForEach (*conditionArray, Attribute, conditionItem)
			if(Attributes* a = unknown_cast<Attributes> (conditionItem->getValue ().asUnknown ()))
			{
				String id = a->getString ("id");
				ExtensionDescription::replacePlatform (id);
				VersionNumber minVersion;
				minVersion.scan (a->getString ("minVersion"));
				int flags = 0;
				if(a->getBool ("deprecated"))
					flags |= Condition::kDeprecated;

				ASSERT (!id.isEmpty ())
				if(!id.isEmpty ())
					conditions.add ({id, minVersion, flags});
			}
		EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ExtensionFilter::isCompatible (StringRef id, const VersionNumber& version) const
{
	for(auto c : conditions)
		if(c.extensionId == id)
		{
			if(c.isDeprecated ())
				return false;
			else
				return version >= c.minVersion;
		}

	return true;
}

//************************************************************************************************
// ExtensionManager
//************************************************************************************************

DEFINE_CLASS_HIDDEN (ExtensionManager, Component)
DEFINE_COMPONENT_SINGLETON (ExtensionManager)

//////////////////////////////////////////////////////////////////////////////////////////////////

const String ExtensionManager::kExtensionUpdateFolderName ("Updates");
static const CString kDeferInstallWithUI ("installWithUI");
static const CString kDeferInstallFromServer ("installFromServer");

//////////////////////////////////////////////////////////////////////////////////////////////////

ExtensionManager::ExtensionManager ()
: Component (CCLSTR ("ExtensionManager")),
  settings (*NEW XmlSettings (CCLSTR ("ExtensionManager"))),
  restored (false),
  started (false),
  credentials (nullptr)
{
	Url settingsPath;
	ExtensionManagement::getLocation (settingsPath, ExtensionType::kUser);
	settingsPath.descend (String () << "Extensions." << XmlSettings::getFileType ().getExtension ());
	settings.setPath (settingsPath);

	handlers.objectCleanup (true);
	extensions.objectCleanup (true);

	// add built-in handlers
	addHandler (NEW ExtensionLanguageHandler); // allow loading string tables first
	addHandler (NEW ExtensionNativePluginHandler);
	addHandler (NEW ExtensionCorePluginHandler);
	addHandler (NEW ExtensionScriptPluginHandler);
	addHandler (NEW ExtensionHelpHandler);
	addHandler (NEW ExtensionPresetHandler);	
	addHandler (NEW ExtensionSkinHandler);
	addHandler (NEW ExtensionSnapshotHandler);	
	addHandler (NEW ExternalExtensionHandler);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ExtensionManager::~ExtensionManager ()
{
	flushSettings ();

	settings.release ();

	cancelSignals ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int ExtensionManager::getExtensionCount () const
{
	return extensions.count ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ExtensionDescription* ExtensionManager::getExtensionDescription (int index) const
{
	return (ExtensionDescription*)extensions.at (index);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ExtensionDescription* ExtensionManager::findExtension (StringRef id) const
{
	ForEach (extensions, ExtensionDescription, e)
		if(e->getID () == id)
			return e;
	EndFor
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

String ExtensionManager::formatMessage (ErrorCode which, const ExtensionDescription& e, bool detailed) const
{
	String message;
	switch(which)
	{
	case kAlreadyInstalled :
		message = ExtensionStrings::AlreadyInstalled (e.getTitle ());
		break;
	case kNotCompatible :
		message.appendFormat (XSTR (ExtensionNotCompatible), e.getTitle ());
		if(detailed)
		{
			if(e.getCompatibilityResult () == File::kAppTooOld)
				message << "\n" << XSTR (NewerVersionNeeded);
		}
		break;
	}
	return message;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ExtensionManager::addHandler (ExtensionHandler* handler, int priority)
{
	if(priority == kFirstHandler)
		if(handlers.insertAt (0, handler))
			return;

	handlers.add (handler);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const Container& ExtensionManager::getHandlers () const
{
	return handlers;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ExtensionManager::getUpdateLocation (Url& path) const
{
	System::GetSystem ().getLocation (path, System::kAppSettingsFolder);
	path.descend (kExtensionUpdateFolderName, Url::kFolder);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ExtensionDescription* ExtensionManager::scanExtension (StringRef id, StringRef shortId)
{
	AutoPtr<ExtensionDescription> e;

	#if EXTENSION_DEVELOPER_ENABLED
	if(!shortId.isEmpty ())
	{
		Url devPath;
		ExtensionManagement::makePath (devPath, shortId, ExtensionType::kDeveloper);
		e = ExtensionDescription::createFromPackage (devPath);
		if(e)
			e->setType (ExtensionType::kDeveloper);
	}
	#endif

	if(e == nullptr)
	{
		Url userPath;
		ExtensionManagement::makePath (userPath, id, ExtensionType::kUser);
		e = ExtensionDescription::createFromPackage (userPath);
		if(e)
			e->setType (ExtensionType::kUser);
	}

	if(e == nullptr)
	{
		Url sharedPath;
		ExtensionManagement::makePath (sharedPath, id, ExtensionType::kShared);
		e = ExtensionDescription::createFromPackage (sharedPath);
		if(e)
			e->setType (ExtensionType::kShared);
	}

	if(e && !checkCompatibility (*e))
		e.release ();

	if(e.isValid ())
	{
		// restore settings
		if(restored == false)
			settings.restore (),
			restored = true;

		// update enabled and uninstall state
		updateEnabledState (*e);
	}

	return e.detach ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ExtensionManager::updateEnabledState (ExtensionDescription& e)
{
	ASSERT (restored == true)
	Attributes& a = settings.getAttributes (e.getID ());
		
	bool enabled = true;
	a.getBool (enabled, "enabled");
	e.setEnabled (enabled);

	bool uninstallPending = false;
	a.getBool (uninstallPending, "uninstallPending");
	e.setUninstallPending (uninstallPending);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ExtensionManager::startupExtension (ExtensionDescription& e)
{
	if(!extensions.contains (e))
	{
		extensions.add (&e);
		e.retain ();
	}

	e.setStarted (true);

	int useCount = 0;
	ForEach (handlers, ExtensionHandler, handler)
		useCount += handler->startupExtension (e);
	EndFor
	e.setUseCount (useCount);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ExtensionManager::startup (IProgressNotify* progress)
{
	// load filter
	ExtensionFilter::instance ().loadFilter ();

	// check for extensions to migrate first
	if(migrationSourceFolder != nullptr)
	{
		migrateFiles (*migrationSourceFolder);
		migrationSourceFolder.release ();
	}

	System::GetLogger ().reportEvent (XSTR (ScanningExtensions));
	if(progress)
		progress->setProgressText (XSTR (ScanningExtensions));
	started = true;

	// give handlers a chance to start up
	ForEach (handlers, ExtensionHandler, handler)
		handler->startup ();
	EndFor

	// restore settings
	if(restored == false)
		settings.restore (),
		restored = true;

	// scan developer location first
	#if EXTENSION_DEVELOPER_ENABLED
	Url devPath;
	ExtensionManagement::getLocation (devPath, ExtensionType::kDeveloper);
	scanFolder (devPath, ExtensionType::kDeveloper);
	#endif

	// scan program folder
	Url progPath;
	ExtensionManagement::getLocation (progPath, ExtensionType::kProgram);
	scanFolder (progPath, ExtensionType::kProgram);

	// check for pending updates
	installUpdates ();
	
	// scan regular extensions folder
	Url userPath;
	ExtensionManagement::getLocation (userPath, ExtensionType::kUser);
	scanFolder (userPath, ExtensionType::kUser);

	// scan shared extensions folder
	Url sharedPath;
	ExtensionManagement::getLocation (sharedPath, ExtensionType::kShared);
	if(ExtensionManagement::lockDirectory (ExtensionType::kShared, RootComponent::instance ().getApplicationTitle ()))
		scanFolder (sharedPath, ExtensionType::kShared);
	else
	{
		UrlDisplayString message (sharedPath);
		message << ": ";
		message << System::GetFileSystem ().getErrorString (INativeFileSystem::kAccesDenied);
		System::GetLogger ().reportEvent (Alert::Event (message, Alert::kWarning));
	}
	
	ObjectArray toUninstall;
	toUninstall.objectCleanup (true);
	ForEach (extensions, ExtensionDescription, e)		
		// update enabled and uninstall state
		updateEnabledState (*e);

		if(e->isUninstallPending () && isUserInstalled (*e))
		{
			toUninstall.add (return_shared (e));
			continue;
		}
	EndFor

	ForEach (toUninstall, ExtensionDescription, e)
		String message;
		message.appendFormat (XSTR (ExtensionUninstall), e->getTitle ());
		System::GetLogger ().reportEvent (Alert::Event (message, Alert::kWarning));
			
		uninstall (*e);
	EndFor

	ForEach (extensions, ExtensionDescription, e)
		if(!e->isCompatible ())
			continue;

		if(e->isEnabled ())
			startupExtension (*e);
		#if DEBUG
		else
			Debugger::println (String () << "Extension " << e->getID () << " is disabled!");
		#endif
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ExtensionManager::isStarted () const
{
	return started; 
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ExtensionManager::checkAutomaticUpdates (bool& restartNeeded)
{
	restartNeeded = false;

	// check incompatible and bundled extensions (bundled = using app product id)
	ObjectArray candidates;
	ForEach (extensions, ExtensionDescription, e)
		if(isUserInstalled (*e))
			if(!e->isCompatible () || e->isUsingAppProductID ())
				candidates.add (e);
	EndFor

	if(candidates.isEmpty ())
		return;

	if(!checkUpdates (candidates, true))
		return;

	ExtensionInstaller installer;
	installer.downloadUpdates (candidates, false);
	restartNeeded = installer.isRestartRequired ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API ExtensionManager::terminate ()
{
	// shutdown extensions
	ForEach (extensions, ExtensionDescription, e)
		ForEach (handlers, ExtensionHandler, handler)
			handler->shutdownExtension (*e);
		EndFor
	EndFor

	// shut down handlers
	ForEach (handlers, ExtensionHandler, handler)
		handler->shutdown ();
	EndFor

	if(isStarted ())
	{
		if(!ExtensionManagement::unlockDirectory (ExtensionType::kShared))
		{
			ASSERT (false)
		}
	}

	return SuperClass::terminate ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ExtensionManager::migrateFiles (UrlRef folder)
{
	WaitCursor waitCursor (System::GetGUI ());
	ForEachFile (System::GetFileSystem ().newIterator (folder, IFileIterator::kAll), path)
		if(path->isFolder ())
			if(AutoPtr<ExtensionDescription> e = ExtensionDescription::createFromPackage (*path))
			{
				// check compatibility
				if(checkCompatibility (*e))
				{
					String folderName;
					path->getName (folderName);

					Url dstPath;
					ExtensionManagement::getLocation (dstPath, ExtensionType::kUser);
					dstPath.descend (folderName, Url::kFolder);

					if(!CCL::File (dstPath).exists ())
					{
						System::GetLogger ().reportEvent (String ().appendFormat (XSTR (MigratingExtensionX), e->getTitle ()));
						CCL::File::copyFolder (dstPath, *path, nullptr, true);
					}
				}
			}		
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ExtensionManager::scanFolder (UrlRef folder, ExtensionTypeID extType)
{
	ForEachFile (System::GetFileSystem ().newIterator (folder, IFileIterator::kAll), path)
		if(path->isFolder ())
			if(AutoPtr<ExtensionDescription> e = ExtensionDescription::createFromPackage (*path))
			{
				// filter duplicates
				ExtensionDescription* existing = findExtension (e->getID ());
				if(existing != nullptr)
				{
					// warn only if two user extensions found with the same identifier
					if(isUserInstalled (*existing))
					{
						// replace existing package if new package has a higher version
						if(existing->getVersion () < e->getVersion ())
						{
							extensions.remove (existing);
							existing->release ();
						}
						else 
						{
							System::GetLogger ().reportEvent (Alert::Event (formatMessage (kAlreadyInstalled, *e), Alert::kWarning));
							continue;
						}
					}
					else
						continue;
				}

				// check compatibility
				if(!checkCompatibility (*e))
				{
					// ignore built-in (and flagged) extensions silently, they might work depending on the program license 
					// also ignore shared extensions, they might work depending on the host application
					if((extType != ExtensionType::kUser) || e->isSilentCheckEnabled ())
						continue;

					// ignore local extensions if there is a shared extension with the same name
					if(extType == ExtensionType::kUser)
					{
						Url sharedPath;
						ExtensionManagement::makePath (sharedPath, e->getID (), ExtensionType::kShared);
						if(System::GetFileSystem ().fileExists (sharedPath))
							continue;
					}
					
					// incompatible user extensions are disabled but kept in list
					System::GetLogger ().reportEvent (Alert::Event (formatMessage (kNotCompatible, *e), Alert::kError));
				}
				
				e->setType (extType);
				e->retain ();
				extensions.add (e);
			}
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ExtensionManager::installUpdates ()
{
	Url folder;
	getUpdateLocation (folder);

	ForEachFile (System::GetFileSystem ().newIterator (folder), path)
		if(path->isFile () && path->getFileType () == ExtensionDescription::getFileType ())
		{
			AutoPtr<ExtensionDescription> e = ExtensionDescription::createFromPackage (*path);
			ExtensionDescription* existing = e ? findExtension (e->getID ()) : nullptr;

			// Note: User extensions can not exist at this stage!
			ASSERT (!existing || !isUserInstalled (*existing))

			if(e && !existing)
			{
				System::GetLogger ().reportEvent (Alert::Event (String ().appendFormat (XSTR (UpdatingExtension), e->getTitle ()), Alert::kWarning));
				
				Url dstPath;
				ExtensionManagement::makePath (dstPath, *e);

				// remove old version (if any)
				if(System::GetFileSystem ().fileExists (dstPath))
				{
					ErrorContextGuard errorContext;
					System::GetFileSystem ().removeFolder (dstPath, INativeFileSystem::kDeleteRecursively);
					if(errorContext->getEventCount () > 0)
						System::GetLogger ().reportEvent (errorContext->getEvent (0));				
				}

				// extract new version
				AutoPtr<IPackageFile> p = System::GetPackageHandler ().openPackage (*path);
				if(p)
				{
					ErrorContextGuard errorContext;
					p->extractAll (dstPath, true);
					if(errorContext->getEventCount () > 0)
						System::GetLogger ().reportEvent (errorContext->getEvent (0));				
					p->close ();
				}
			}

			// remove update file
			{
				ErrorContextGuard errorContext;
				if(!System::GetFileSystem ().removeFile (*path))
					if(errorContext->getEventCount () > 0)
						System::GetLogger ().reportEvent (errorContext->getEvent (0));				
			}
		}
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ExtensionManager::deferInstallWithUI (UrlRef path)
{
	if(!started)
		return;
	AutoPtr<Url> pathCopy = NEW Url (path);
	(NEW Message (kDeferInstallWithUI, pathCopy->asUnknown ()))->post (this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ExtensionManager::deferInstallFromServer ()
{
	(NEW Message (kDeferInstallFromServer))->post (this, 100);	
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IDragHandler* ExtensionManager::createDragHandler (const DragEvent& event, IView* view)
{
	AutoPtr<ExtensionDragHandler> handler (NEW ExtensionDragHandler (view));
	if(handler->prepare (event.session.getItems (), &event.session))
	{
		event.session.setResult (IDragSession::kDropCopyReal);
		handler->retain ();
		return handler;
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API ExtensionManager::notify (ISubject* subject, MessageRef msg)
{
	if(msg == kDeferInstallWithUI)
	{
		if(System::GetDesktop ().isInMode (IDesktop::kProgressMode))
		{
			(NEW Message (msg))->post (this, 1000);
			return;
		}

		UnknownPtr<IUrl> path (msg[0]);
		ASSERT (path != nullptr)

		if(ExtensionInstaller* activeInstaller = ExtensionInstaller::getActiveInstance ())
			activeInstaller->runInstallation (*path);
		else
		{
			ExtensionInstaller ().runInstallation (*path);
		}
	}
	else if(msg == kDeferInstallFromServer)
	{
		Attributes args;
		args.set ("userContent", true);
		System::GetCommandTable ().performCommand (CommandMsg ("Application", "Install Packages", static_cast<IAttributeList*> (&args)), true);
	}
	else
		SuperClass::notify (subject, msg);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ExtensionManager::setSharedLocation (UrlRef path)
{
	ExtensionManagement::setSharedLocation (path);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ExtensionManager::isInsideExtension (UrlRef path) const
{
	return ExtensionManagement::isInsideExtension (path);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ExtensionManager::isUserInstalled (const ExtensionDescription& e) const
{
	return ExtensionManagement::isUserInstalled (e);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ExtensionManager::checkSignature (UrlRef srcPath, IProgressNotify* progress)
{
	return ExtensionManagement::checkSignature (srcPath, signatureFilter, progress);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ExtensionManager::checkCompatibility (ExtensionDescription& candidate) const
{
	candidate.setCompatibilityResult (File::kAppUnknown);

	// check platform
	if(!candidate.getPlatform ().isEmpty ())
		if(candidate.getPlatform () != ExtensionDescription::getPlatformName ())
			return false;

	// check install manifest
	if(File* file = candidate.getManifestEntry ())
		candidate.setCompatibilityResult (file->canInstallWithVersion (appIdentity, appVersion));

	// check filter
	if(ExtensionFilter::instance ().isCompatible (candidate.getID (), candidate.getVersion ()) == false)
		candidate.setCompatibilityResult (File::kAppTooNew);

	// give handlers a chance to mark extensions incompatible
	if(candidate.isCompatible ())
	{
		ForEach (handlers, ExtensionHandler, handler)
			UnknownPtr<IExtensionCompatibilityHandler> compatibilityHandler (handler->asUnknown ());
			if(compatibilityHandler)
			{
				tresult result = compatibilityHandler->checkCompatibility (candidate);
				if(result == kResultFailed)
				{
					candidate.setCompatibilityResult (File::kAppTooNew); // assuming a newer extension version is required
					break;
				}
			}
		EndFor
	}

	return candidate.isCompatible ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringRef ExtensionManager::getParentProductID (ExtensionDescription& e) const
{
	if(e.isUsingAppProductID ())
		return ApplicationUpdater::instance ().getAppDefinition ().productId;
	else
	{
		if(e.getParentProductID ().isEmpty () && e.isStarted () == false)
		{
			// try to load product information for extensions that haven't been started
			UnknownPtr<IExtensionProductHandler> productHandler;
			ForEach (handlers, ExtensionHandler, handler)
				productHandler = handler->asUnknown ();
				if(productHandler.isValid ())
					break;
			EndFor
			
			if(productHandler)
				productHandler->detectProducts (e);
		}

		return e.getParentProductID ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ExtensionManager::checkUpdates (Container& extensions, bool silent)
{
	// When not passing "progress" to checkUpdatesAsync, the operation runs with a modal progress window.
	// The returned operation is already completed (if not canceled or failed), so it's safe to release it here.
	AutoPtr<IAsyncOperation> operation = checkUpdatesAsync (extensions, silent);
	return operation.isValid ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IAsyncOperation* ExtensionManager::checkUpdatesAsync (Container& extensions, bool silent, IProgressNotify* progress)
{
	// ensure that credentials are available for download afterwards
	IContentServer* server = ApplicationUpdater::instance ().getContentServer ();
	ASSERT (server != nullptr)
	if(!credentials && server)
		credentials = server->requestCredentials (IContentServer::kContentDownload, silent ? IContentServer::kSuppressErrors | IContentServer::kSuppressLogin : 0);
	if(!credentials)
		return nullptr;

	ApplicationUpdater& updater = ApplicationUpdater::instance ();
	Vector<ContentDefinition> definitions;
	definitions.add (updater.getAppDefinition ()); // always check if application is up-to-date!

	ForEach (extensions, ExtensionDescription, e)
		definitions.add (ContentDefinition (e->getTitle (), getParentProductID (*e), e->getID (), e->getVersion ().print (), true));
	EndFor

	Promise p = updater.checkUpdates (definitions, definitions.count (), credentials, progress);
	return return_shared<IAsyncOperation> (p.then ([&] (IAsyncOperation& operation)
		{
			if(operation.getState () == IAsyncInfo::kCompleted)
			{
				bool first = true, appUpdatesNeeded = false;
				UnknownPtr<IContainer> c (operation.getResult ().asUnknown ());
				if(c.isValid ())
					ForEachUnknown (*c, unk)
						UnknownPtr<IUpdateCheckResult> result (unk);
						ASSERT (result.isValid ())
						if(first == true)
						{
							appUpdatesNeeded = result->getCurrentVersion () > VersionNumber ().scan (result->getDefinition ().version);
							first = false;
						}
						else if(ExtensionDescription* e = findExtension (result->getDefinition ().contentId))
						{
							#if RELEASE
							if(result->getCurrentVersion () == VersionNumber () || appUpdatesNeeded == true) // ignore if application not up-to-date
							#else
							if(result->getCurrentVersion () == VersionNumber ())
							#endif
							{
								e->setUpdateAvailable (false);
								e->setNewVersion (VersionNumber ());
							}
							else
							{
								e->setNewVersion (result->getCurrentVersion ());
								e->setUpdateAvailable (e->getNewVersion () > e->getVersion ());
							}
						}
					EndFor
			}
		}));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ExtensionManager::downloadUpdate (Url& dstPath, ExtensionDescription& e, IProgressNotify* progress)
{
	ASSERT (credentials != nullptr)
	if(!credentials)
		return false;

	if(progress)
		progress->setProgressText (String ().appendFormat (XSTR (UpdatingExtension), e.getTitle ()));

	ContentDefinition definition;
	definition.productId = getParentProductID (e);
	definition.contentId = e.getID ();
	definition.isExtension = true;

	return ApplicationUpdater::instance ().downloadFile (dstPath, definition, credentials, progress);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ExtensionManager::installFile (UrlRef srcPath, ExtensionDescription& e, IProgressNotify* progress)
{
	ASSERT (e.isCompatible ()) // must be checked before!

	StringList lockingApplications;
	ExtensionManagement::getLockingApplicationNames (lockingApplications, e.isUsingSharedLocation () ? ExtensionType::kShared : ExtensionType::kUser);
	for(auto applicationName : lockingApplications)
	{
		String message;
		System::GetLogger ().reportEvent (Alert::Event (ExtensionStrings::DirectoryLocked (*applicationName), Alert::kWarning));
	}

	if(!ExtensionManagement::installFile (srcPath, e, progress))
		return false;

	// add to list
	extensions.add (&e);
	e.retain ();

	e.setEnabled (true);
	e.setStarted (false); // needs restart!

	// emit global signal
	AutoPtr<Url> path2 = NEW Url (e.getPath ());
	SignalSource (Signals::kExtensionManager).signal (Message (Signals::kExtensionInstalled, e.asUnknown ()));
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ExtensionManager::signalInstalled (ExtensionDescription& e, bool silent)
{
	// notify handlers
	ForEach (handlers, ExtensionHandler, handler)
		handler->onExtensionInstalled (e, silent);
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ExtensionManager::uninstall (ExtensionDescription& e)
{
	if(!isUserInstalled (e))
		return false;
	
	StringList lockingApplications;
	ExtensionManagement::getLockingApplicationNames (lockingApplications, e.isUsingSharedLocation () ? ExtensionType::kShared : ExtensionType::kUser);
	for(auto applicationName : lockingApplications)
	{
		String message;
		System::GetLogger ().reportEvent (Alert::Event (ExtensionStrings::DirectoryLocked (*applicationName), Alert::kWarning));
	}

	if(e.isStarted ())
	{
		e.setUninstallPending (true);

		// notify handlers
		ForEach (handlers, ExtensionHandler, handler)
			handler->onExtensionChanged (e);
		EndFor
		return false;
	}

	if(!ExtensionManagement::uninstall (e))
		return false;

	ASSERT (extensions.contains (&e))
	extensions.remove (&e);
	e.release ();
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ExtensionManager::updateFile (UrlRef srcPath, ExtensionDescription& e, IProgressNotify* progress)
{
	StringList lockingApplications;
	ExtensionManagement::getLockingApplicationNames (lockingApplications, e.isUsingSharedLocation () ? ExtensionType::kShared : ExtensionType::kUser);
	for(auto applicationName : lockingApplications)
	{
		String message;
		System::GetLogger ().reportEvent (Alert::Event (ExtensionStrings::DirectoryLocked (*applicationName), Alert::kWarning));
	}
	if(lockingApplications.isEmpty () == false)
		return false;

	String fileName;
	srcPath.getName (fileName);

	Url dstPath;
	getUpdateLocation (dstPath);
	dstPath.descend (fileName);

	// copy new package to update location
	if(!System::GetFileSystem ().copyFile (dstPath, srcPath, 0, progress))
		return false;

	// update state of existing extension
	e.setUpdatePending (true);

	// notify handlers
	ForEach (handlers, ExtensionHandler, handler)
		handler->onExtensionChanged (e);
	EndFor
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ExtensionManager::enable (Container& extensions, bool state)
{
	bool anyChanged = false;
	ForEach (extensions, ExtensionDescription, e)
		bool changed = false;
		if(state == true) // clear uninstall flag when re-enabled
		{
			if(e->isUninstallPending ())
			{
				e->setUninstallPending (false);
				changed = true;
			}
		}

		if(e->isEnabled () != state)
		{
			e->setEnabled (state);
			changed = true;
		}

		if(changed)
		{
			anyChanged = true;

			// notify handlers
			ForEach (handlers, ExtensionHandler, handler)
				handler->onExtensionChanged (*e);
			EndFor
		}
	EndFor
	return anyChanged;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ExtensionManager::flushSettings ()
{
	settings.removeAll ();

	ForEach (extensions, ExtensionDescription, e)
		Attributes& a = settings.getAttributes (e->getID ());
		a.set ("enabled", e->isEnabled ());
		a.set ("uninstallPending", e->isUninstallPending ());
	EndFor

	settings.flush ();
}

////////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API ExtensionManager::countDiagnosticData () const
{
	return 1;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ExtensionManager::getDiagnosticDescription (DiagnosticDescription& description, int index) const
{
	if(index == 0)
	{
		description.categoryFlags = DiagnosticDescription::kPlugInInformation;
		settings.getPath ().getName (description.fileName, false);
		description.fileType = FileTypes::Text ();
		return true;
	}
	return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

IStream* CCL_API ExtensionManager::createDiagnosticData (int index)
{
	if(index == 0)
	{
		AutoPtr<IStream> stream = NEW MemoryStream;
		AutoPtr<ITextStreamer> streamer = System::CreateTextStreamer (*stream, {Text::kUTF8, Text::kSystemLineFormat});
		if(streamer == nullptr)
			return nullptr;

		ForEach (extensions, ExtensionDescription, e)
			streamer->writeLine (String ().appendFormat ("%(1) %(2)", e->getTitle (), (String)e->getVersion ()));
			if(!e->isEnabled ())
				streamer->writeLine ("\tdisabled");
			if(e->isUsingSharedLocation ())
				streamer->writeLine ("\tshared");
		EndFor

		return stream.detach ();
	}
	return nullptr;
}

//************************************************************************************************
// ExtensionFileHandler
//************************************************************************************************

ExtensionFileHandler::ExtensionFileHandler ()
: AbstractFileInstallHandler (kInstallOrderFirst)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ExtensionFileHandler::openFile (UrlRef path)
{
	if(path.getFileType () == ExtensionDescription::getFileType ())
	{
		ExtensionManager::instance ().deferInstallWithUI (path);
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IFileHandler::State CCL_API ExtensionFileHandler::getState (IFileDescriptor& descriptor)
{
	FileType fileType;
	descriptor.getFileType (fileType);
	if(fileType == ExtensionDescription::getFileType ())
	{
		AutoPtr<ExtensionDescription> e = ExtensionDescription::createFromDescriptor (descriptor);
		if(e == nullptr)
			return kNotCompatible;

		if(!ExtensionManager::instance ().checkCompatibility (*e))
			return kNotCompatible;

		if(ExtensionDescription* existing = ExtensionManager::instance ().findExtension (e->getID ()))
		{
			if(e->getVersion () > existing->getVersion ())
				return kCanUpdate;
			else
				return kInstalled;
		}

		return kCanInstall;
	}
	return kNotHandled;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ExtensionFileHandler::getDefaultLocation (IUrl& dst, IFileDescriptor& descriptor)
{
	// no special location for .install files
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ExtensionFileHandler::canHandle (IFileDescriptor& descriptor)
{
	if(ExtensionManager::instance ().isStarted () == false)
		return false;
	FileType fileType;
	descriptor.getFileType (fileType);
	return fileType == ExtensionDescription::getFileType ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API ExtensionFileHandler::beginInstallation (tbool state)
{
	ForEach (ExtensionManager::instance ().getHandlers (), ExtensionHandler, handler)
		handler->beginInstallation (state != 0);
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ExtensionFileHandler::performInstallation (IFileDescriptor& descriptor, IUrl& path)
{
	// install directly without dialogs (similar to extension updates)
	ExtensionInstaller ().runInstallation (path, true);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ExtensionFileHandler::isRestartRequired () const
{
	auto& extensionManager = ExtensionManager::instance ();
	for(int i = 0, count = extensionManager.getExtensionCount (); i < count; i++)
	{
		ExtensionDescription* e = extensionManager.getExtensionDescription (i);
		if(e->isUpdatePending ())
			return true;
		if(e->isCompatible () && (e->isEnabled () != e->isStarted () && !e->canPlugInRescanInsteadRestart ()))
			return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ExtensionFileHandler::getFileLocation (IUrl& path, IFileDescriptor& descriptor)
{
	FileType fileType;
	descriptor.getFileType (fileType);
	if(fileType == ExtensionDescription::getFileType ())
	{
		Attributes metaInfo;
		descriptor.getMetaInfo (metaInfo);
		String id = metaInfo.getString (Meta::kPackageID);
		if(ExtensionDescription* e = ExtensionManager::instance ().findExtension (id))
			if(ExtensionManager::instance ().isUserInstalled (*e))
			{
				path.assign (e->getPath ());
				return true;
			}
	}
	return false;
}

//************************************************************************************************
// ExtensionInstaller
//************************************************************************************************

ExtensionInstaller* ExtensionInstaller::activeInstance = nullptr;

//////////////////////////////////////////////////////////////////////////////////////////////////

ExtensionInstaller* ExtensionInstaller::getActiveInstance ()
{
	return activeInstance;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_CLASS_HIDDEN (ExtensionInstaller, Component)

//////////////////////////////////////////////////////////////////////////////////////////////////

ExtensionInstaller::ExtensionInstaller ()
: manager (ExtensionManager::instance ()),
  restartRequired (false)
{
	ASSERT (activeInstance == nullptr)
	if(activeInstance == nullptr)
		activeInstance = this;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ExtensionInstaller::~ExtensionInstaller ()
{
	if(activeInstance == this)
		activeInstance = nullptr;

	cancelSignals ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ExtensionInstaller::runInstallation (UrlRef path, bool silent, IProgressNotify* outerProgress)
{
	auto makeProgress = [] (IProgressNotify* outerProgress)
	{
		if(outerProgress)
			return outerProgress->createSubProgress ();

		IProgressNotify* progress = ccl_new<IProgressNotify> (ClassID::ProgressDialog);
		UnknownPtr<IProgressDialog> (progress)->setOpenDelay (1.); // do not open immediately
		return progress;
	};

	Alert::IReporter& reporter = Alert::getReporter (silent);

	// verify package
	AutoPtr<ExtensionDescription> e = ExtensionDescription::createFromPackage (path);
	if(e == nullptr)
	{
		reporter.reportEvent (Alert::Event (ExtensionStrings::InvalidInstallFile (), Alert::kError));
		return;
	}

	// check digital signature (hashing might take a while with large files!)
	{
		AutoPtr<IProgressNotify> progress = makeProgress (outerProgress);
		if(progress)
			progress->setTitle (ExtensionStrings::SignatureCheck ());
		ProgressNotifyScope notifyScope (progress);
		if(!manager.checkSignature (path, progress))
		{
			reporter.reportEvent (Alert::Event (ExtensionStrings::InvalidSignature (), Alert::kError));
			return;
		}
	}

	// check compatibility
	if(manager.checkCompatibility (*e) == false)
	{
		reporter.reportEvent (Alert::Event (manager.formatMessage (ExtensionManager::kNotCompatible, *e), Alert::kWarning));
		return;
	}

	// check if already installed
	SharedPtr<ExtensionDescription> existing = manager.findExtension (e->getID ());
	if(existing)
	{
		// ignore if not a user extension
		if(!manager.isUserInstalled (*existing))
		{
			reporter.reportEvent (Alert::Event (XSTR (BuiltInExtensionWarning), Alert::kInformation));
			return;
		}

		// ignore if installed version is newer
		if(existing->getVersion () >= e->getVersion ())
		{
			reporter.reportEvent (Alert::Event (manager.formatMessage (ExtensionManager::kAlreadyInstalled, *e), Alert::kWarning));
			return;
		}
	}

	// ask before install
	if(silent == false && !ExtensionPropertiesUI (e).askInstall ())
		return;

	// install (or update) file
	AutoPtr<IProgressNotify> progress = makeProgress (outerProgress);
	if(progress)
	{
		progress->setTitle (XSTR (InstallExtension));
		if(UnknownPtr<IProgressDialog> dialog = static_cast<IProgressNotify*> (progress)) // avoid flicker
			dialog->constrainLevels (2, 2);
	}
	ProgressNotifyScope notifyScope (progress);

	bool installed = false;
	if(existing)
		installed = manager.updateFile (path, *existing, progress);
	else
		installed = manager.installFile (path, *e, progress);

	bool canceled = progress && progress->isCanceled ();
	notifyScope.finish ();

	if(installed == false)
	{
		if(silent == false && canceled == false)
			Alert::errorWithContext (ExtensionStrings::InstallationFailed (), true);
	}
	else
	{
		ExtensionDescription* description = existing ? existing.as_plain () : e.as_plain ();
		if(description->isUpdatePending ())
			restartRequired = true;
		else if(description->isUninstallPending ())
			restartRequired = true;
		else if(description->isCompatible () && (description->isEnabled () != description->isStarted ()))
		{
			if(existing == nullptr && description->canPlugInRescanInsteadRestart ())
			{
				// extension contains plug-ins only and can be installed (not updated) without restarting the application
				manager.startupExtension (*description);
				AutoPtr<IProgressNotify> scanProgress = makeProgress (outerProgress);
				scanProgress->setTitle (XSTR (ScanningPlugIns));
				SignalSource (Signals::kPlugIns).signal (Message (Signals::kRescanPlugIns, scanProgress.as_plain (), 0));
			}
			else
				restartRequired = true;
		}

		if(silent == false && restartRequired)
			SignalSource (Signals::kApplication).deferSignal (NEW Message (Signals::kRequestRestart, ApplicationStrings::RestartRequired (), false));

		if(existing == nullptr) // don't signal for extension updates!
			manager.signalInstalled (*e, true);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ExtensionInstaller::downloadUpdates (Container& candidates, bool forceDialog)
{
	// ask user if updates should be installed
	String availableList;
	ForEach (candidates, ExtensionDescription, e)
		#if TEST_CHECK_UPDATES
		e->setUpdateAvailable (true);
		#endif

		if(e->isUpdateAvailable ())
			availableList << e->getTitle () << " " << e->getNewVersion ().print () << ENDLINE;
	EndFor

	if(availableList.isEmpty ())
	{
		if(forceDialog)
			Alert::info (ExtensionStrings::NoUpdatesFound ());
		return;
	}

	String message;
	message << ExtensionStrings::UpdatesAvailable () << ENDLINE << ENDLINE;
	message << availableList << ENDLINE;
	message << XSTR (AskUpdateNow);
	if(Alert::ask (message) != Alert::kYes)
		return;

	// download and install updates
	ErrorContextGuard errorContext;
	AutoPtr<IProgressNotify> progress = ccl_new<IProgressNotify> (ClassID::ProgressDialog);
	ProgressNotifyScope notifyScope (progress);

	ForEach (candidates, ExtensionDescription, e)
		if(!e->isUpdateAvailable ())
			continue;

		Url dstPath;
		System::GetSystem ().getLocation (dstPath, System::kTempFolder);
		bool downloaded = manager.downloadUpdate (dstPath, *e, progress);
		#if TEST_CHECK_UPDATES
		downloaded = true;
		#endif
		if(downloaded)
		{
			runInstallation (dstPath, progress);
			CCL::File (dstPath).remove ();
		}

		// reset state
		e->setUpdateAvailable (false);
		e->setNewVersion (VersionNumber ());
	EndFor

	notifyScope.finish ();

	if(errorContext.hasErrors () && forceDialog)
		Alert::errorWithContext (ExtensionStrings::InstallationFailed (), true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ExtensionInstaller::isRestartRequired () const
{
	return restartRequired;
}

//************************************************************************************************
// ExtensionDragHandler
//************************************************************************************************

ExtensionDragHandler::ExtensionDragHandler (IView* view)
: DragHandler (view)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ExtensionDragHandler::matches (const FileType& fileType) const
{
	return fileType == ExtensionDescription::getFileType ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ExtensionDragHandler::install (UrlRef path)
{
	ExtensionManager::instance ().deferInstallWithUI (path);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ExtensionDragHandler::drop (const DragEvent& event)
{
	UnknownPtr<IUrl> path (event.session.getItems ().getFirst ());
	ASSERT (path)
	if(path)
		install (*path);

	return DragHandler::drop (event);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IUnknown* ExtensionDragHandler::prepareDataItem (IUnknown& item, IUnknown* context)
{
	UnknownPtr<IUrl> path (&item);
	if(path && matches (path->getFileType ()))
	{
		AutoPtr<IImage> icon (FileIcons::instance ().createIcon (*path));
		String fileName;
		path->getName (fileName, false);
		spriteBuilder.addItem (icon, fileName);
		path->retain ();
		return path;
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ExtensionDragHandler::finishPrepare ()
{
	if(!getData ().isEmpty ())
		spriteBuilder.addHeader (XSTR (InstallExtension), -1);
}
