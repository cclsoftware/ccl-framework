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
// Filename    : ccl/extras/packages/unifiedpackageinstaller.cpp
// Description : Unified Package Installer
//
//************************************************************************************************

#include "ccl/extras/packages/unifiedpackageinstaller.h"
#include "ccl/extras/packages/packagehandlerregistry.h"
#include "ccl/extras/packages/factorycontentpackages.h"

#include "ccl/base/message.h"
#include "ccl/base/asyncoperation.h"
#include "ccl/base/storage/file.h"
#include "ccl/base/storage/packageinfo.h"
#include "ccl/base/security/featureauthorizer.h"

#include "ccl/public/text/translation.h"
#include "ccl/public/systemservices.h"
#include "ccl/public/system/ifileutilities.h"
#include "ccl/public/system/ipackagehandler.h"
#include "ccl/public/system/ipackagefile.h"
#include "ccl/public/extras/icontentinstaller.h"

namespace CCL {
namespace Packages {

//************************************************************************************************
// InstallTransaction
//************************************************************************************************

class InstallTransaction: public Object
{
public:
	DECLARE_CLASS (InstallTransaction, Object)

	InstallTransaction (Install::File* file = nullptr, UrlRef srcPath = Url (), bool isExtension = true, bool isLocal = false, UnifiedPackageAction* action = nullptr);

	PROPERTY_SHARED_AUTO (Install::File, file, File)
	PROPERTY_OBJECT (Url, srcPath, SrcPath)
	PROPERTY_BOOL (extension, Extension)
	PROPERTY_BOOL (local, Local)
	PROPERTY_BOOL (installed, Installed)
	PROPERTY_SHARED_AUTO (UnifiedPackageAction, action, Action)
};

} // namespace Packages
} // namespace CCL

using namespace CCL;
using namespace Packages;
using namespace Install;

//////////////////////////////////////////////////////////////////////////////////////////////////
// Strings
//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_XSTRINGS ("UnifiedPackageInstaller")
	XSTRING (RestartRequired, "The installed content will be available next time you start $APPNAME.")
	XSTRING (InstallFailed, "Installation failed for %(1).")
END_XSTRINGS

//////////////////////////////////////////////////////////////////////////////////////////////////
// Initialization
//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_KERNEL_INIT_LEVEL (RegisterUnifiedPackageInstaller, kSetupLevel)
{
	PackageHandlerRegistry::instance ().registerHandler (&UnifiedPackageInstaller::instance ());
	return true;
}

CCL_KERNEL_TERM_LEVEL (UnRegisterUnifiedPackageInstaller, kSetupLevel)
{	
	UnifiedPackageInstaller::instance ().terminate ();
	
	PackageHandlerRegistry::instance ().unregisterHandler (&UnifiedPackageInstaller::instance ());
}

//************************************************************************************************
// PackageInstallerStrings
//************************************************************************************************

StringRef PackageInstallerStrings::InstallFailed () { return XSTR (InstallFailed); }
StringRef PackageInstallerStrings::RestartRequired () { return XSTR (RestartRequired); }

//************************************************************************************************
// UnifiedPackageInstaller
//************************************************************************************************

DEFINE_CLASS (UnifiedPackageInstaller, Object)

DEFINE_STRINGID_MEMBER_ (UnifiedPackageInstaller, kRunInstallation, "runInstallation")

//////////////////////////////////////////////////////////////////////////////////////////////////

