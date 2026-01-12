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
// Filename    : ccl/extras/extensions/contentinstallengine.cpp
// Description : Content Installation Engine
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "contentinstallengine.h"

#include "ccl/public/extras/icontentinstaller.h"

#include "ccl/base/message.h"
#include "ccl/base/asyncoperation.h"
#include "ccl/base/signalsource.h"
#include "ccl/public/base/itrigger.h"

#include "ccl/public/gui/framework/ialert.h"
#include "ccl/public/gui/framework/iwindow.h"
#include "ccl/public/gui/framework/iuserinterface.h"
#include "ccl/public/guiservices.h"

#include "ccl/public/network/web/iwebcredentials.h"
#include "ccl/public/netservices.h"

#include "ccl/public/system/ifileutilities.h"
#include "ccl/public/system/isysteminfo.h"
#include "ccl/public/system/inativefilesystem.h"
#include "ccl/public/systemservices.h"

using namespace CCL;
using namespace Install;
using namespace Web;

//************************************************************************************************
// ContentInstallEngine::Transaction
//************************************************************************************************

class ContentInstallEngine::Transaction: public Object,
						                 public ITriggerAction
{
public:
	DECLARE_CLASS_ABSTRACT (Transaction, Object)
	
	Transaction (ContentInstallEngine* engine)
	: engine (engine)	  
	{}

	void addCandidate (ITransfer* t)
	{
		canditates.append (t);
	}

	void removeCandidate (ITransfer* t)
	{
		if(canditates.remove (t))
			t->release ();
	}

	void releaseCandidates ()
	{
		canditates.removeAll ();
	}

	typedef InterfaceList<Web::ITransfer> TransferList;

	const TransferList& getCandidates () const {return canditates;}
	const TransferList& getExecutables () const {return executable;}

	// ITriggerAction
	void CCL_API execute (IObject* target) override;

	CLASS_INTERFACE (ITriggerAction, Object)

protected:
	ContentInstallEngine* engine;
	TransferList canditates;	 
	TransferList executable;
};

//************************************************************************************************
// IContentInstallEngineObserver
//************************************************************************************************

DEFINE_IID_ (IContentInstallEngineObserver, 0xb30cd602, 0x62e, 0x429a, 0xa4, 0x50, 0x66, 0x5c, 0x31, 0x67, 0x93, 0xba)

//************************************************************************************************
// ContentInstallEngine
//************************************************************************************************

DEFINE_CLASS (ContentInstallEngine, Object)
const String ContentInstallEngine::kInstallerFolder ("Installer");
DEFINE_STRINGID_MEMBER_ (ContentInstallEngine, kFinishTransactions, "finishTransactions")
DEFINE_STRINGID_MEMBER_ (ContentInstallEngine, kInstallationDone, "installationDone")

//////////////////////////////////////////////////////////////////////////////////////////////////

