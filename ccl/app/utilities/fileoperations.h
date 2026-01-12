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
// Filename    : ccl/app/utilities/fileoperations.h
// Description : File Operations: Copy / Move / Delete
//
//************************************************************************************************

#ifndef _ccl_fileoperations_h
#define _ccl_fileoperations_h

#include "ccl/app/utilities/batchoperation.h"

namespace CCL {

//************************************************************************************************
// FileCopyTask
//************************************************************************************************

class FileCopyTask: public BatchOperation::Task
{
public:
	DECLARE_CLASS (FileCopyTask, BatchOperation::Task)
	
	// Task
	String getProgressText () override;
	bool prepare () override;
	bool perform (IProgressNotify* progress) override;
	void onFinished () override;
	void onCanceled () override;
};

//************************************************************************************************
// FileMoveTask
//************************************************************************************************

class FileMoveTask: public BatchOperation::Task
{
public:
	DECLARE_CLASS (FileMoveTask, BatchOperation::Task)
	
	// Task
	String getProgressText () override;
	bool prepare () override;
	bool perform (IProgressNotify* progress) override;
};

//************************************************************************************************
// FileDeleteTask
//************************************************************************************************

class FileDeleteTask: public BatchOperation::Task
{
public:
	DECLARE_CLASS (FileDeleteTask, BatchOperation::Task)
		
	FileDeleteTask ();

	static void resetBypassTrashState ();
	
	PROPERTY_VARIABLE (int, mode, Mode)

	// Task
	String getProgressText () override;
	bool perform (IProgressNotify* progress) override;

protected:
	static int bypassTrashState;
	enum { kMustAskBypassTrash, kBypassTrashAllowed, kBypassTrashDenied };

	bool removeObject (UrlRef path, int mode);
};

//************************************************************************************************
// FileTransferOperation
//************************************************************************************************

class FileTransferOperation: public BatchOperation
{
public:
	enum TransferMode { kCopy, kMove, kDelete };

	FileTransferOperation (int transferMode = kCopy);

	/// configuration (must be done before adding files)
	PROPERTY_VARIABLE (int, transferMode, TransferMode)
	PROPERTY_OBJECT (CCL::Url, destFolder, DestFolder) ///< common destination folder for copy or move
	PROPERTY_BOOL (silent, Silent)

	Task* addFile (UrlRef path, Object* userData = nullptr);

protected:
	void makeDestPath (Task& task, UrlRef destFolder);
	String createUserMessageText ();

	// BatchOperation
	bool prepare () override;
	IAsyncOperation* prepareAsync () override;
	void onFinished (bool allSucceeded) override;
};

//************************************************************************************************
// NewFolderOperation
/** Asks user for a foldername and creates the folder. */
//************************************************************************************************

class NewFolderOperation
{
public:
	bool run (UrlRef parentFolder);
};

//************************************************************************************************
// FileStrings
//************************************************************************************************

namespace FileStrings
{
	StringRef CopyingFiles ();
	StringRef MovingFiles ();
	StringRef DeletingFiles ();
	StringRef RenameFile ();
	StringRef RenameFolder ();
	StringRef DeleteFile ();
	StringRef DeleteFolder ();
	StringRef NewFolder ();
	StringRef MoveToNewFolder ();
	StringRef NewFolderTitle ();
	StringRef RenameFileTitle ();
	StringRef RenameFolderTitle ();
	StringRef Copy ();
	StringRef Move ();
	StringRef CopyTo ();
	StringRef MoveTo ();
	StringRef MoveToFolder ();
	StringRef CopyToRoot ();
	StringRef MoveToRoot ();
	StringRef DoYouWantToCopyTheseFilesTo (int number, const IUrl* example = nullptr);
	StringRef DoYouWantToMoveTheseFilesTo (int number, const IUrl* example = nullptr);
	StringRef DoYouWantToDeleteTheseFiles (int number, const IUrl* example = nullptr);
	StringRef DoYouWantToCopyThisFolderTo ();
	StringRef DoYouWantToMoveThisFolderTo ();
	StringRef DoYouWantToDeleteThisFolder ();
	StringRef SomeFilesCouldNotBeCopied ();
	StringRef SomeFilesCouldNotBeMoved ();
	StringRef SomeFilesCouldNotBeDeleted ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// inline
//////////////////////////////////////////////////////////////////////////////////////////////////

inline FileTransferOperation::FileTransferOperation (int transferMode): transferMode (transferMode), silent (false) {}

//////////////////////////////////////////////////////////////////////////////////////////////////

} // namespace CCL

#endif // _ccl_fileoperations_h