UnifiedPackageInstaller::UnifiedPackageInstaller ()
: preparingInstallation (false),
  restartRequired (false),
  checkAuthorization (false),
  insideInstallationDone (false)
{
	engine.setObserver (this);
	installQueue.objectCleanup (true);
	activeTransactions.objectCleanup (true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

UnifiedPackageInstaller::~UnifiedPackageInstaller ()
{
	cancelSignals ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Install::ContentInstallEngine& UnifiedPackageInstaller::getInstallEngine ()
{
	return engine;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void UnifiedPackageInstaller::initialize (const VersionNumber& versionNumber)
{
	version = versionNumber;
	if(history.restore () == true)
	{
		// check if manifest has changed
		if(version > history.getVersion ())
			saveHistory ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void UnifiedPackageInstaller::terminate ()
{
	// make sure to save the history if it does not exist yet
	if(history.getVersion () == VersionNumber ())
		saveHistory ();

	// must release transactions before main() ends (images etc)
	if(activeTransactions.isEmpty () == false)
	{
		engine.abortInstallation ();
		activeTransactions.removeAll ();
	}
	if(finishedTransactions.isEmpty() == false)
	{
		for(InstallTransaction* transaction : iterate_as<InstallTransaction> (finishedTransactions))
			transaction->release ();
		finishedTransactions.removeAll ();
	}
	
	stopTimer ();
	cancelSignals ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool UnifiedPackageInstaller::canHandle (UnifiedPackage* package) const
{
	if(engine.getContentServer () == nullptr)
		return false;

	if(package)
	{
		if(package->getData<Manifest> ())
			return true;
		CCL::File* file = nullptr;
		for(int i = 0; file = package->getData<CCL::File> (i); i++)
			if(file->exists () && file->isFile ())
				return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void UnifiedPackageInstaller::getActions (Container& actions, UnifiedPackage* package)
{
	if(canHandle (package))
		actions.add (createAction (package, kInstall));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void UnifiedPackageInstaller::updateAction (UnifiedPackageAction& action)
{
	action.setState (UnifiedPackageAction::kInvalid);

	UnifiedPackage* package = action.getPackage ();
	if(!canHandle (package))
		return;

	if(action.getId () == kInstall)
	{
		if(package->isLocalPackage () && package->isLocalInstallationAllowed () == false)
			return;
		
		StringRef authId = package->getAuthorizerId ();
		if(checkAuthorization && authId.isEmpty () == false)
		{
			if(isAuthorized (*package) == false)
			{
				action.setState (UnifiedPackageAction::kDisabled);
				return;
			}
		}

		action.isCancelEnabled (true);
		action.isResumable (true);
		
		Manifest* manifest = nullptr;
		for(int i = 0; manifest = package->getData<Manifest> (i); i++)
		{
			if(Install::File* file = manifest->findFile (package->getId ()))
			{
				if(InstallTransaction* transaction = findTransaction (package->getId ()))
				{
					bool paused = engine.isInstallationPaused (*file);
					action.setState (paused ? UnifiedPackageAction::kPaused : UnifiedPackageAction::kActive);
					return;
				}
				else
				{
					if(isInstalled (file))
						return;
					action.setState (UnifiedPackageAction::kEnabled);
					return;
				}
			}
		}

		CCL::File* file = nullptr;
		for(int i = 0; file = package->getData<CCL::File> (i); i++)
		{
			if(file->isFile () && file->exists ())
			{
				if(findHandlerForFile (file->getPath ()))
				{
					action.setState (UnifiedPackageAction::kEnabled);
					return;
				}
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool UnifiedPackageInstaller::performAction (UnifiedPackageAction& action)
{
	if(engine.getContentServer () == nullptr)
		return false;

	StringID actionId = action.getId ();

	UnifiedPackage* package = action.getPackage ();
	if(package == nullptr)
		return false;

	StringRef authId = package->getAuthorizerId ();
	if(checkAuthorization && authId.isEmpty () == false)
	{
		if(isAuthorized (*package) == false)
			return false;
	}

	Manifest* manifest = nullptr;
	for(int i = 0; manifest = package->getData<Manifest> (i); i++)
	{
		for(Install::File* file : iterate_as<Install::File> (manifest->getFiles ()))
		{
			if(file->getID () == package->getId ())
			{
				if(actionId == kInstall)
				{
					Url srcPath;
					if(!file->getSourceFolder ().isEmpty ())
						srcPath.descend (file->getSourceFolder ());
					srcPath.descend (file->getFileName ());

					bool isLocal = System::GetFileSystem ().fileExists (srcPath);
					bool isExtension = package->getOrigin () != UnifiedPackage::kFactoryContentOrigin && manifest != package->getData<Manifest> (FactoryContentPackageSource::kSourceName);

					return addTransaction (file, Url (), isExtension, isLocal, &action);
				}
				break;
			}
		}
	}

	CCL::File* file = nullptr;
	for(int i = 0; file = package->getData<CCL::File> (i); i++)
	{
		if(file->isFile () && file->exists ())
		{
			String fileName;
			String pathName;
			Url srcPath = file->getPath ();
			srcPath.getName (fileName);
			srcPath.getPathName (pathName);
			srcPath.setPath (nullptr);

			AutoPtr<Install::File> installFile = NEW Install::File;
			installFile->setFileName (fileName);
			installFile->setSourceFolder (pathName);
			installFile->setTitle (package->getId ());
			installFile->setID (package->getId ());

			if(actionId == kInstall)
				return addTransaction (installFile, srcPath, package->getOrigin () != UnifiedPackage::kFactoryContentOrigin, true, &action);
		}
	}

	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool UnifiedPackageInstaller::cancelAction (UnifiedPackageAction& action)
{
	if(action.getId () == kInstall)
		return cancelInstallation (action.getPackage ()->getId ());
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool UnifiedPackageInstaller::pauseAction (UnifiedPackageAction& action, bool state)
{
	if(action.getId () == kInstall)
		if(engine.pauseInstallation (action.getPackage ()->getId (), state))
		{
			action.setState (state ? UnifiedPackageAction::kPaused : UnifiedPackageAction::kActive);
			return true;
		}
	
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Component* UnifiedPackageInstaller::createComponent (UnifiedPackage* package)
{
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool UnifiedPackageInstaller::addTransaction (Install::File* file, UrlRef srcPath, bool isExtension, bool isLocal, UnifiedPackageAction* action)
{
	file->retain ();
	installQueue.add (file);

	activeTransactions.add (NEW InstallTransaction (file, srcPath, isExtension, isLocal, action));
	action->setState (UnifiedPackageAction::kActive);

	if(engine.isInstalling () == false || engine.isMultipleTransactions ())
		(NEW Message (kRunInstallation))->post (this);

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool UnifiedPackageInstaller::isInstalled (Install::File* file) const
{
	return System::GetFileTypeRegistry ().getHandlers ().getState (*file) == IFileHandler::kInstalled;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool UnifiedPackageInstaller::isAuthorized (const UnifiedPackage& package) const
{
	Security::FeatureAuthorizer authorizer (package.getAuthorizerId ());
	return authorizer.isAccessible (package.getId ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void UnifiedPackageInstaller::performInstallationStep (Step step)
{
	if(step == Step::kPrepare)
	{
		preparingInstallation = true;
		Promise p = engine.beginInstallationAsync ();
		p.then ([&] (IAsyncOperation& op)
		{
			preparingInstallation = false;
			if(op.getState () == IAsyncInfo::kCompleted)
			{
				performInstallationStep (Step::kInstall);
			}
			else
			{
				for(InstallTransaction* transaction : iterate_as<InstallTransaction> (activeTransactions))
					transaction->getAction ()->complete (false);

				activeTransactions.removeAll ();
			}
		});	
	}
	else if(step == Step::kInstall)
	{
		for(Install::File* file : iterate_as<Install::File> (installQueue))
		{
			if(file == nullptr)
				continue;
			bool isExtension = true;
			bool isLocal = false;
			Url srcPath;
			InstallTransaction* transaction = findTransaction (file->getID ());
			if(transaction)
			{
				isExtension = transaction->isExtension ();
				srcPath = transaction->getSrcPath ();
				isLocal = transaction->isLocal ();
			}
			if(isLocal == false)
			{
				bool succeeded = false;
				if(!installationSourcePath.isEmpty ())
					succeeded = engine.installLocalFile (*file, installationSourcePath, false);
				if(!succeeded)
					engine.installRemoteFile (*file, isExtension);
			}
			else if(engine.installLocalFile (*file, srcPath, true))
			{
				installQueue.remove (file);
				file->release ();
				continue;
			}
			else
				engine.installLocalFile (*file, srcPath, false);
		}

		if(!engine.performInstallation ())
		{
			engine.abortInstallation ();
			for(Install::File* file : iterate_as<Install::File> (installQueue))
				if(InstallTransaction* transaction = findTransaction (file->getID ()))
				{
					activeTransactions.remove (transaction);
					transaction->getAction ()->complete (false);
					transaction->release ();
				}
		}

		installQueue.removeAll ();
	
		if(activeTransactions.isEmpty () == false)
			startTimer ();	
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void UnifiedPackageInstaller::runInstallation ()
{
	if(engine.isInstalling () && engine.isMultipleTransactions () == false)
		return;

	if(preparingInstallation)
		return;

	performInstallationStep (Step::kPrepare);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool UnifiedPackageInstaller::cancelInstallation (StringRef id)
{
	bool succeeded = engine.cancelInstallation (id);
	for(Install::File* file : iterate_as<Install::File> (installQueue))
		if(file->getID () == id)
		{
			installQueue.remove (file);
			onFileInstallationCanceled (*file);
			file->release ();
			return true;
		}
	return succeeded;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

InstallTransaction* UnifiedPackageInstaller::findTransaction (StringRef id) const
{
	for(InstallTransaction* transaction : iterate_as<InstallTransaction> (activeTransactions))
		if(transaction->getFile ()->getID () == id)
			return transaction;
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IFileInstallHandler* UnifiedPackageInstaller::findHandlerForFile (UrlRef path) const
{
	String fileName;
	path.getName (fileName);
	FileDescriptor descriptor (fileName);

	AutoPtr<IPackageFile> packageFile = System::GetPackageHandler ().openPackage (path);
	if(packageFile.isValid ())
	{
		PackageInfo info;
		if(info.loadFromPackage (*packageFile))
			descriptor.getMetaInfo ().copyFrom (info);
	}

	IterForEachUnknown (System::GetFileTypeRegistry ().newHandlerIterator (), unk)
		UnknownPtr<IFileInstallHandler> fileHandler = unk;
		if(fileHandler && fileHandler->canHandle (descriptor))
			return fileHandler;
	EndFor
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void UnifiedPackageInstaller::updateFileInstallationProgress (const Install::File& file, double progress)
{
	if(InstallTransaction* transaction = findTransaction (file.getID ()))
		transaction->getAction ()->progress (progress);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void UnifiedPackageInstaller::onFileInstallationPaused (const Install::File& file, bool state)
{
	if(InstallTransaction* transaction = findTransaction (file.getID ()))
		transaction->getAction ()->onPause (state);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void UnifiedPackageInstaller::onFileInstallationSucceeded (const Install::File& file, const DateTime& time, UrlRef path)
{
	if(InstallTransaction* transaction = findTransaction (file.getID ()))
	{
		activeTransactions.remove (transaction);
		transaction->setInstalled (true);
		finishedTransactions.add (transaction);

		if(activeTransactions.isEmpty ())
			stopTimer ();		
	}

	history.setInstalled (file, time, path);
	saveHistory ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void UnifiedPackageInstaller::onFileInstallationFailed (const Install::File& file)
{
	if(InstallTransaction* transaction = findTransaction (file.getID ()))
	{
		activeTransactions.remove (transaction);
		String message;
		message.appendFormat (XSTR (InstallFailed), transaction->getFile ()->getTitle ());
		transaction->getAction ()->reportEvent ({ message, Alert::kError });
		finishedTransactions.add (transaction);

		if(activeTransactions.isEmpty ())
			stopTimer ();		
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void UnifiedPackageInstaller::onFileInstallationCanceled (const Install::File& file)
{
	if(InstallTransaction* transaction = findTransaction (file.getID ()))
	{
		activeTransactions.remove (transaction);
		finishedTransactions.add (transaction);

		if(activeTransactions.isEmpty ())
			stopTimer ();		
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void UnifiedPackageInstaller::onInstallationDone ()
{
	if(insideInstallationDone)
		return;
	ScopedVar<bool> scope (insideInstallationDone, true);

	for(InstallTransaction* transaction : iterate_as<InstallTransaction> (finishedTransactions))
	{
		if(transaction->isInstalled () && installQueue.isEmpty () && restartRequired)
		{
			transaction->getAction ()->requestRestart (XSTR (RestartRequired));
			restartRequired = false;
		}
		transaction->getAction ()->complete (transaction->isInstalled ());
		transaction->release ();
	}
	finishedTransactions.removeAll ();
	if(installQueue.isEmpty () == false)
		runInstallation ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void UnifiedPackageInstaller::onRestartRequired ()
{
	restartRequired = true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void UnifiedPackageInstaller::setContentServer (IContentServer* server)
{
	engine.setContentServer (server);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void UnifiedPackageInstaller::setAppProductID (StringRef identity)
{
	engine.setAppProductID (identity);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void UnifiedPackageInstaller::addFileType (const FileType& fileType, StringRef targetFolder)
{
	engine.addFileType (fileType, targetFolder);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void UnifiedPackageInstaller::setTargetPath (UrlRef path)
{
	engine.setTargetPath (path);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool UnifiedPackageInstaller::getPackageLocation (Url& path, StringRef packageId) const
{
	HistoryEntry* entry = history.lookup (packageId);
	if(entry)
		path = entry->getPath ();
	else
		path = Url ();
	return entry != nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

VersionNumber UnifiedPackageInstaller::getHistoryVersion () const
{
	return history.getVersion ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void UnifiedPackageInstaller::setSourcePath (UrlRef path)
{
	installationSourcePath = path;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool UnifiedPackageInstaller::isInstallingPackage (StringRef packageId) const
{
	return findTransaction (packageId) != nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void UnifiedPackageInstaller::onIdleTimer ()
{
	engine.updateInstallationProgress ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API UnifiedPackageInstaller::notify (ISubject* subject, MessageRef msg)
{
	if(msg == kRunInstallation)
		runInstallation ();
	else
		SuperClass::notify (subject, msg);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void UnifiedPackageInstaller::saveHistory ()
{
	history.store (version);
}

//************************************************************************************************
// InstallTransaction
//************************************************************************************************

DEFINE_CLASS_HIDDEN (InstallTransaction, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

InstallTransaction::InstallTransaction (Install::File* file, UrlRef srcPath, bool isExtension, bool isLocal, UnifiedPackageAction* action)
: file (nullptr),
  srcPath (srcPath),
  extension (isExtension),
  local (isLocal),
  action (nullptr),
  installed (false)
{
	setFile (file);
	setAction (action);
}