ContentInstallEngine::ContentInstallEngine ()
: insideTransaction (false),
  insideUpdate (false),
  isNetworkActivity (false),
  delayTransactionFinishInModalMode (true),
  multipleTransactions (false),
  contentServer (nullptr),
  observer (nullptr),
  newTransaction (nullptr),
  signalSink (*NEW SignalSink (Signals::kTransfers))
{
	startedTransactions.objectCleanup (true);
	deferredTransactions.objectCleanup (true);
	signalSink.setObserver (this);
	signalSink.enable (true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ContentInstallEngine::~ContentInstallEngine ()
{
	cancelSignals ();
	signalSink.enable (false);
	delete &signalSink;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ContentInstallEngine::beginInstallation ()
{
	if(isInstalling () && isMultipleTransactions () == false)
		return false;

	if(!credentials)
		credentials = contentServer->requestCredentials (IContentServer::kContentDownload);

	return credentials != nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IAsyncOperation* ContentInstallEngine::beginInstallationAsync ()
{
	if(isInstalling () && isMultipleTransactions () == false)
		return AsyncOperation::createFailed ();

	if(!credentials)
	{
		Promise p = contentServer->requestCredentialsAsync (IContentServer::kContentDownload);
		return return_shared <IAsyncOperation> (p.modifyState ([this] (const IAsyncOperation& operation)
		{
			credentials.share (operation.getResult ().asUnknown ());
			return credentials.isValid () ? IAsyncOperation::kCompleted : IAsyncOperation::kFailed;
		}));
	}

	return AsyncOperation::createCompleted ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ContentInstallEngine::appendTransfer (ITransfer* transfer)
{		
	if(newTransaction == nullptr)
		newTransaction = NEW Transaction (this);

	newTransaction->addCandidate (transfer);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ContentInstallEngine::installRemoteFile (File& file, bool isExtension)
{
	if(insideTransaction)
		return false;

	if(credentials == nullptr)
		return false;

	Url url;
	String productId = isExtension ? file.getParentID () : appProductId;
	if(isExtension && productId.isEmpty ())
	{
		// special handling for product bundles sharing the same content
		productId = file.getSavedParentID ();
		ASSERT (!productId.isEmpty ())
	}

	contentServer->getContentURL (url, productId, file.getID (), isExtension, credentials);

	Url dstPath;
	getTargetPathForFile (dstPath, file);

	AutoPtr<IWebCredentials> webCredentials = contentServer->createCredentialsForURL (credentials);
	ITransfer* t = System::GetTransferManager ().createTransfer (dstPath, url, ITransfer::kDownload, webCredentials);
	t->setUserData (file.asUnknown ());
	t->setSrcDisplayString (contentServer->getServerTitle ());
	appendTransfer (t);

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ContentInstallEngine::installLocalFile (File& file, UrlRef srcFolder, bool skipTransfer)
{
	Url srcPath (srcFolder);
	if(!file.getSourceFolder ().isEmpty ())
		srcPath.descend (file.getSourceFolder ());
	srcPath.descend (file.getFileName ());

	if(skipTransfer)
	{
		if(System::GetFileSystem ().fileExists (srcPath) == false)
			return false;

		IFileInstallHandler* fileHandler = findHandlerForFile (file);
		if(fileHandler == nullptr)
			return false;

		fileHandler->beginInstallation (true);
		bool succeeded = fileHandler->performInstallation (file, srcPath);
		fileHandler->beginInstallation (false);

		if(succeeded)
		{
			if(fileHandler->isRestartRequired ())
				observer->onRestartRequired ();

			DateTime now;
			System::GetSystem ().getLocalTime (now);
			observer->onFileInstallationSucceeded (file, now, srcPath);
		}
		else
			observer->onFileInstallationFailed (file);
	}
	else
	{
		if(insideTransaction)
			return false;

		Url dstPath;
		getTargetPathForFile (dstPath, file);

		ITransfer* t = System::GetTransferManager ().createTransfer (dstPath, srcPath, ITransfer::kDownload);
		t->setUserData (file.asUnknown ());
		t->setSrcDisplayString (UrlDisplayString (srcFolder));
		
		appendTransfer (t);
	}

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ContentInstallEngine::getExistingFiles (Container& existingFiles) const
{
	if(newTransaction)	
		for(ITransfer* transfer : newTransaction->getCandidates ())
			if(isExisting (transfer))
				existingFiles.add (unknown_cast<File> (transfer->getUserData ()));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ContentInstallEngine::skipExistingFiles ()
{
	if(insideTransaction)
		return false;

	if(newTransaction)	
	{
		Vector<ITransfer*> existing;
		for(ITransfer* transfer : newTransaction->getCandidates ())
		{
			if(isExisting (transfer))
				existing.add (transfer);
		}

		for(ITransfer* transfer : existing)
			newTransaction->removeCandidate (transfer);
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ContentInstallEngine::skipFiles (const Container& files)
{
	if(newTransaction)
	{
		Vector<Web::ITransfer*> toBeRemoved;

		for(ITransfer* transfer : newTransaction->getCandidates ())
		{
			if(File* transferFile = unknown_cast<File> (transfer->getUserData ()))
				for(File* file : iterate_as<File> (files))
				{
					if(transferFile->getID () == file->getID ())
						toBeRemoved.add (transfer);
				}
		}

		for(ITransfer* transfer : toBeRemoved)
			newTransaction->removeCandidate (transfer);				
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int ContentInstallEngine::countFiles () const
{
	int count = 0;
	if(newTransaction)
		count += newTransaction->getCandidates ().count ();

	for(Transaction* transaction : iterate_as<Transaction> (startedTransactions))
		count += transaction->getCandidates ().count ();

	return count;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ContentInstallEngine::isInstalling () const
{
	return startedTransactions.isEmpty () == false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ContentInstallEngine::performInstallation ()
{
	ASSERT (observer)
	if(observer == nullptr)
		return false;

	if(insideTransaction)
		return false;

	if(newTransaction == nullptr)
	{
		observer->onInstallationDone ();
		return true;
	}

	AutoPtr<Transaction> transaction = newTransaction;
	newTransaction = nullptr;
	Transaction* started = nullptr;
	for(ITransfer* transfer : transaction->getCandidates ())
	{
		ITransfer* existing = System::GetTransferManager ().find (transfer);
		if(!existing || existing->getState () >= ITransfer::kCompleted)
		{
			int options = ITransferManager::kSuppressSignals;
			if(multipleTransactions)
			{
				// create one transaction for each transfer
				AutoPtr<Transaction> singleTransaction = NEW Transaction (this);
				singleTransaction->addCandidate (return_shared (transfer));
				transfer->addFinalizer (return_shared<ITriggerAction> (singleTransaction));
				startedTransactions.add (return_shared<Transaction> (singleTransaction));
			}
			else
			{
				options |= ITransferManager::kNonSimultaneous;
				transfer->addFinalizer (return_shared<ITriggerAction> (transaction));
				started = transaction;
			}

			if(System::GetTransferManager ().queue (transfer, options) != kResultOk)
				return false;
		}
	}
	
	if(started)
		startedTransactions.add (return_shared<Transaction> (started));

	updateNetworkActivity ();

	SignalSource (Signals::kTransfers).deferSignal (NEW Message (Signals::kRevealTransfer, transaction->getCandidates ().getFirst ()));

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ContentInstallEngine::cancelInstallation (const File& file)
{
	if(newTransaction)
	{
		for(ITransfer* transfer : newTransaction->getCandidates ())
			if(File* transferFile = unknown_cast<File> (transfer->getUserData ()))
				if(transferFile->getID () == file.getID ())
				{
					newTransaction->removeCandidate (transfer);
					return true;
				}					
	}
	
	for(SharedPtr<Transaction> transaction : iterate_as<Transaction> (startedTransactions)) 
	{	
		for(ITransfer* transfer : transaction->getCandidates ())
			if(File* transferFile = unknown_cast<File> (transfer->getUserData ()))
				if(transferFile->getID () == file.getID ())
				{
					bool succeeded = true;
					if(insideTransaction == false)
					{
						succeeded = System::GetTransferManager ().remove (transfer, true) == kResultOk;
						transaction->removeCandidate (transfer);
						observer->onFileInstallationCanceled (file);
					}
					else
						succeeded = false;
					return succeeded;
				}
	}

	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ContentInstallEngine::pauseInstallation (const File& file, bool state)
{
	for(SharedPtr<Transaction> transaction : iterate_as<Transaction> (startedTransactions))
		for(ITransfer* transfer : transaction->getCandidates ())
			if(File* transferFile = unknown_cast<File> (transfer->getUserData ()))
				if(transferFile->getID () == file.getID ())
					if(insideTransaction == false)
					{
						ITransferManager& manager = System::GetTransferManager ();
						tresult result = state ? manager.pause (transfer) : manager.resume (transfer);
						if(result == kResultOk)
							return true;
					}
	
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ContentInstallEngine::isInstallationPaused (const File& file)
{
	for(SharedPtr<Transaction> transaction : iterate_as<Transaction> (startedTransactions))
		for(ITransfer* transfer : transaction->getCandidates ())
			if(File* transferFile = unknown_cast<File> (transfer->getUserData ()))
				if(transferFile->getID () == file.getID ())
					return transfer->getState () == ITransfer::kPaused;
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ContentInstallEngine::updateInstallationProgress ()
{
	if(insideUpdate)
		return true;
	ScopedVar<bool> scope (insideUpdate, true);

	bool success = false;
	for(Transaction* transaction : iterate_as<Transaction> (startedTransactions))
	{
		SharedPtr<Unknown> lifeGuard (transaction);
		VectorForEach (transaction->getCandidates (), ITransfer*, transfer)
			if(File* transferFile = unknown_cast<File> (transfer->getUserData ()))
			{
				observer->updateFileInstallationProgress (*transferFile, transfer->getProgressValue ());
				success = true;
			}
		EndFor
	}
	return success;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ContentInstallEngine::abortInstallation ()
{
	if(insideTransaction)
		return false;

	safe_release (newTransaction);	
	for(Transaction* transaction : iterate_as<Transaction> (startedTransactions))
		transaction->releaseCandidates (); // explicitly needed because of circular refcount
	startedTransactions.removeAll ();
		
	updateNetworkActivity ();

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ContentInstallEngine::getTargetPathForFile (IUrl& dstPath, const File& file) const
{
	// query install location from file handler first
	Url handlerLocation;
	UnknownPtr<IFileHandler> fileHandler (findHandlerForFile (file));
	if(fileHandler)
		if(fileHandler->getDefaultLocation (handlerLocation, const_cast<File&> (file)) == false)
			handlerLocation = Url::kEmpty;

	if(handlerLocation.isEmpty () == false)
	{
		dstPath.assign (handlerLocation);
	}
	else
	{
		dstPath.assign (targetPath);

		if(!file.getTargetFolder ().isEmpty ())
			dstPath.descend (file.getTargetFolder (), Url::kFolder);
		else
		{
			FileType fileType;
			file.getFileType (fileType);
			for(FolderDefinition& def : targetFolders)
			{
				if(def.fileType == fileType)
				{
					dstPath.descend (def.folderName, Url::kFolder);
					break;
				}
			}
		}
	}

	dstPath.descend (file.getFileName ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ContentInstallEngine::finishTransaction (Transaction* transaction)
{
	if(startedTransactions.remove (transaction))
	{
		transaction->releaseCandidates ();
		transaction->release ();
	}
	
	updateNetworkActivity ();

	bool deferred = false;
	if(insideTransaction)
		deferred = true;

	if(deferred == false && delayTransactionFinishInModalMode)
	{
		int modeFlags = IDesktop::kProgressMode|IDesktop::kModalMode|IDesktop::kMenuLoopMode;
		deferred = System::GetDesktop ().isInMode (modeFlags) || System::GetAlertService ().getCurrentDialog () != nullptr;	
	}

	if(deferred)
	{
		deferredTransactions.add (return_shared (transaction));	
		(NEW Message (kFinishTransactions))->post (this, 500);
		return;
	}

	ScopedVar<bool> scope (insideTransaction, true);

	class FilesForHandler: public Object,
						   public Vector<ITransfer*>
	{
	public:
		PROPERTY_POINTER (IFileInstallHandler, handler, Handler)

		FilesForHandler (IFileInstallHandler* handler = nullptr)
		: handler (handler)
		{}

		bool equals (const Object& obj) const override
		{
			const FilesForHandler& other = (const FilesForHandler&)obj;
			return handler == other.handler;
		}

		int compare (const Object& obj) const override
		{
			const FilesForHandler& other = (const FilesForHandler&)obj;
			return handler->getInstallationOrder () - other.handler->getInstallationOrder ();
		}
	};

	ObjectArray handlerList;
	handlerList.objectCleanup (true);

	auto getFiles = [&] (IFileInstallHandler* handler)
	{
		FilesForHandler* files = (FilesForHandler*)handlerList.findEqual (FilesForHandler (handler));
		if(files == nullptr)
			handlerList.addSorted (files = NEW FilesForHandler (handler));
		return files;
	};

	// sort completed transfers by handler
	for(ITransfer* transfer : transaction->getExecutables ())
	{
		if(File* file = unknown_cast<File> (transfer->getUserData ()))
			if(transfer->getState () == ITransfer::kCompleted)
			{
				IFileInstallHandler* handler = findHandlerForFile (*file);
				ASSERT (handler != nullptr)
				if(handler)
					getFiles (handler)->add (transfer);
				else
					observer->onFileInstallationFailed (*file);
			}
			else if(transfer->getState () == ITransfer::kFailed)
				observer->onFileInstallationFailed (*file);
			else
				observer->onFileInstallationCanceled (*file);
	}

	// install files
	bool restartRequired = false;
	for(FilesForHandler* files : iterate_as<FilesForHandler> (handlerList))
	{
		IFileInstallHandler* handler = files->getHandler ();
		handler->beginInstallation (true);

		for(ITransfer* transfer : *files)
		{
			File* file = unknown_cast<File> (transfer->getUserData ());
			ASSERT (file != nullptr)
			if(file == nullptr)
				continue;

			Url path (transfer->getDstLocation ());
			bool installed = handler->performInstallation (*file, path) != 0;
			if(installed == true)
			{
				DateTime now;
				System::GetSystem ().getLocalTime (now);
				observer->onFileInstallationSucceeded (*file, now, path);
			}
			else
				observer->onFileInstallationFailed (*file);
		}

		handler->beginInstallation (false);

		if(handler->isRestartRequired ())
			restartRequired = true;
	}

	if(restartRequired)
		observer->onRestartRequired ();

	(NEW Message (kInstallationDone))->post (this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API ContentInstallEngine::notify (ISubject* subject, MessageRef msg)
{
	if(msg == kFinishTransactions)
	{
		ObjectArray toFinish;
		toFinish.objectCleanup (true);
		toFinish.add (deferredTransactions, ObjectArray::kShare);
		deferredTransactions.removeAll ();
		for(Transaction* transaction : iterate_as<Transaction> (toFinish))
			finishTransaction (transaction);
	}
	else if(msg == Signals::kTransferPaused)
	{
		UnknownPtr<ITransfer> transfer (msg[0]);
		bool state = msg.getArgCount () > 1 ? msg[1].asBool () : true;
		if(File* file = unknown_cast<File> (transfer->getUserData ()))
			observer->onFileInstallationPaused (*file, state);
	}
	else if(msg == kInstallationDone)
		observer->onInstallationDone ();
	else
		SuperClass::notify (subject, msg);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IFileInstallHandler* ContentInstallEngine::findHandlerForFile (const File& file) const
{
	IterForEachUnknown (System::GetFileTypeRegistry ().newHandlerIterator (), unk)
		if(UnknownPtr<IFileInstallHandler> fileHandler = unk)
		{
			if(fileHandler->canHandle (const_cast<File&> (file)))
				return fileHandler;
		}
	EndFor
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ContentInstallEngine::isExisting (const ITransfer* transfer) const
{
	if(transfer->getDstLocation ().isFile ())
		if(System::GetFileSystem ().fileExists (transfer->getDstLocation ()) != 0)
			return true;
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ContentInstallEngine::addFileType (const FileType& fileType, StringRef targetFolder)
{
	ASSERT (!targetFolder.isEmpty ())
	if(!targetFolder.isEmpty ())
		targetFolders.add (FolderDefinition (targetFolder, fileType));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ContentInstallEngine::updateNetworkActivity ()
{
	bool isNetworkActivityNew = isInstalling ();

	if(isNetworkActivityNew != isNetworkActivity)
	{	
		isNetworkActivity = isNetworkActivityNew;
		System::GetGUI ().setActivityMode (isNetworkActivity ? IUserInterface::ActivityMode::kBackground : IUserInterface::ActivityMode::kNormal, IUserInterface::ActivityType::kNetwork);
	}
}

//************************************************************************************************
// ContentInstallEngine::Transaction
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (ContentInstallEngine::Transaction, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API ContentInstallEngine::Transaction::execute (IObject* target)
{
	if(UnknownPtr<ITransfer> transfer = target)
	{
		executable.append (return_shared<ITransfer> (transfer));
		if(executable.count () == canditates.count ())
			engine->finishTransaction (this);
	}
}
