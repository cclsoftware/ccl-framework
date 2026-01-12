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
// Filename    : ccl/extras/extensions/contentinstallengine.h
// Description : Content Installation Engine
//
//************************************************************************************************

#ifndef _ccl_contentinstallengine_h
#define _ccl_contentinstallengine_h

#include "ccl/extras/extensions/installdata.h"
#include "ccl/extras/extensions/icontentserver.h"

#include "ccl/public/collections/unknownlist.h"
#include "ccl/public/network/web/itransfermanager.h"

namespace CCL {
interface IFileInstallHandler;
class SignalSink;

namespace Install {

//************************************************************************************************
// IContentInstallEngineObserver
//************************************************************************************************

interface IContentInstallEngineObserver: IUnknown
{
	/** A file has been installed successfully */
	virtual void onFileInstallationSucceeded (const File& file, const DateTime& time, UrlRef path) = 0;

	/** A file could not be installed */
	virtual void onFileInstallationFailed (const File& file) = 0;

	/** A file transaction has been canceled */
	virtual void onFileInstallationCanceled (const File& file) = 0;

	/** All transactions have been processed */
	virtual void onInstallationDone () = 0;

	/** The application needs to be restarted */
	virtual void onRestartRequired () = 0;

	/** A file transaction progress has changed */
	virtual void updateFileInstallationProgress (const File& file, double progress) = 0;
	
	/** A file installation was paused */
	virtual void onFileInstallationPaused (const File& file, bool state) = 0;

	DECLARE_IID (IContentInstallEngineObserver)
};

//************************************************************************************************
// ContentInstallEngine
//************************************************************************************************

class ContentInstallEngine: public Object
{
public:
	DECLARE_CLASS (ContentInstallEngine, Object)

	ContentInstallEngine ();
	~ContentInstallEngine ();

	PROPERTY_POINTER (IContentInstallEngineObserver, observer, Observer)
	PROPERTY_POINTER (IContentServer, contentServer, ContentServer)
	PROPERTY_OBJECT (Url, targetPath, TargetPath)
	PROPERTY_STRING (appProductId, AppProductID)
	PROPERTY_BOOL (delayTransactionFinishInModalMode, DelayTransactionFinishInModalMode) // default: true	
	PROPERTY_BOOL (multipleTransactions, MultipleTransactions) // allow simultanious transfers and multiple transaction at the same time. default: false

	static const String kInstallerFolder; // folder for downloaded installer files
	void addFileType (const FileType& fileType, StringRef targetFolder);
	void getTargetPathForFile (IUrl& dstPath, const File& file) const;

	bool beginInstallation ();
	IAsyncOperation* beginInstallationAsync ();

	bool installRemoteFile (File& file, bool isExtension = false);
	bool installLocalFile (File& file, UrlRef srcFolder, bool skipTransfer = false);

	void getExistingFiles (Container& existingFiles) const;
	bool skipExistingFiles ();
	void skipFiles (const Container& files);
	
	bool performInstallation ();
	bool isInstalling () const;
	bool cancelInstallation (const File& file);
	bool pauseInstallation (const File& file, bool state);
	bool isInstallationPaused (const File& file);
	bool updateInstallationProgress ();
	bool abortInstallation ();
	int countFiles () const;
	
	// Object
	void CCL_API notify (ISubject* subject, MessageRef msg) override;

protected:
	struct FolderDefinition
	{
		String folderName;
		FileType fileType;

		FolderDefinition (StringRef folderName = nullptr, const FileType& fileType = FileType ())
		: folderName (folderName),
		  fileType (fileType)
		{}
	};
	class Transaction;
	DECLARE_STRINGID_MEMBER (kFinishTransactions)
	DECLARE_STRINGID_MEMBER (kInstallationDone)
	
	Vector<FolderDefinition> targetFolders;
	AutoPtr<IUnknown> credentials;

	Transaction* newTransaction;
	ObjectArray startedTransactions;
	ObjectArray deferredTransactions;
	bool insideTransaction;
	bool insideUpdate;
	bool isNetworkActivity;
	SignalSink& signalSink;
	
	void appendTransfer (Web::ITransfer* transfer);

	IFileInstallHandler* findHandlerForFile (const File& file) const;
	bool isExisting (const Web::ITransfer* transfer) const;

	void finishTransaction (Transaction* transaction);	
	void updateNetworkActivity ();
};

} // namespace Install
} // namespace CCL

#endif // _ccl_contentinstallengine_h
